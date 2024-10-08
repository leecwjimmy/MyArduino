/*
  日期：2022-09-13
  程式設計：李進衛
  版本：V0
  開發板： 李進衛

  Demonstrates how to use U8g2_for_TFT_eSPI library.

  U8g2_for_TFT_eSPI:
    - Use U8g2 fonts with TFT_eSPI
    - Supports UTF-8 in print statement
    - 90, 180 and 270 degree text direction
  
  List of all U8g2 fonts:    https://github.com/olikraus/u8g2/wiki/fntlistall

  TFT_eSPI library:          https://github.com/Bodmer/TFT_eSPI
  U8g2_for_TFT_eSPI library: https://github.com/Bodmer/U8g2_for_TFT_eSPI
	開發板選擇「ESP32 Dev Module」
	這個測試程式開發板是使用TTGO 含TFT的開發板在Arduino IDE 的板子設定要設定為「ESP32 Dev Module」
	中文化的部分比較麻煩：
	1. 在D:\onedrive\文件\Arduino\libraries\U8g2_for_TFT_eSPI-master.zip\U8g2_for_TFT_eSPI-master\src 找到u8g2_fonts.c 這個檔案使用編輯軟體打開search chinese1 這個區塊的
	   資料替換為原先在OLED/LCD 的資料
	2.官方建議LCD是用TFT-eSPI這個強大的函式庫來驅動，安裝好函式庫後，要做一點點小設定，進到「\Documents\Arduino\libraries\TFT_eSPI」目錄，打開「User_Setup_Select.h」檔案。

		「#include <User_Setup.h>」前面加上//，把這行註解掉。

		「#include <User_Setups/Setup25_TTGO_T_Display.h>」這行就要取消註解，表示我們要用TTGO T-Display所搭配的LCD。 


	
*/
#include "SPI.h"
#include "TFT_eSPI.h"
//#include <U8g2lib.h>
#include "U8g2_for_TFT_eSPI.h"
int flag = 0;
TFT_eSPI tft = TFT_eSPI();   // tft instance
U8g2_for_TFT_eSPI u8f;       // U8g2 font instance

void setup() {
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  u8f.begin(tft);                     // connect u8g2 procedures to TFT_eSPI
}

unsigned long x = 0;

void loop() {
  u8f.setFontMode(0);                 // use u8g2 none transparent mode
  u8f.setFontDirection(0);            // left to right (this is default)
  u8f.setForegroundColor(TFT_WHITE);  // apply color
  //***********************************************************************************************
  u8f.setFont(u8g2_font_unifont_t_chinese1);  // use chinese2 for all the glyphs of "你好世界"
//  u8g2.setFontDirection(0);	
//  u8g2.clearBuffer();
//  u8g2.setCursor(0, 15);
//  u8g2.print("亞弘電科技生技課");

//  u8f.setFont(u8g2_font_helvR14_tf);  // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
  u8f.setForegroundColor(TFT_WHITE);  // apply color
  u8f.setCursor(0,20);                // start writing at this position
  u8f.print("亞弘電科技生技課 李進衛");
  u8f.setForegroundColor(TFT_RED);  // apply color
  u8f.setCursor(0,40);                // start writing at this position
  u8f.print("不良!!");            // UTF-8 string with german umlaut chars

//  u8f.setFont(u8g2_font_inb63_mn);    // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
  u8f.setFontMode(0);                 // use u8g2 none transparent mode

    


  while (1) {
      if (flag ==0)
      {
  u8f.setForegroundColor(TFT_BLACK);  // apply color
        u8f.setCursor(0,40);                // start writing at this position
        u8f.print("不良!!");            // UTF-8 string with german umlaut chars
        flag = 1;
        } else{
  u8f.setForegroundColor(TFT_RED);  // apply color
        u8f.setCursor(0,40);                // start writing at this position
        u8f.print("不良!!");            // UTF-8 string with german umlaut chars
        flag = 0;

          
        }
  u8f.setForegroundColor(TFT_GREEN);  // apply color
        u8f.setCursor(0,60);                // start writing at this position
        u8f.print("良品!!");            // UTF-8 string with german umlaut chars
        
//    u8f.setCursor(0,110);             // start writing at this position
//    u8f.print(x);                     // numerical value
//    x++;
    delay(250);
  }
} 
