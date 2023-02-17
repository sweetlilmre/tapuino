#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include "config.h"
#include "buffer.h"

#ifdef ENABLE_SERIAL

#define USART_BAUDRATE 57600
#define BAUD_PRESCALE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1)


void serial_init( void )
{
  RingBuffer_InitBuffer( &comms_rx_buffer );
  RingBuffer_InitBuffer( &comms_tx_buffer );

  DDRD |= _BV( 1 );
  UCSR0B |= _BV( RXEN0 ) | _BV( TXEN0 );          // Turn on the transmission and reception circuitry
  UCSR0C |= _BV( UCSZ00 ) | _BV( UCSZ01 );        // Use 8-bit character sizes
  UBRR0H = (unsigned char) (BAUD_PRESCALE >> 8);  // Load upper 8-bits of the baud rate value into the high byte of the UBRR register
  UBRR0L = (unsigned char) BAUD_PRESCALE;         // Load lower 8-bits of the baud rate value into the low byte of the UBRR register
  
  UCSR0B |= _BV( RXCIE0 );        // Enable the USART Recieve Complete interrupt
  
  sei(); // Enable the Global Interrupt Enable flag so that interrupts can be processed
}

int serial_available( void )
{
  return( comms_rx_buffer.Count );
}

void serial_write( char data )
{
  // wait for interrupt to clean
  while( RingBuffer_IsFull( &comms_tx_buffer ) );
  RingBuffer_Insert( &comms_tx_buffer, data );
  UCSR0B |= _BV( UDRIE0 ); 
}

void serial_print( char* data )
{
   uint8_t v;
   while ((v = *data++)) {
        serial_write(v);
   }
}

void serial_print_P( const char* data )
{
   uint8_t v;
   while ((v = pgm_read_byte(data++))) {
        serial_write(v);
   }
}

static const char* hex = "0123456789ABCDEF";
void serial_printByte( unsigned char data )
{
  serial_write( hex[ ( data >> 4 ) & 0x0F ] );
  serial_write( hex[ ( data & 0x0F ) ] );
}

void serial_println( char* data )
{
  serial_print( data );
  serial_write( '\n' );
}

void serial_println_P( const char* data )
{
  serial_print_P( data );
  serial_write( '\n' );
}

char serial_read( void )
{
  RingBuff_Data_t data = RingBuffer_Remove( &comms_rx_buffer );
  return( data );
}

SIGNAL(USART_RX_vect)
{
  char rx = UDR0;
  if( !RingBuffer_IsFull( &comms_rx_buffer ) )
  {
    RingBuffer_Insert( &comms_rx_buffer, rx );
  }
}

ISR(USART_UDRE_vect)
{
  if( RingBuffer_IsEmpty( &comms_tx_buffer ) )
  {
    UCSR0B &= ~_BV( UDRIE0 );
  }
  else
  {
    UDR0 = RingBuffer_Remove( &comms_tx_buffer );
  }
}

#endif //ENABLE_SERIAL

