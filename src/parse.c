#include "common.h"
#include "parse.h"

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>


int
create_db_header( dbheader_t** header_out )
{
    dbheader_t* header = calloc( 1, DBHDRSIZE );
    if ( !header ) {
        puts( "Failed to allocate memory to create database header!" );
        return STATUS_FAILURE;
    }
    *header = (dbheader_t){
        .version  = 0x1,
        .count    = 0,
        .magic    = HEADER_MAGIC,
        .filesize = DBHDRSIZE
    };
    *header_out = header;
    
    return STATUS_SUCCESS;
}

void
convert_header_to( dbheader_t* header, convto_t to )
{
    unsigned short (*convs)( unsigned short );
    unsigned int (*convl)( unsigned int );
    convs = ( to == HOST ) ? &ntohs : &htons;    
    convl = ( to == HOST ) ? &ntohl : &htonl;    
    
    header->magic    = convl( header->magic );
    header->version  = convs( header->version );
    header->count    = convs( header->count );
    header->filesize = convl( header->filesize );
}

int
validate_db_header( int fd, dbheader_t** header_out )
{
    dbheader_t* header = calloc( 1, DBHDRSIZE );
    if ( !header ) {
        puts( "Failed to allocate momory to create database header!" );
        return STATUS_FAILURE;
    }
    if ( read( fd, header, DBHDRSIZE ) != DBHDRSIZE ) {
        perror( "read" );
        free( header );
        return STATUS_FAILURE;
    }
    convert_header_to( header, HOST );

    if ( header->magic != HEADER_MAGIC ) {
        puts( "Improper header magic!" );
        free( header );
        return STATUS_FAILURE;
    }
    if ( header->version != 1 ) {
        puts( "Improper header version!" );
        free( header );
        return STATUS_FAILURE;
    }
    struct stat dbstats = { 0 };
    if ( fstat( fd, &dbstats ) == -1 ) {
        perror( "fstat" );
        return STATUS_FAILURE;
    }
    if ( dbstats.st_size != header->filesize ) {
        puts( "Corrupted database!" );
        free( header );
        return STATUS_FAILURE;
    }

    *header_out = header;
    return STATUS_SUCCESS;
}

int
output_file( int fd, dbheader_t* header, employee_t* employees )
{
    lseek( fd, 0, SEEK_SET );
    
    dbheader_t header_out = *header;
    convert_header_to( &header_out, NET );
    if ( write( fd, &header_out, DBHDRSIZE ) != DBHDRSIZE ) {
        perror( "write" );
        return STATUS_FAILURE;
    }

    for ( unsigned short i = 0; i < header->count; ++i ) {
        employees[i].hours = htonl( employees[i].hours );
        if ( write( fd, &employees[i], EMPLSIZE ) != EMPLSIZE ) {
            perror( "write" );
            return STATUS_FAILURE;
        }
        employees[i].hours = ntohl( employees[i].hours );
    }

    return STATUS_SUCCESS;
}

int
read_employees( int fd, dbheader_t* header, employee_t** employees_out )
{
    unsigned short count = header->count;
    employee_t* employees = calloc( count, EMPLSIZE );
    if ( !employees ) {
        puts( "Failed to allocate memory for reading employees!" );
        return STATUS_FAILURE;
    }
    if ( read( fd, employees, count * EMPLSIZE ) != count * EMPLSIZE ) {
        perror( "read" );
        return STATUS_FAILURE;
    }
    for ( size_t i = 0; i < count; ++i )
        employees[i].hours = ntohl( employees[i].hours );

    *employees_out = employees;
    return STATUS_SUCCESS;
}

int
parse_addstring( char* string, employee_t* employee )
{
    // Parsing string
    long unsigned target_len = strlen( string ) - 2;
    char const* msg = "Invalid record (required \"<name>,<address>,<hours>\")!";
    char* parts[3];
    char* part = strtok( string, "," );
    for ( size_t i = 0; i < 3; ++i ) {
        if ( !part ) {
            puts( msg );
            return STATUS_FAILURE;
        }
        parts[i] = part;
        part = strtok( NULL, "," );
    }
    if (
        strlen( parts[0] ) + strlen( parts[1] ) + strlen( parts[2] )
            != target_len
    ) {
        puts( msg );
        return STATUS_FAILURE;        
    }
    
    char* tail = NULL;
    unsigned long hours = strtoul( parts[2], &tail, 10 );
    if ( *tail != '\0' ) {
        puts( msg );
        return STATUS_FAILURE;        
    }

    // Update employee
    strncpy( employee->name, parts[0], sizeof( employee->name ) );
    strncpy( employee->address, parts[1], sizeof( employee->address ) );
    employee->hours = (unsigned int)hours;
    
    return STATUS_SUCCESS; 
}

int
file_append_employee( int fd, dbheader_t* header, employee_t* employees )
{
    dbheader_t header_out = *header;
    convert_header_to( &header_out, NET );
    lseek( fd, 0, SEEK_SET );
    if ( write( fd, &header_out, DBHDRSIZE ) != DBHDRSIZE ) {
        perror( "write" );
        return STATUS_FAILURE;
    }    

    employee_t* employee = &employees[ header->count - 1 ];
    employee->hours = htonl( employee->hours );
    lseek( fd, 0, SEEK_END );
    if ( write( fd, employee, EMPLSIZE ) != EMPLSIZE ) {
        perror( "write" );
        return STATUS_FAILURE;
    }
    employee->hours = ntohl( employee->hours );
    
    return STATUS_SUCCESS;
}

int
parse_updatestring( char* string, char* name, unsigned int* hours )
{
    // Parse string
    long unsigned target_len = strlen( string ) - 1;
    char const* msg = "Invalid input (required \"<name>,<hours>\")!";
    char* parts[2];
    char* part = strtok( string, "," );
    for ( size_t i = 0; i < 2; ++i ) {
        if ( !part ) {
            puts( msg );
            return STATUS_FAILURE;
        }
        parts[i] = part;
        part = strtok( NULL, "," );
    }
    if ( strlen( parts[0] ) + strlen( parts[1] ) != target_len ) {
        puts( msg );
        return STATUS_FAILURE;        
    }
    
    char* tail = NULL;
    unsigned long number = strtoul( parts[1], &tail, 10 );
    if ( *tail != '\0' ) {
        puts( msg );
        return STATUS_FAILURE;        
    }

    // Copy results
    strncpy( name, parts[0], sizeof( parts[0] ) );
    *hours = (unsigned int)number;
    
    return STATUS_SUCCESS;
}

int
file_update_employee( int fd, unsigned short position, employee_t* employee )
{
    lseek( fd, DBHDRSIZE + position * EMPLSIZE, SEEK_SET );
    employee->hours = htonl( employee->hours );
    if ( write( fd, employee, EMPLSIZE ) != EMPLSIZE ) {
        perror( "write" );
        return STATUS_FAILURE;
    }
    employee->hours = ntohl( employee->hours );    

    return STATUS_SUCCESS;
}
