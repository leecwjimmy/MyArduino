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
#include <Button2.h>        //Button2函式庫
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
uint16_t loopInterval = 10;  // main loop 10ms
float V =0.00;
float CV1 =0.00;
float CV2 =0.00;
float CV3 =0.00;
float CV4 =0.00;
float cr =0.00;
float duc = 0.00;
int counter = 0;
#define TBASE 10
#define T20MS 20/TBASE  //4
#define T30MS 30/TBASE  //6
#define T100MS 100/TBASE
#define T150MS 150/TBASE
#define T200MS 200/TBASE
#define T250MS 250/TBASE
#define T300MS 300/TBASE  //60
#define T500MS 500/TBASE
#define T1000MS 1000/TBASE
#define T2000MS 2000/TBASE
int KEY_CHEN;
int KEY_REP;
bool Blink1HZ;
bool Blink2HZ;
bool TMAIN;
bool F_CMDON;
 
bool F_KEYREP;

int T_CNT1;
int SQN = 0;
int TB_100MS;
int TB_250MS;
int TB_500MS;
int TB_1SEC;
int TB_1MINS;
int TB_1HOURS;
int T_Blink_Count;
int KEY_NEW = 0xff;   		// = 0xFF;
int KEY_OLD = 0xff;   		// = 0xFF;
int KEY_CMD;   		// = 0xFF;
int KEY_CHT;        	// = T20MS;
int KEY_SQN  = 0;        	// = 1;
int READ_SQN = 0;
int MODE = 0;

#define BUTTON_A_PIN  0  //按鍵A，PIN 0
#define BUTTON_B_PIN  35 //按鍵B，PIN 35
Button2 buttonA = Button2(BUTTON_A_PIN);
Button2 buttonB = Button2(BUTTON_B_PIN);




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
  
	buttonA.setPressedHandler(press);   //建立A按鍵按下Pressed的事件
//	buttonA.setReleasedHandler(release);//建立A按鍵放開Released的事件
	buttonB.setPressedHandler(press);   //建立B按鍵按下Pressed的事件
//	buttonB.setReleasedHandler(release);//建立B按鍵放開Released的事件  
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


void t100ms_process()
{
  if (TB_500MS ==0)
  {
    TB_500MS = 5;
    //0.5sec要處理的程式放這裡
 //     ledState ^= 1;
      Blink1HZ ^=1;  // Blink1HZ與1 互斥或  【反向的意思】
//	  bittest();
  } else {
           
           TB_500MS--;
            
  }
//	Print_VR_VAL();
  
  
}

//***************************
//***** 系統時間
//***************************
void check_time()
{
  if (TB_100MS ==0)
  {
    TB_100MS = T100MS;
    t100ms_process();    
  } else {
    
            TB_100MS--;
          }
    
  if (TB_250MS ==0)
  {
      TB_250MS = T250MS;
      Blink2HZ ^=1;  // Blink2HZ與1 互斥或  【反向的意思】
      
    } else{
        TB_250MS--;
      }
  if (T_Blink_Count ==0)
  {
      T_Blink_Count = 50; //100;    
//      Blink1HZ ^=1;	// Blink1HZ與1 互斥或  【反向的意思】
//      Blink2HZ ^=1;	// Blink2HZ與1 互斥或  【反向的意思】
      
    } else{
        T_Blink_Count--;
        if (T_Blink_Count == 25)  //50) 
        {
//                      Blink2HZ ^=1;
       
        }
    
    }
      
  if (T_CNT1 !=0)
   T_CNT1--;
   
//   if (KEY_CHT !=0)
//   KEY_CHT--;
   
//   if (KEY_REP !=0)
//   KEY_REP--;
   


}

void Dsp1_000(void)

{
  tft.fillScreen(TFT_BLACK);
  SQN = 1;
//  T_CNT1 = T500MS;
}

