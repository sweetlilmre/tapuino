#ifndef _SERIAL_H
#define _SERIAL_H

void serial_init( void );
int serial_available( void );
char serial_read( void );

void serial_write( char data );
void serial_print( char* data );
void serial_printByte( unsigned char data );
void serial_println( char* data );
void serial_println_P( char* data );

#endif
