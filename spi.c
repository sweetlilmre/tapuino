#include <avr/io.h>
#include "integer.h"
#include "spi.h"


void SPI_Init()
{
  SPI_DDR  |= _BV(SPI_MOSI) | _BV(SPI_SCK) | _BV(SPI_SS);   // setup SPI output ports
  SPI_DDR  &= ~_BV(SPI_MISO); 								// setup SPI input ports

  SPI_PORT |= _BV(SPI_MOSI) | _BV(SPI_SCK) | _BV(SPI_SS);   // SPI outputs high
  SPI_PORT |= _BV(SPI_MISO); 								// set pull-up resistor on input
  SPCR = _BV(SPE) | _BV(MSTR) |_BV(SPR1);
}


void SPI_Speed_Slow()
{
  SPCR =   _BV(SPE) | _BV(MSTR) | _BV(SPR1) | _BV(SPR0);
  SPSR &= ~_BV(SPI2X);
}

void SPI_Speed_Fast()
{
  SPCR =  _BV(SPE) | _BV(MSTR);
  SPSR |= _BV(SPI2X);
}

void SPI_Send( BYTE data )
{
  SPDR = ( data ); 
  while( !( SPSR & ( 1<<SPIF ) ) );
}

BYTE SPI_Recv( void )
{
  SPDR = 0xFF; 
  while( !( SPSR & ( 1<<SPIF ) ) );  
  return SPDR;  
}