void  Dsp1_100(void)
{
	if (T_CNT1 !=0)
		return;
	
  u8f.setFontMode(0);                 // use u8g2 none transparent mode
  u8f.setFontDirection(0);            // left to right (this is default)
  u8f.setForegroundColor(TFT_WHITE);  // apply color
  u8f.setFont(u8g2_font_unifont_t_chinese1);  // use chinese2 for all the glyphs of "你好世界"
  u8f.setCursor(0, 15);
  u8f.print("亞弘電科技生技課 V5 TFT李進衛");
  BQ4050.readReg( registers.relativeStateOfCharge );
  u8f.setCursor(0, 35);
  u8f.print("剩餘");		//
//	u8g2_1.setFont(u8g2_font_courB08_tf); //(u8g2_font_amstrad_cpc_extended_8f);  //(u8g2_font_courB08_tf);  //(u8g2_font_victoriabold8_8u);  //(u8g2_font_fub20_tf); //(u8g2_font_ncenB14_tr); // choose a suitable font
	u8f.print(":");   // 
	u8f.print(registers.relativeStateOfCharge.val);   // 
	u8f.print("%");   //
  pa =  registers.relativeStateOfCharge.val;
  if ( pa < 28)
  {
		if (Blink2HZ ==1) {
		u8f.setForegroundColor(TFT_RED);  // apply color
        u8f.print(" 請充電");   //
		} else{
		u8f.setForegroundColor(TFT_BLACK);  // apply color
        u8f.print(" 請充電");   //
		}
    } 
    else if ( pa >30){
		
		if (Blink2HZ ==1)
		{	
			u8f.setForegroundColor(TFT_RED);  // apply color
			u8f.print(" 請放電");   //
		}else {
				u8f.setForegroundColor(TFT_BLACK);  // apply color
				u8f.print(" 請放電");   //
			}
      }else{
		u8f.setForegroundColor(TFT_GREEN);  // apply color
          u8f.print(" 良好");  } //
  BQ4050.readReg( registers.voltage );  
  u8f.setForegroundColor(TFT_WHITE);  // apply color
//  u8g2.setFont(u8g2_font_unifont_t_chinese1);  // use chinese2 for all the glyphs of "你好世界"
    u8f.setCursor(0, 55);
    u8f.print("電壓");		//  
//	u8g2_1.setFont(u8g2_font_courB08_tf); //(u8g2_font_amstrad_cpc_extended_8f);  //(u8g2_font_courB08_tf);  //(u8g2_font_victoriabold8_8u);  //(u8g2_font_fub20_tf); //(u8g2_font_ncenB14_tr); // choose a suitable font
	  u8f.print(":");   // 
    V = registers.voltage.val;
    V /= 1000;
	  u8f.print(V);   // 
	  u8f.print("V");   // 
 //   u8f.sendBuffer();	
 BQ4050.readReg( registers.current );
  u8f.setCursor(0, 75);
  u8f.print("電流:");
   u8f.print(registers.current.val);
//  cr = registers.current.val;
  u8f.print("mA");
  T_CNT1 = T100MS;
	
	
	
	
}


void Dsp2_000(void)
{
  tft.fillScreen(TFT_BLACK);
  SQN = 1;
	
	
}

void Dsp2_100(void)
{
	if (T_CNT1 !=0)
		return;
  u8f.setFont(u8g2_font_unifont_t_chinese1);  // use chinese2 for all the glyphs of "你好世界"
  u8f.setFontDirection(0);	
  u8f.setForegroundColor(TFT_WHITE);  // apply color
  u8f.setCursor(0, 15);
  u8f.print("亞弘電科技生技課 V5 TFT李進衛");
//   u8f.print(registers.current.val);
//  cr = registers.current.val;
//  u8f.print("mA");
//***********************************************************************************************
	BQ4050.readReg( registers.cellVoltage1 );

  u8f.setCursor(0, 35);
  u8f.print("核心一:");		//
  CV1 =  registers.cellVoltage1.val;
  CV1 /= 1000;
  u8f.print(CV1);		//
  u8f.print("V");		//
    BQ4050.readReg( registers.cellVoltage2 );
  
  u8f.print("核心二:");		//
  CV2 =  registers.cellVoltage2.val;
  CV2 /= 1000;
  u8f.print(CV2);		//
  u8f.print("V");		//
    BQ4050.readReg( registers.cellVoltage3 );

  u8f.setCursor(0, 60);
  u8f.print("核心三:");		//
  CV3 =  registers.cellVoltage3.val;
  CV3 /= 1000;
  u8f.print(CV3);		//
  u8f.print("V");		//
    BQ4050.readReg( registers.cellVoltage4 );
  
  u8f.print("核心四:");		//
  CV4 =  registers.cellVoltage4.val;
  CV4 /= 1000;
  u8f.print(CV4);		//
  u8f.print("V");		//
		T_CNT1 = T100MS;
		
}




