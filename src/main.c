#include "common.h"
#include "file.h"
#include "parse.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>


void print_usage     ( char* );
int  add_employee    ( int, dbheader_t*, employee_t*, char* );
int  remove_employee ( dbheader_t*, employee_t*, char* );
int  update_hours    ( int, dbheader_t*, employee_t*, char* );
void list_employees  ( dbheader_t*, employee_t* );


int
main( int argc, char* argv[] )
{
    opterr = 0;
    
    bool newfile = false;
    char* filepath = NULL;
    char* addstring = NULL;
    char* removestring = NULL;
    char* updatestring = NULL;
    bool list = false;
    
    int c = 0;
    int dbfd = 0;
    dbheader_t* dbhdr = NULL;
    employee_t* employees = NULL;

    while ( (c = getopt( argc, argv, "nf:a:r:u:l" )) != -1 )
    {
        switch ( c ) {
            case 'n':
                newfile = true;
                break;
            case 'f':
                filepath = optarg;
                break;
            case 'a':
                addstring = optarg;
                break;
            case 'r':
                removestring = optarg;
                break;
            case 'u':
                updatestring = optarg;
                break;
            case 'l':
                list = true;
                break;
            case '?':
                printf( "Unknown option `-%c`\n", optopt );
                break;
            default:
                return EXIT_SUCCESS;
        }
    }

    if ( !filepath ) {
        puts( "Filepath is a required argument!" );
        print_usage( argv[0] );
        return EXIT_SUCCESS;
    }

    if ( newfile ) 
    {
        dbfd = create_db_file( filepath, false );
        if ( dbfd == STATUS_FAILURE ) {
            puts( "Failed to create database file!" );
            exit(EXIT_FAILURE);
        }
        if ( create_db_header( &dbhdr ) == STATUS_FAILURE ) {
            puts( "Failed to create database header!" );
            exit(EXIT_FAILURE);
        }
        if ( output_file( dbfd, dbhdr, employees ) == STATUS_FAILURE ) {
            puts( "Failed to write new database file!" );
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        dbfd = open_db_file( filepath );
        if ( dbfd == STATUS_FAILURE ) {
            puts( "Failed to open database file!" );
            exit(EXIT_FAILURE);
        }
        if ( validate_db_header( dbfd, &dbhdr ) == STATUS_FAILURE ) {
            puts( "Failed to validate database header!" );
            exit(EXIT_FAILURE);
        }
    }

    if ( read_employees( dbfd, dbhdr, &employees ) != STATUS_SUCCESS ) {
        puts( "Failed to read employees from database!" );
        close( dbfd );
        exit(EXIT_FAILURE);
    }
    
    if ( addstring )
    {
        unsigned long new_size = (dbhdr->count + 1) * EMPLSIZE;
        employees = realloc( employees, new_size );
        if ( !employees ) {
            puts( "Failed to allocate memory for new employee!" );
            exit(EXIT_FAILURE);
        }
        int status = add_employee( dbfd, dbhdr, employees, addstring );
        if ( status == STATUS_FAILURE ) {
            puts( "Failed to append new employee to database!" );
            exit(EXIT_FAILURE);
        }
    }
    else if ( removestring )
    {
        int delta = remove_employee( dbhdr, employees, removestring );
        if ( !delta ) {
            printf( "No employee with name `%s` in database\n", removestring );
            return EXIT_SUCCESS;
        }
        char* msg = "Removing %d employee(s) with name `%s` from database\n";
        printf( msg, delta, removestring );
        close( dbfd );
        if ( move_db_to_backup( filepath ) == STATUS_FAILURE )
            exit(EXIT_FAILURE);
        if (
            (dbfd = create_db_file( filepath, false )) == STATUS_FAILURE
            || output_file( dbfd, dbhdr, employees ) == STATUS_FAILURE
        ) {
            unbackup_db( filepath );
            puts( "Failed to remove employee(s) from database!" );
            exit(EXIT_FAILURE);
        }
        remove_db_backup( filepath );
    }
    else if ( updatestring )
    {
        int status = update_hours( dbfd, dbhdr, employees, updatestring );
        if ( status == STATUS_FAILURE) {
            puts( "Failed to update hours!" );
            exit(EXIT_FAILURE);
        }    
    }
    
    if ( list ) list_employees( dbhdr, employees );
    
    return EXIT_SUCCESS;
}


void
print_usage( char* name )
{
    printf(
        "Usage: %s -n -f <database file>\n"
        "\t-n   create new database file\n"
        "\t-f   (required) path to database file\n"
        "\t-a   add new employee (\"<name>,<address>,<hours>\")\n"
        "\t-r   remove employee (\"<name>\")\n"
        "\t-u   update hours of employee `name` (\"<name>,<hours>\")\n"
        "\t-l   list all employees in database\n",
        name
    );
}

int
add_employee( int fd, dbheader_t* header, employee_t* employees, char* string )
{
    // Parse input string
    employee_t* employee = &employees[ header->count ];
    if ( parse_addstring( string, employee ) == STATUS_FAILURE )
        return STATUS_FAILURE;
    
    // Update header (count and filesize)
    ++header->count;
    header->filesize += EMPLSIZE;
    
    char* msg = "Adding employee: name = %s, address = %s, hours = %u\n";
    printf( msg, employee->name, employee->address, employee->hours );
    
    return file_append_employee( fd, header, employees );
}

int
remove_employee( dbheader_t* header, employee_t* employees, char* name )
{
    // Adjust employees array: move records forward
    unsigned short count = 0;
    for ( unsigned short i = 0; i < header->count; ++i )
        if ( strcmp( employees[i].name, name ) != 0 )
            count++;

    // Exit if no employee with given name exists
    int delta = header->count - count;
    if ( !delta ) return 0;
    
    // Adjust header
    unsigned short count_orig = header->count;
    header->count = count;
    header->filesize = DBHDRSIZE + count * EMPLSIZE;

    // Update employee array
    count = 0;
    for ( unsigned short i = 0; i < count_orig; ++i )
        if ( strcmp( employees[i].name, name ) != 0 )
            employees[ count++ ] = employees[i];
            
    return delta;
}

int
update_hours( int fd, dbheader_t* header, employee_t* employees, char* string )
{
    // Parse input string
    char name[256] = { '\0' };
    unsigned int hours;
    if ( parse_updatestring( string, name, &hours ) == STATUS_FAILURE )
        return STATUS_FAILURE;

    // Update employee array and database file
    printf( "Updating employee `%s` with %u hours\n", name, hours );
    unsigned short count = 0;
    for ( unsigned short i = 0; i < header->count; ++i ) {
        if ( strcmp( employees[i].name, name ) == 0 ) {
            count++;
            employees[i].hours = hours;
            if ( file_update_employee( fd, i, &employees[i] ) == STATUS_FAILURE )
                return STATUS_FAILURE;
        }
    }
    if ( !count )
        printf( "No employee with name `%s` in database\n", name );
    
    return STATUS_SUCCESS;
}

void
list_employees( dbheader_t* header, employee_t* employees )
{
    if ( !header->count ) {
        puts( "Database empty" );
        return;
    }
        
    char* fmt = "Employee %u:\n\tName: %s\n\tAddress: %s\n\tHours: %u\n";
    for ( unsigned short i = 0; i < header->count; ++i ) {
        employee_t* employee = &employees[i];
        printf(fmt, i + 1, employee->name, employee->address, employee->hours);
    }
}
