#ifndef ARDUINO

#include <avr/interrupt.h>
#include "tapuino.h"

int main(void)
{
  sei();
  tapuino_run();
  while(1){
  }
}

#endif
