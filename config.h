
#ifndef _CONFIG_H
#define _CONFIG_H

#include "config-user.h"

// thanks stack overflow: http://stackoverflow.com/questions/4301471/c-macro-to-test-if-more-than-one-defined
#if defined(LCD_USE_SSD1306_OLED_MODULE) + defined(LCD_USE_1602_LCD_MODULE) + defined(LCD_USE_SSD131X_OLED_MODULE) != 1
  #error Ether no or multiple LCD types defined! Have you created your config-user.h file?
#endif

#if defined(LCD_USE_SSD1306_OLED_MODULE)
  #if defined(LCD_SSD1306_128x64) + defined(LCD_SSD1306_128x32) != 1
    #error When using the SSD1306 module, define EITHER: LCD_SSD1306_128x64 or LCD_SSD1306_128x32 depending on your module type
  #endif
#endif

#if defined(TAPUINO_LANGUAGE_EN) + defined(TAPUINO_LANGUAGE_ES) + defined(TAPUINO_LANGUAGE_IT) + defined(TAPUINO_LANGUAGE_TR) + defined(TAPUINO_LANGUAGE_DE) != 1
  #error Either no or multiple languages defined! Have you created your config-user.h file?
#endif


#define LCD_BIT_RS          0    // Register select
#define LCD_BIT_RW          1    // Read / Write
#define LCD_BIT_EN          2    // Enable
#define LCD_BIT_BACKLIGHT   3    // Backlight
#define LCD_BIT_DATA0       4    // 4 bit data, bit 0
#define LCD_BIT_DATA1       5    // 4 bit data, bit 1
#define LCD_BIT_DATA2       6    // 4 bit data, bit 2
#define LCD_BIT_DATA3       7    // 4 bit data, bit 3
// LCD dimensions config
#define LCD_NUM_LINES       2    // number of display lines on the LCD
#define MAX_LCD_LINE_LEN    16   // max number of characters on a line

// Timing constant defaults, these will only apply until eeprom values are saved
#define TICKER_RATE         250  // milliseconds, granularity 10ms
#define TICKER_HOLD         1250 // milliseconds, ticker begin and end hold time, granularity 10ms
#define KEY_REPEAT_START    500  // milliseconds, granularity 10ms
#define KEY_REPEAT_NEXT     300  // milliseconds, granularity 10ms
#define REC_FINALIZE_TIME   2000 // milliseconds, granularity 10ms

// TWI for pullups
#define TWI_PORT            PORTC
#define TWI_PIN_SDA         4
#define TWI_PIN_SCL         5

// port definitions, change for different wiring
#define TAPE_READ_PORT      PORTD
#define TAPE_READ_DDR       DDRD
#define TAPE_READ_PIN       3
#define TAPE_READ_PINS      PIND
#define TAPE_READ_LOW()     TAPE_READ_PORT &= ~_BV(TAPE_READ_PIN)
#define TAPE_READ_HIGH()    TAPE_READ_PORT |=  _BV(TAPE_READ_PIN)

#define TAPE_WRITE_PORT     PORTB
#define TAPE_WRITE_DDR      DDRB
#define TAPE_WRITE_PINS     PINB
#define TAPE_WRITE_PIN      0

/* The Sense pin on the original Datassette is a physical switch that only grounds the line when pressed, it's left
 * floating when no keys are pressed. This means that the pin should be driven in an open-collector fashion, otherwise
 * it can create problems, in particular on the Plus/4 where the Sense line is shared with data pin 2 of the User Port.
 *
 * This could be compensated in hardware (put a diode in series, cathode to Arduino side) or in software, by taking
 * advantage of the way ATmega pins are configured, which is what we do here:
 * - To set sense "high" we set the pin to input mode, pin will go Hi-Z and basically float
 * - To set it low, we switch to output mode which will implicitly be low (PORTD defaults to 0 and we never touch it)
 */
#define SENSE_PORT          PORTD
#define SENSE_DDR           DDRD
#define SENSE_PIN           5
#define SENSE_ON()          SENSE_DDR |= _BV(SENSE_PIN)
#define SENSE_OFF()         SENSE_DDR &= ~_BV(SENSE_PIN)

#define MOTOR_PORT          PORTD
#define MOTOR_DDR           DDRD
#define MOTOR_PIN           4
#define MOTOR_PINS          PIND
#define MOTOR_IS_OFF()      (MOTOR_PINS & _BV(MOTOR_PIN))

#define CONTROL_PORT        PORTD
#define CONTROL_DDR         DDRD
#define CONTROL_PIN0        6
#define CONTROL_PIN1        7
#define CONTROL_SET_BUS0()  CONTROL_PORT &= ~(_BV(CONTROL_PIN0) | _BV(CONTROL_PIN1))
#define CONTROL_SET_BUS1()  { CONTROL_PORT &= ~_BV(CONTROL_PIN1); CONTROL_PORT |= _BV(CONTROL_PIN0); }

#define REC_LED_PORT        PORTD
#define REC_LED_DDR         DDRD
#define REC_LED_PIN         2
#define REC_LED_OFF()       REC_LED_PORT |= _BV(REC_LED_PIN)
#define REC_LED_ON()        REC_LED_PORT &= ~_BV(REC_LED_PIN)

 // comment this line if you are using HW1.0
#define KEYS_INPUT_PULLUP
#define KEYS_READ_PORT      PORTC
#define KEYS_READ_DDR       DDRC
#define KEYS_READ_PINS      PINC
#define KEY_SELECT_PIN      3
#define KEY_ABORT_PIN       2
#define KEY_PREV_PIN        1
#define KEY_NEXT_PIN        0

// debugging
//#define ENABLE_SERIAL

#endif
