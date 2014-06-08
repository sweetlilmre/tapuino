#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

extern "C"
{
  #include "player.h"
  void responseCallback(char* msg)
  {
    lcd.home();
    lcd.print(msg);
  }
}

void setup()
{
  lcd.init();                      // initialize the lcd 
 
  // Print a message to the LCD.
  lcd.backlight();
  lcd.print("Init");
  player_run();
}

void loop()
{
}
