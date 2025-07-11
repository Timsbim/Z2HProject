#include "file.h"
#include "common.h"

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>


static mode_t const MODE = S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH;


int
create_db_file( char* filepath, bool override )
{
    if ( !override ) {
        int fd = open( filepath, O_RDONLY );
        if ( fd != -1 ) {
            close( fd );
            puts( "File already exists" );
            return STATUS_FAILURE;
        }
    }

    int flags = override ? O_RDWR | O_TRUNC : O_RDWR | O_CREAT;
    int fd = open( filepath, flags, MODE );
    if ( fd == -1 ) {
        perror( "open" );
        puts( "Failed to create database!" );
        return STATUS_FAILURE;
    }
    
    return fd;
}

int
open_db_file( char* filepath )
{
    int fd = open( filepath, O_RDWR, MODE );
    if ( fd == -1 ) {
        perror( "open" );
        return STATUS_FAILURE;
    }
    
    return fd; 
}

int
move_db_to_backup( char* filepath )
{
    char filepath_bak[256] = { '\0' };
    sprintf( filepath_bak, "%s.bak", filepath );
    if ( rename( filepath, filepath_bak ) == -1 ) {
        perror( "rename" );
        puts( "Failed to backup database!" );
        return STATUS_FAILURE;
    }
    
    return STATUS_SUCCESS;
}

int
unbackup_db( char* filepath )
{
    char filepath_bak[256] = { '\0' };
    sprintf( filepath_bak, "%s.bak", filepath );
    if ( rename( filepath_bak, filepath ) == -1 ) {
        perror( "rename" );
        puts( "Failed to restore dabase file from backup!" );
        return STATUS_FAILURE;
    }

    return STATUS_SUCCESS;
}

void
remove_db_backup( char* filepath )
{
    char filepath_bak[256] = { '\0' };
    sprintf( filepath_bak, "%s.bak", filepath );
    if ( unlink( filepath_bak ) == -1 ) {
        perror( "unlink" );
        printf( "Failed to remove backup (`%s`)!\n", filepath_bak );
    }
}
