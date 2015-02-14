#ifndef _CONFIG_H
#define _CONFIG_H

// LCD Definitions
// I2C config and expander data lines
#define LCD_I2C_ADDR        0x27 // I2C address for the LCD 
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
#define SENSE_PORT          PORTD
#define SENSE_DDR           DDRD
#define SENSE_PIN           5
#define SENSE_ON()          SENSE_PORT &= ~_BV(SENSE_PIN)
#define SENSE_OFF()         SENSE_PORT |=  _BV(SENSE_PIN)

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

 // uncomment this line if you are using HW2.0
//#define KEYS_INPUT_PULLUP
#define KEYS_READ_PORT      PORTC
#define KEYS_READ_DDR       DDRC
#define KEYS_READ_PINS      PINC
#define KEY_SELECT_PIN      3
#define KEY_ABORT_PIN       2
#define KEY_PREV_PIN        1
#define KEY_NEXT_PIN        0

#endif
