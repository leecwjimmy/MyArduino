/*
  Blink without Delay

  Turns on and off a light emitting diode (LED) connected to a digital pin,
  without using the delay() function. This means that other code can run at the
  same time without being interrupted by the LED code.

  The circuit:
  - Use the onboard LED.
  - Note: Most Arduinos have an on-board LED you can control. On the UNO, MEGA
    and ZERO it is attached to digital pin 13, on MKR1000 on pin 6. LED_BUILTIN
    is set to the correct LED pin independent of which board is used.
    If you want to know what pin the on-board LED is connected to on your
    Arduino model, check the Technical Specs of your board at:
    https://www.arduino.cc/en/Main/Products

  created 2005
  by David A. Mellis
  modified 8 Feb 2010
  by Paul Stoffregen
  modified 11 Nov 2013
  by Scott Fitzgerald
  modified 9 Jan 2017
  by Arturo Guadalupi

  This example code is in the public domain.

  https://www.arduino.cc/en/Tutorial/BuiltInExamples/BlinkWithoutDelay
2022/12/20 新增加OLED 程式


*/
#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
// constants won't change. Used here to set a pin number:
const int ledPin = LED_BUILTIN; // the number of the LED pin

// Variables will change:
int ledState = LOW; // ledState used to set the LED
#define P_VR A0
#define TBASE 5
float voltage = 0.000;
#define T250MS 250 / TBASE
int workCounter = T250MS;
int analogValue = 0;
int old_analogValue = 0;
int MIDI = 0;
String S_MIDI;
String S_VOLTAGE;
// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0; // will store last time LED was updated

// constants won't change:
const long interval = 5; // interval at which to blink (milliseconds)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);
// U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);
// U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);   // All Boards without Reset of the Display
void setup()
{

  Serial.begin(115200);
  Serial.println("VR20231220_OLED.ino");
  // set the digital pin as output:
  pinMode(ledPin, OUTPUT);
  pinMode(P_VR, INPUT);
  u8g2.begin();
  u8g2.enableUTF8Print(); // enable UTF8 support for the Arduino print() function
}

void loop()
{
  // here is where you'd put code that needs to be running all the time.

  // check to see if it's time to blink the LED; that is, if the difference
  // between the current time and last time you blinked the LED is bigger than
  // the interval at which you want to blink the LED.
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval)
  {
    // save the last time you blinked the LED
    previousMillis = currentMillis;

    analogValue = analogRead(P_VR);
    if (analogValue != old_analogValue)
    {
      voltage = analogValue;
      old_analogValue = analogValue;
      Serial.println("ADC:" + String(voltage));
      int shiftvalue = analogValue >> 3;    //右移3bit
      S_MIDI = "MIDI==>" + String(shiftvalue, HEX);
      Serial.println("MIDI==>" + S_MIDI);

      //  MIDI = map(analogValue,0,1024,0,127);
      voltage = (voltage * 3.3030) / 1024.00;
      S_VOLTAGE = String(voltage, 4) + "V";
      Serial.println("VR voltage:" + S_VOLTAGE + "V");

      u8g2.clearBuffer();              // clear the internal memory
      u8g2.setFont(u8g2_font_cu12_tr); // u8g2_font_unifont_t_chinese2); //u8g2_font_ncenB08_tr);	// choose a suitable font
      u8g2.setCursor(0, 15);
      u8g2.print(S_MIDI); // write something to the internal memory
      u8g2.setCursor(0, 40);
      u8g2.print(S_VOLTAGE); // write something to the internal memory

      u8g2.sendBuffer(); // transfer internal memory to the display
                         //  Serial.println("VR MIDI Value:" + String(MIDI));
                         //  MIDI = MAP(analoagValue,0,1024,0,127);
                         //    Serial.print(voltage);
                         //    Serial.println("V");
    }
    workCounter--;
    if (workCounter == 0)
    {
      workCounter = T250MS;
      // if the LED is off turn it on and vice-versa:
      if (ledState == LOW)
      {
        ledState = HIGH;
      }
      else
      {
        ledState = LOW;
      }
      digitalWrite(ledPin, ledState);
    }
    // set the LED with the ledState of the variable:
  }
}