//***************************************
// 顯示第一個資料
//***************************************
void Display1_proce(void)
{
	switch(SQN)
	{
		case 0: Dsp1_000(); break;
		case 1: Dsp1_100(); break;
//		case 2: Dsp1_200(); break;
//		case 3: Dsp1_300(); break;
//		case 4: sel4(); break;
//		case 5: sel5(); break;
//		case 6: sel6(); break;
//		case 7: sel7(); break;
	}
	
	
	
	
	
}


//***************************************
// 顯示第二個資料
//***************************************
void Display2_proce(void)
{
	switch(SQN)
	{
		case 0: Dsp2_000(); break;
		case 1: Dsp2_100(); break;
//		case 2: Dsp1_200(); break;
//		case 3: Dsp1_300(); break;
//		case 4: sel4(); break;
//		case 5: sel5(); break;
//		case 6: sel6(); break;
//		case 7: sel7(); break;
	}
	
}


void  Lcd_display(void)
{
	
//  u8f.clearBuffer();
//  u8f,clear();
  tft.fillScreen(TFT_BLACK);
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
    u8f.setCursor(0, 55);
    u8f.print("電壓");		//  
//	u8g2_1.setFont(u8g2_font_courB08_tf); //(u8g2_font_amstrad_cpc_extended_8f);  //(u8g2_font_courB08_tf);  //(u8g2_font_victoriabold8_8u);  //(u8g2_font_fub20_tf); //(u8g2_font_ncenB14_tr); // choose a suitable font
	  u8f.print(":");   // 
    V = registers.voltage.val;
    V /= 1000;
	  u8f.print(V);   // 
	  u8f.print("V");   // 
 //   u8f.sendBuffer();	
  u8f.setCursor(0, 75);
  u8f.print("電流:");
   u8f.print(registers.current.val);
//  cr = registers.current.val;
  u8f.print("mA");
  
  
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
  u8f.print("亞弘電科技生技課");
//   u8f.print(registers.current.val);
//  cr = registers.current.val;
//  u8f.print("mA");
//***********************************************************************************************
	BQ4050.readReg( registers.cellVoltage1 );

  u8f.setCursor(0, 35);
  u8f.print("核心一:");		//
  CV1 =  registers.cellVoltage1.val;
  CV1 /= 1000;
  u8f.print(CV1);		//
  u8f.print("V");		//
    BQ4050.readReg( registers.cellVoltage2 );
  
  u8f.print("核心二:");		//
  CV2 =  registers.cellVoltage2.val;
  CV2 /= 1000;
  u8f.print(CV2);		//
  u8f.print("V");		//
    BQ4050.readReg( registers.cellVoltage3 );

  u8f.setCursor(0, 60);
  u8f.print("核心三:");		//
  CV3 =  registers.cellVoltage3.val;
  CV3 /= 1000;
  u8f.print(CV3);		//
  u8f.print("V");		//
    BQ4050.readReg( registers.cellVoltage4 );
  
  u8f.print("核心四:");		//
  CV4 =  registers.cellVoltage4.val;
  CV4 /= 1000;
  u8f.print(CV4);		//
  u8f.print("V");		//
//    u8g2.sendBuffer();  







}

//******************************************
//***** 串列傳輸資料
//******************************************


void Ser_txt(void)
{
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
	
	
}
void displaychange(void)
{
    counter +=1;
    Serial.println(counter);
    // Lorro_BQ4050::Regt registers;

	
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
	
}

void press(Button2& btn) {
    if (btn == buttonA) {   //按下A按鍵
      
	  MODE = 0;
	  SQN = 0;
	  
    } else if (btn == buttonB) {  //按下B按鍵
		MODE = 1;
		SQN  = 0;

    }
}

void loop() {
  buttonA.loop();  //重複按鍵的觸發設定
  buttonB.loop();
  uint32_t currentMillis = millis();

  if( currentMillis - previousMillis > loopInterval ){
    previousMillis = currentMillis;
         
		check_time();
		
		switch(MODE)
		{
			case 0:	Display1_proce(); break;
			case 1: Display2_proce(); break;
		}

  }

}
