#include "config.h"
#ifdef LCD_USE_SSD131X_OLED_MODULE

#include <util/delay.h>
#include "i2c_master.h"

#define MODE_OLED_COMMAND 0x80
#define MODE_OLED_DATA 0x40

uint8_t _addr;

void sendData(unsigned char data);
void sendCommand(unsigned char command);
void send_string(const char *String);


void lcd_init(uint8_t lcd_addr) {
  _addr = lcd_addr;
  i2c_init();
  
 // *** I2C initial *** //
 _delay_ms(100);
 sendCommand(0x2A);	// **** Set "RE"=1	00101010B
 sendCommand(0x71);
 sendCommand(0x5C);
 sendCommand(0x28);

 sendCommand(0x08);	// **** Set Sleep Mode On
 sendCommand(0x2A);	// **** Set "RE"=1	00101010B
 sendCommand(0x79);	// **** Set "SD"=1	01111001B

 sendCommand(0xD5);
 sendCommand(0x70);
 sendCommand(0x78);	// **** Set "SD"=0

 sendCommand(0x08);	// **** Set 5-dot, 3 or 4 line(0x09), 1 or 2 line(0x08)

 sendCommand(0x06);	// **** Set Com31-->Com0  Seg0-->Seg99

 // **** Set OLED Characterization *** //
 sendCommand(0x2A);  	// **** Set "RE"=1 
 sendCommand(0x79);  	// **** Set "SD"=1

 // **** CGROM/CGRAM Management *** //
 sendCommand(0x72);  	// **** Set ROM
 sendCommand(0x00);  	// **** Set ROM A and 8 CGRAM


 sendCommand(0xDA); 	// **** Set Seg Pins HW Config
 sendCommand(0x10);   

 sendCommand(0x81);  	// **** Set Contrast
 sendCommand(0xFF); 

 sendCommand(0xDB);  	// **** Set VCOM deselect level
 sendCommand(0x30);  	// **** VCC x 0.83

 sendCommand(0xDC);  	// **** Set gpio - turn EN for 15V generator on.
 sendCommand(0x03);

 sendCommand(0x78);  	// **** Exiting Set OLED Characterization
 sendCommand(0x28); 
 sendCommand(0x2A); 
 //sendCommand(0x05); 	// **** Set Entry Mode
 sendCommand(0x06); 	// **** Set Entry Mode
 sendCommand(0x08);  
 sendCommand(0x28); 	// **** Set "IS"=0 , "RE" =0 //28
 sendCommand(0x01); 
 sendCommand(0x80); 	// **** Set DDRAM Address to 0x80 (line 1 start)

 _delay_ms(100);
 sendCommand(0x0C);  	// **** Turn on Display
}

void sendData(uint8_t data)
{
	i2c_start((_addr << 1) | I2C_WRITE);
  i2c_write(MODE_OLED_DATA);     		 // **** Set OLED Command mode
  i2c_write(data);
	i2c_stop();   
}

void sendCommand(uint8_t command)
{
	i2c_start((_addr << 1) | I2C_WRITE);
  i2c_write(MODE_OLED_COMMAND);     		 // **** Set OLED Command mode
  i2c_write(command);
	i2c_stop();   
  _delay_ms(10);
}


void lcd_cursor() {
  sendCommand(0x0E);  	// **** Turn on Display, cursor
}

void lcd_noCursor(){
  sendCommand(0x0C);  	// **** Turn on Display
}

void lcd_backlight(){
}

void lcd_noBacklight(){
}

void lcd_createChar(uint8_t location, uint8_t charmap[]){
}

void lcd_setCursor(uint8_t col, uint8_t row){
  int row_offsets[] = { 0x00, 0x40 };
  sendCommand(0x80 | (col + row_offsets[row]));  
}

void lcd_write(uint8_t value){
  sendData(value);
}

void lcd_print(char* msg) {
  uint8_t v;
  while((v = *msg++)) {
    sendData(v);
  }
  //sendCommand(0xC0);  	// **** New Line
}

#endif