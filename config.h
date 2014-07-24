#ifndef _CONFIG_H
#define _CONFIG_H

#define SPINNER_RATE        12500
#define LCD_I2C_ADDR        0x27
#define LCD_NUM_LINES       2
#define MAX_LCD_LINE_LEN    16
#define TICKER_HOLD         5

// don't mess with this!
#define FAT_BUF_SIZE        256

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
#define TAPE_READ_TOGGLE()  TAPE_READ_PINS |=  _BV(TAPE_READ_PIN)

#define TAPE_WRITE_PORT     PORTD
#define TAPE_WRITE_DDR      DDRD
#define TAPE_WRITE_PIN      2

#define MOTOR_PORT          PORTD
#define MOTOR_DDR           DDRD
#define MOTOR_PIN           4
#define MOTOR_PINS          PIND
#define MOTOR_IS_OFF()      (MOTOR_PINS & _BV(MOTOR_PIN))

#define KEYS_READ_PORT      PORTC
#define KEYS_READ_DDR       DDRC
#define KEYS_READ_PINS      PINC
#define KEY_SELECT_PIN      3
#define KEY_ABORT_PIN       2
#define KEY_PREV_PIN        1
#define KEY_NEXT_PIN        0
#endif
