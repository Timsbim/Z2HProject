#ifndef FILE_H
#define FILE_H


#include <stdbool.h>


int  create_db_file    ( char*, bool );
int  open_db_file      ( char* );
int  move_db_to_backup ( char* );
int  unbackup_db       ( char* );
void remove_db_backup  ( char* );


#endif
