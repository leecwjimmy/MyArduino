/**
  Example of how to read battery fuel percentage, total voltage, cell voltages and current
  from TI BQ4050 fuel gauge chip.
  Fuel percentage reading will not be correct unless the battery has been calibrated for this chip
  2002-09-02 Arduino UNO 記憶體不足無法使用本程式
  2022-09-02 改為使用Wemos D1 ESP8266 開發板
  2022-09-02 建立繁體中文字庫以及焊接12864 LCD 
  2022-09-03 撰寫中文字庫
  2022-09-04 debug
  2022-09-06 改為12863 LCD 增加剩餘電量判斷式
  2022-09-07 增加顯示core 電壓
  CV4 筆誤成CV2 修正回來 2022-09-07 by leecw
**/

//Libraries to be included

#include <Wire.h>
#include "Arduino.h"
#include <Lorro_BQ4050.h>
//#include <LiquidCrystal_I2C.h>  // I2C LCD 使用的函數 使用LiquidCrystal目錄之下的LiquidCrystal_I2C.h
//#include <U8g2lib.h>
#include <SPI.h>
#include "TFT_eSPI.h"
#include "U8g2_for_TFT_eSPI.h"

TFT_eSPI tft = TFT_eSPI();   // tft instance
U8g2_for_TFT_eSPI u8f;       // U8g2 font instance
//Default address for device. Note, it is without read/write bit. When read with analyser,
//this will appear 1 bit shifted to the left
#define BQ4050addr     0x0B
//Initialise the device and library
Lorro_BQ4050 BQ4050( BQ4050addr );
//Instantiate the structs
extern Lorro_BQ4050::Regt registers;
uint32_t pa;
uint32_t previousMillis;
uint16_t loopInterval = 1000;
float V =0.00;
float CV1 =0.00;
float CV2 =0.00;
float CV3 =0.00;
float CV4 =0.00;
float cr =0.00;
float duc = 0.00;
int counter = 0;
//LiquidCrystal_I2C lcd(0x27, 16, 2);  // set the LCD address to 0x27 for a 16 chars and 2 line display
//U8G2_ST7920_128X64_F_HW_SPI u8g2(U8G2_R0, /* CS=*/ 15, /* reset=*/ 16); // Feather HUZZAH ESP8266, E=clock=14, RW=data=13, RS=CS
//U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);   // All Boards without Reset of the Display


void setup() {

  Serial.begin(115200);
  Serial.println("bq4050_20220907_V2.ino Version:01 YH PE !!");
//  lcd.init();  // initialize the lcd
//  lcd.backlight();
//  u8g2.begin();
//  u8g2.enableUTF8Print();		// enable UTF8 support for the Arduino print() function
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  u8f.begin(tft);                     // connect u8g2 procedures to TFT_eSPI
}

//*************************************************************
/*
	u8g2_1.clearBuffer();					// clear the internal memory
	u8g2_1.setFont(u8g2_font_amstrad_cpc_extended_8f);  //(u8g2_font_courB08_tf);  //(u8g2_font_victoriabold8_8u);  //(u8g2_font_fub20_tf); //(u8g2_font_ncenB14_tr); // choose a suitable font
	u8g2_1.setCursor(0,9);
	u8g2_1.print("Max temp:");   // 
	u8g2_1.print(TEMP_F_VAL);   // 
	u8g2_1.print("F");
	u8g2_1.setFont(u8g2_font_fub35_tr);  //(u8g2_font_fub20_tf); //(u8g2_font_ncenB14_tr);	// choose a suitable font
////	u8g2_1.setCursor(10,54);
	u8g2_1.setCursor(15,60);
	u8g2_1.print(mlx.readObjectTempF(),0);	//(mlx.readAmbientTempF(),0);   // 2019-08-05 發現bug 溫度sensor 反應速度慢
	u8g2_1.print("F");   //
	u8g2_1.sendBuffer();					// transfer internal memory to the display
	
	
  u8g2.setFont(u8g2_font_unifont_t_chinese1);  // use chinese2 for all the glyphs of "你好世界"
  u8g2.setFontDirection(0);
  u8g2.clearBuffer();
  u8g2.setCursor(0, 15);
  u8g2.print("亞弘電科技生技課");
  u8g2.setCursor(0, 40);
  u8g2.print("李進衛");		// Chinese "Hello World" 
  u8g2.print("繼學民");    // Chinese "Hello World" 
  
  u8g2.setCursor(0, 60);
  u8g2.print("電壓：   電流：  ");    // Chinese "Hello World" 
  
  u8g2.sendBuffer();
	
	
	
*/




