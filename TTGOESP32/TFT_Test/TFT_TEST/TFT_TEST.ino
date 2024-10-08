/*
*************************************************
測試結果每行字可以寫15個中文字
30個數字以及英文字
2022-09-21
李進衛
這是一個測試程式

*************************************************

*/

#include "Arduino.h"
#include <Wire.h>
#include "TFT_eSPI.h"
#include "U8g2_for_TFT_eSPI.h"
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI(); // tft instance
U8g2_for_TFT_eSPI u8f;     // U8g2 font instance

uint32_t pa;
int32_t curr;
int32_t avgcurr;

uint32_t previousMillis;
uint16_t loopInterval = 10; // main loop 10ms
float V = 0.00;
float CV1 = 0.00;
float CV2 = 0.00;
float CV3 = 0.00;
float CV4 = 0.00;
float cr = 0.00;
float duc = 0.00;
int counter = 0;
#define TBASE 10
#define T20MS 20 / TBASE // 4
#define T30MS 30 / TBASE // 6
#define T100MS 100 / TBASE
#define T150MS 150 / TBASE
#define T200MS 200 / TBASE
#define T250MS 250 / TBASE
#define T300MS 300 / TBASE // 60
#define T500MS 500 / TBASE
#define T1000MS 1000 / TBASE
#define T1SEC 1000 / TBASE
#define T2000MS 2000 / TBASE
int KEY_CHEN;
int KEY_REP;
bool Blink1HZ;
bool Blink2HZ;
bool TMAIN;
bool F_CMDON;

bool F_KEYREP;
bool F_changeColer1 = 0;
bool F_changeColer2 = 0;
int T_CNT1;
int T_CNT1SEC = 1000 / TBASE;
int SQN = 0;
int TB_100MS;
int TB_250MS;
int TB_500MS;
int TB_1SEC;
int TB_1MINS;
int TB_1HOURS;
int T_Blink_Count;
int KEY_NEW = 0xff; // = 0xFF;
int KEY_OLD = 0xff; // = 0xFF;
int KEY_CMD;        // = 0xFF;
int KEY_CHT;        // = T20MS;
int KEY_SQN = 0;    // = 1;
int READ_SQN = 0;
int MODE = 0;
//**************************************
//*** 2022-09-17 by Lee chin wei append
//**************************************
int Tss = 0;            //秒計時器
int Tmm = 0;            //分計時器
int Thh = 0;            //時計時器

void setup()
{

  Serial.begin(115200);
  Serial.println("TFT test by Lee chin wei 2022-09-21 V0 ");
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK); //clear All 
  u8f.begin(tft); // connect u8g2 procedures to TFT_eSPI
}

void t100ms_process()
{
  if (TB_500MS == 0)
  {
    TB_500MS = 5;
    // 0.5sec要處理的程式放這裡
    //     ledState ^= 1;
    Blink1HZ ^= 1; // Blink1HZ與1 互斥或  【反向的意思】
                   //	  bittest();
  }
  else
  {

    TB_500MS--;
  }
  //	Print_VR_VAL();
}

//***************************
//***** 系統時間
//***************************
void check_time()
{
  if (TB_100MS == 0)
  {
    TB_100MS = T100MS;
    t100ms_process();
  }
  else
  {

    TB_100MS--;
  }

  if (TB_250MS == 0)
  {
    TB_250MS = T250MS;
    Blink2HZ ^= 1; // Blink2HZ與1 互斥或  【反向的意思】
  }
  else
  {
    TB_250MS--;
  }
  if (T_Blink_Count == 0)
  {
    T_Blink_Count = 50; // 100;
    //      Blink1HZ ^=1;	// Blink1HZ與1 互斥或  【反向的意思】
    //      Blink2HZ ^=1;	// Blink2HZ與1 互斥或  【反向的意思】
  }
  else
  {
    T_Blink_Count--;
    if (T_Blink_Count == 25) // 50)
    {
      //                      Blink2HZ ^=1;
    }
  }

  if (T_CNT1 != 0)
    T_CNT1--;
  //******************************************
  //**** 2022-09-17 by Lee chin wei
  //**** 計算時間
  //******************************************
  if (T_CNT1SEC != 0)
  {
    T_CNT1SEC--;
  }
  else
  {
    T_CNT1SEC = T1000MS;
    Time_counter();
  }

  //   if (KEY_CHT !=0)
  //   KEY_CHT--;

  //   if (KEY_REP !=0)
  //   KEY_REP--;
}

void Time_counter(void)
{
  Tss++; // Advance second
  if (Tss == 60)
  {
    Tss = 0;
    Tmm++; // Advance minute
    if (Tmm > 59)
    {
      Tmm = 0;
      Thh++; // Advance hour
      if (Thh > 23)
      {
        Thh = 0;
      }
    }
  }
}


//*****************************************
//量測螢幕可以容下幾個字體
//*****************************************

void DisplayShow(void)
{
	if (T_CNT1 != 0)
		return;
	u8f.setFontMode(0);                        // use u8g2 none transparent mode
	u8f.setFontDirection(0);                   // left to right (this is default)
                                             //  u8f.setForegroundColor(TFT_WHITE);  // apply color
	u8f.setFont(u8g2_font_unifont_t_chinese1); // use chinese2 for all the glyphs of "你好世界"
    u8f.setForegroundColor(TFT_WHITE); // apply color
    u8f.setCursor(0, 15);
	u8f.print("亞弘電科技生技課李進衛電電電上"); //15個中文
	u8f.setCursor(0, 35);
	u8f.print("1234567890.12345"); //16個數字
	u8f.setCursor(0, 55);
	u8f.print("ABCDEFGHIJKLMNOP"); //16個數字
	u8f.setCursor(0, 75);
	u8f.print(Thh); //
	u8f.print(":"); //
	u8f.print(Tmm); //
	u8f.print(":"); //
	u8f.print(Tss); //
	
	
	
	

	T_CNT1 = T250MS;
	
	
	
	
}


void loop()
{
  uint32_t currentMillis = millis();

  if (currentMillis - previousMillis > loopInterval)
  {
    previousMillis = currentMillis;

    check_time();
	DisplayShow();

  }  //main loop time loop 
  
}









