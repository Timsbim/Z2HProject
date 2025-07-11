#ifndef PARSE_H
#define PARSE_H


#define HEADER_MAGIC 0x4c4c4144
#define DBHDRSIZE sizeof( dbheader_t )
#define EMPLSIZE  sizeof( employee_t )


typedef struct {
    unsigned int   magic;
    unsigned short version;
    unsigned short count;
    unsigned int   filesize;
} dbheader_t;

typedef struct {
    char         name[256];
    char         address[256];
    unsigned int hours;
} employee_t;

typedef enum {
    HOST,
    NET
} convto_t;


int  create_db_header      ( dbheader_t** );
void convert_header_to     ( dbheader_t*, convto_t );
int  validate_db_header    ( int, dbheader_t** );
int  file_write            ( int, dbheader_t*, employee_t* );
int  read_employees        ( int, dbheader_t*, employee_t** );
int  parse_addstring       ( char*, employee_t* );
int  parse_updatestring    ( char*, char*, unsigned int* );
int  file_append_employee  ( int, dbheader_t*, employee_t* );
int  file_update_employee  ( int, unsigned short, employee_t* );


#endif