void  Lcd_display(void)
{
	
//  u8f.clearBuffer();
//  u8f,clear();
//  tft.fillScreen(TFT_BLACK);
  u8f.setFontMode(0);                 // use u8g2 none transparent mode
  u8f.setFontDirection(0);            // left to right (this is default)
  u8f.setForegroundColor(TFT_WHITE);  // apply color
  u8f.setFont(u8g2_font_unifont_t_chinese1);  // use chinese2 for all the glyphs of "你好世界"
  u8f.setCursor(0, 15);
  u8f.print("亞弘電科技生技課 V4");
  u8f.setCursor(0, 35);
  u8f.print("剩餘");		//
//	u8g2_1.setFont(u8g2_font_courB08_tf); //(u8g2_font_amstrad_cpc_extended_8f);  //(u8g2_font_courB08_tf);  //(u8g2_font_victoriabold8_8u);  //(u8g2_font_fub20_tf); //(u8g2_font_ncenB14_tr); // choose a suitable font
	u8f.print(":");   // 
	u8f.print(registers.relativeStateOfCharge.val);   // 
	u8f.print("%");   //
  pa =  registers.relativeStateOfCharge.val;
  if ( pa < 28)
  {
		u8f.setForegroundColor(TFT_RED);  // apply color
        u8f.print(" 請充電");   //
    
    } 
    else if ( pa >30){
		u8f.setForegroundColor(TFT_RED);  // apply color
        u8f.print(" 請放電");   //
    
      }else{
		u8f.setForegroundColor(TFT_GREEN);  // apply color
          u8f.print(" 良好");  } //
    
  u8f.setForegroundColor(TFT_WHITE);  // apply color
//  u8g2.setFont(u8g2_font_unifont_t_chinese1);  // use chinese2 for all the glyphs of "你好世界"
    u8f.setCursor(0, 60);
    u8f.print("電壓");		//  
//	u8g2_1.setFont(u8g2_font_courB08_tf); //(u8g2_font_amstrad_cpc_extended_8f);  //(u8g2_font_courB08_tf);  //(u8g2_font_victoriabold8_8u);  //(u8g2_font_fub20_tf); //(u8g2_font_ncenB14_tr); // choose a suitable font
	  u8f.print(":");   // 
    V = registers.voltage.val;
    V /= 1000;
	  u8f.print(V);   // 
	  u8f.print("V");   // 
 //   u8f.sendBuffer();	
  
  
/*	
    lcd.setCursor(0, 0);  //設定游標在第1列第0行
    lcd.print("Charge:");
    lcd.setCursor(7, 0);  //設定游標在第1列第0行
    lcd.print(registers.relativeStateOfCharge.val);
    lcd.setCursor(9, 0);
    lcd.print("%");
    lcd.setCursor(0, 1);  //設定游標在第2列第0行
    lcd.print("Voltage:");
    lcd.setCursor(8, 1);  //設定游標在第2列第0行
    V = registers.voltage.val;
    V /= 1000;
    lcd.print(V);
    lcd.setCursor(13, 1);
    lcd.print("V");    
*/

}

