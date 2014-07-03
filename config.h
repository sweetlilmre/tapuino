#ifndef _CONFIG_H
#define _CONFIG_H

#define MAX_LCD_LINE_LEN    20
#define FAT_BUF_SIZE        256

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

#define MOTOR_PORT          PORTD
#define MOTOR_DDR           DDRD
#define MOTOR_PIN           4
#define MOTOR_PINS          PIND
#define MOTOR_IS_OFF()      (MOTOR_PINS & _BV(MOTOR_PIN))

#endif