void Lcd_display1(void)
{
//  u8f.clearBuffer();
//  u8f.clear();
//  tft.fillScreen(TFT_BLACK);
  u8f.setFont(u8g2_font_unifont_t_chinese1);  // use chinese2 for all the glyphs of "你好世界"
  u8f.setFontDirection(0);	
  u8f.setForegroundColor(TFT_WHITE);  // apply color
  u8f.setCursor(0, 15);
  u8f.print("電流:");
   u8f.print(registers.current.val);
//  cr = registers.current.val;
  u8f.print("mA");
//***********************************************************************************************
  u8f.setCursor(0, 35);
  u8f.print("核心一:");		//
  CV1 =  registers.cellVoltage1.val;
  CV1 /= 1000;
  u8f.print(CV1);		//
  u8f.print("V");		//
  u8f.print("核心二:");		//
  CV2 =  registers.cellVoltage2.val;
  CV2 /= 1000;
  u8f.print(CV2);		//
  u8f.print("V");		//

  u8f.setCursor(0, 60);
  u8f.print("核心三:");		//
  CV3 =  registers.cellVoltage3.val;
  CV3 /= 1000;
  u8f.print(CV3);		//
  u8f.print("V");		//
  u8f.print("核心四:");		//
  CV4 =  registers.cellVoltage4.val;
  CV4 /= 1000;
  u8f.print(CV4);		//
  u8f.print("V");		//
//    u8g2.sendBuffer();  







}

void loop() {

  uint32_t currentMillis = millis();

  if( currentMillis - previousMillis > loopInterval ){
    previousMillis = currentMillis;
    counter +=1;
    Serial.println(counter);
    // Lorro_BQ4050::Regt registers;
    BQ4050.readReg( registers.relativeStateOfCharge );
    Serial.print( "State of charge: " );
    Serial.print( registers.relativeStateOfCharge.val );
    Serial.println( "%" );

    delay( 15 );

    BQ4050.readReg( registers.voltage );
    Serial.print( "Pack voltage: " );
    Serial.print( registers.voltage.val );
    Serial.println( "mV" );
    delay( 15 );

    BQ4050.readReg( registers.cellVoltage1 );
    Serial.print( "Cell voltage 1: " );
    Serial.print( registers.cellVoltage1.val );
    Serial.println( "mV" );
    delay( 15 );

    BQ4050.readReg( registers.cellVoltage2 );
    Serial.print( "Cell voltage 2: " );
    Serial.print( registers.cellVoltage2.val );
    Serial.println( "mV" );
    delay( 15 );

    BQ4050.readReg( registers.cellVoltage3 );
    Serial.print( "Cell voltage 3: " );
    Serial.print( registers.cellVoltage3.val );
    Serial.println( "mV" );
    delay( 15 );

    BQ4050.readReg( registers.cellVoltage4 );
    Serial.print( "Cell voltage 4: " );
    Serial.print( registers.cellVoltage4.val );
    Serial.println( "mV" );
    delay( 15 );

    BQ4050.readReg( registers.current );
    Serial.print( "Current: " );
    Serial.print( registers.current.val );
    Serial.println( "mA" );
    delay( 15 );
// read temperature 
// 	C= K-273.15
// leechin wei 2022-09-08 append 
    BQ4050.readReg( registers.temperature );
	duc = registers.temperature.val;
	duc /=10;
	duc = duc-273.15;
    Serial.print( "Temperature: " );
    Serial.print( duc );
    Serial.println( "C" );
    delay( 15 );
//**********************************	
// averageCurrent
// 2022-09-08 leecw append
//**********************************
    BQ4050.readReg( registers.averageCurrent );
    Serial.print( "averageCurrent " );
    Serial.print( registers.averageCurrent.val);
    Serial.println( "mA" );
    delay( 15 );

	
    if (counter <=5)  //顯示資料5秒變化一次
    {
      if (counter ==5)
      {
          tft.fillScreen(TFT_BLACK);

        }
    Lcd_display(); //顯示logo 電量以及電壓
    Serial.print(" <5");
    Serial.println(counter);
    } else if ((counter>5)&&(counter <=10))
    {
     if (counter ==10)
      {
          tft.fillScreen(TFT_BLACK);

        }
     Serial.print(" >5 <=10 ");
    Serial.println(counter);
      
        Lcd_display1(); //顯示每個電池電壓以及充電電流

    } else if (counter > 10) {
        
        counter = 0;
          tft.fillScreen(TFT_BLACK);

    }
          //    delay(1000);

  }

}
