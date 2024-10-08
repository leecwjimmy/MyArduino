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
2022-10-03 開始使用WEMOS D1 I2C讀取資料都是錯誤的目前還沒有查出原因 2024/02/16李進衛處理好了降低clock速度就可以解決
2022-10-05 使用ESP32 開發板但是當接上充電時就會讀到錯誤的資料,2024/02/16李進衛查出原因，是clock速度太快，傳輸資料出現錯誤
           只要將頻率由內定的400KHZ-->降為100KHZ 就可以了
2024/02/02 使用Waveshare RP2040 Zero 剛compier 完成時我不知道上次的毛病是I2C不能使用，所以影響到OLED的顯示，我修改bq4050 lib 的內容將Wirw.being() mark
起來。之後OLED恢復正常，功能是否正常要跟邱盈繼確認。
2024/02/16 使用Waveshare RP2040 Zero 邱盈繼拿給我電池，我所讀到的資料為0，我查到最底層的程式依然沒有結果，我問SKYPE AI，
AI的回覆如下：我所得到的ack值是04 其他錯誤，因此我就將I2C的clock速度從400KHZ降到100KHZ我就可以正常的得到數值

您的問題很有意思，我會盡力為您解答。blush

Wire.beginTransmission( 0x0B/devAddress/ ) 這個函數是用來開始向一個 I2C 裝置發送資料的。
您需要在這個函數之後使用 Wire.write() 函數來寫入要傳送的字節，
然後使用 Wire.endTransmission() 函數來結束傳輸。 
https://reference.arduino.cc/reference/en/language/functions/communication/wire/begintransmission/ 
https://www.arduino.cc/en/Reference/WireBeginTransmission 
http://www.taichi-maker.com/homepage/reference-index/arduino-library-index/wire-library/arduino-wire-begintransmission/

當您使用 Wire.endTransmission() 函數時，Arduino 會自動等待裝置的 ACK bit 回應，並且返回一個錯誤代碼來表示傳輸的狀態。錯誤代碼的含義如下：

•  0: 成功

•  1: 傳輸過程中發生數據衝突

•  2: 裝置沒有回應 ACK bit

•  3: 裝置回應了 NACK bit

•  4: 其他錯誤 https://dotblogs.com.tw/April_Notes/2021/11/24/165906

因此，您可以使用 Wire.endTransmission() 函數的返回值來判斷裝置是否有正常回應。如果返回值為 0，則表示裝置已經接收了您的資料並且回應了 ACK bit。如果返回值為 2 或 3，則表示裝置沒有回應或者拒絕了您的資料，這時您可以檢查您的裝置地址和資料格式是否正確，或者嘗試重新傳輸。如果返回值為 1 或 4，則表示傳輸過程中發生了錯誤，這時您可以檢查您的連接線路和電源是否穩定，或者減少傳輸的速度和距離。

如果您使用示波器來觀察 I2C 的通訊波形，您需要注意以下幾點：

•  您需要同時連接 SDA 和 SCL 線到示波器的兩個通道，並且設置合適的觸發條件和時間基準。

•  您需要確保您的示波器的探頭和接地線與 Arduino 和裝置的接地線相連，否則可能會產生雜訊或者偏移。

•  您需要確保您的示波器的探頭和接地線的長度盡量短，否則可能會影響 I2C 的信號品質。

•  您需要確保您的示波器的探頭和接地線不要與其他的線路或元件相交或靠近，否則可能會產生干擾或者耦合。

如果您按照以上的方法操作，您應該可以正確地讀取 I2C 的通訊波形，並且看到裝置的 ACK bit 回應。如果您仍然看到 ACK bit 是 HIGH，那麼可能是因為您的裝置有問題，或者您的示波器有問題。您可以嘗試更換其他的裝置或者示波器，或者使用其他的 I2C 工具來檢測您的裝置的狀態。

希望這些資訊對您有幫助。如果您還有其他問題，歡迎再次詢問。blush
void Lcd_display(void)
{
//  Wire.beginTransmission(0x0b);  // Transmit to device number 44 (0x2C)
  digitalWrite(P_debug, LOW);
  	Wire.setClock(100000); // 設定 i2c 的速率為 100 KHz RP2040 
	^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^          
  BQ4050.readReg(registers.relativeStateOfCharge);
  digitalWrite(P_debug,HIGH);
**/

// Libraries to be included

#include <Wire.h>
#include "Arduino.h"
#include <Lorro_BQ4050.h>
// #include <LiquidCrystal_I2C.h>  // I2C LCD 使用的函數 使用LiquidCrystal目錄之下的LiquidCrystal_I2C.h
#include <U8g2lib.h>
#include <SPI.h>

// Default address for device. Note, it is without read/write bit. When read with analyser,
// this will appear 1 bit shifted to the left
//#define BQ4050addr 0x0B
// Initialise the device and library
//Lorro_BQ4050 BQ4050(BQ4050addr);
// Instantiate the structs
//extern Lorro_BQ4050::Regt registers;
uint32_t pa;
uint32_t previousMillis;
uint16_t loopInterval = 1000;
float V = 0.00;
float CV1 = 0.00;
float CV2 = 0.00;
float CV3 = 0.00;
float CV4 = 0.00;
float cr = 0.00;
int counter = 0;
#define P_debug 14
// LiquidCrystal_I2C lcd(0x27, 16, 2);  // set the LCD address to 0x27 for a 16 chars and 2 line display
// U8G2_ST7920_128X64_F_HW_SPI u8g2(U8G2_R0, /* CS=*/ 2/* 15*/, /* reset=*/ 8); // Feather HUZZAH ESP8266, E=clock=14, RW=data=13, RS=CS
//  使用軟體的方式減少雜訊的輸出造成資料錯誤，使用硬體的方式會導致LCD無法顯示
//
// U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, /* clock=*/ 14, /* data=*/ 13, /* CS=*/ 15, /* reset=*/ 16); //2022-10-06使用在ESP32黃色pin的開發板【這個參數for WEMOS D1 ESP8266 】 Feather HUZZAH ESP8266, E=clock=14, RW=data=13, RS=CS
// U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, /* clock=*/ D5, /* data=*/ D7, /* CS=*/ D8, /* reset=*/ 16);  // for ESP8266
// U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, /* clock=*/ 4, /* data=*/ 5, /* CS=*/ 7, /* reset=*/ 12); //2022-10-06使用在Arduino Zero
//U8G2_ST7920_128X64_F_HW_SPI u8g2(U8G2_R0, /* CS=*/53, /* reset=*/8); // for Arduino Mega2560 HD SPI
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);
#define BQ4050addr 0x0B
// Initialise the device and library
Lorro_BQ4050 BQ4050(BQ4050addr);
// Instantiate the structs
extern Lorro_BQ4050::Regt registers;
void setup()
{

  Serial.begin(115200);
//  Wire.begin();
  Serial.println("bq4050_20240202_V4.ino Version:01 YH PE !! RP2040 Zero");
  pinMode(P_debug,OUTPUT);
  digitalWriteFast(P_debug,HIGH);
  //  lcd.init();  // initialize the lcd
  //  lcd.backlight();
  // Wire.setClock(50000); //從100KHZ降到50KHZ SCK頻率
  u8g2.begin();
  u8g2.enableUTF8Print(); // enable UTF8 support for the Arduino print() function
  u8g2.clearBuffer(); 
  u8g2.setFont(u8g2_font_unifont_t_chinese1);  // use chinese2 for all the glyphs of "你好世界"
    u8g2.setFontDirection(0);
  //  u8g2.clearBuffer();
    u8g2.setCursor(0, 15);
    u8g2.print("亞弘電科技生技課");
    u8g2.setCursor(0, 35);
    u8g2.print("0202V4");   //
      u8g2.sendBuffer();
      delay(2000);
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

void Lcd_display(void)
{
//  Wire.beginTransmission(0x0b);  // Transmit to device number 44 (0x2C)
  digitalWrite(P_debug, LOW);
  Wire.setClock(100000); // 設定 i2c 的速率為 100 KHz RP2040 
  BQ4050.readReg(registers.relativeStateOfCharge);
  digitalWrite(P_debug,HIGH);

  pa = 0;
  pa = registers.relativeStateOfCharge.val;
  Serial.print("State of charge: ");
  Serial.print(pa);
  Serial.println("%");

  delay(15);

  digitalWrite(P_debug, LOW);
  BQ4050.readReg(registers.voltage);
    digitalWrite(P_debug,HIGH);

  V = 0.00;
  V = registers.voltage.val;
  V /= 1000;
      Serial.print( "Pack voltage: " );
      Serial.print( V );
      Serial.println( "V" );
  delay(15);

  u8g2.setFont(u8g2_font_unifont_t_chinese1); // use chinese2 for all the glyphs of "你好世界"
  u8g2.setFontDirection(0);
  u8g2.clearBuffer();
  u8g2.setCursor(0, 15);
  u8g2.print("亞弘電科技生技課");
  u8g2.setCursor(0, 35);
  u8g2.print("剩餘"); //
                      //	u8g2_1.setFont(u8g2_font_courB08_tf); //(u8g2_font_amstrad_cpc_extended_8f);  //(u8g2_font_courB08_tf);  //(u8g2_font_victoriabold8_8u);  //(u8g2_font_fub20_tf); //(u8g2_font_ncenB14_tr); // choose a suitable font
  u8g2.print(":");    //
  u8g2.print(pa);     //
  u8g2.print("%");    //
                      // pa = 0;
                      //  pa =  registers.relativeStateOfCharge.val;
  if (pa < 28)
  {
    u8g2.print(" 請充電"); //
  }
  else if (pa > 30)
  {
    u8g2.print(" 請放電"); //
  }
  else
  {
    u8g2.print(" 良好");
  } //

  //  u8g2.setFont(u8g2_font_unifont_t_chinese1);  // use chinese2 for all the glyphs of "你好世界"
  u8g2.setCursor(0, 60);
  u8g2.print("電壓"); //
                      //	u8g2_1.setFont(u8g2_font_courB08_tf); //(u8g2_font_amstrad_cpc_extended_8f);  //(u8g2_font_courB08_tf);  //(u8g2_font_victoriabold8_8u);  //(u8g2_font_fub20_tf); //(u8g2_font_ncenB14_tr); // choose a suitable font
  u8g2.print(":");    //
                      //    V = 0;
                      //    V = registers.voltage.val;
                      //    V /= 1000;
  u8g2.print(V);      //
  u8g2.print("V");    //
  u8g2.sendBuffer();

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
  BQ4050.readReg(registers.cellVoltage1);
  CV1 = 0.00;
  CV1 = registers.cellVoltage1.val;
  CV1 /= 1000;
  //    Serial.print( "Cell voltage 1: " );
  //    Serial.print( CV1 );
  //    Serial.println( "V" );
  delay(15);

  BQ4050.readReg(registers.cellVoltage2);
  CV2 = 0.00;
  CV2 = registers.cellVoltage2.val;
  CV2 /= 1000;

  //    Serial.print( "Cell voltage 2: " );
  //    Serial.print( CV2 );
  //    Serial.println( "V" );
  delay(15);

  BQ4050.readReg(registers.cellVoltage3);
  CV3 = 0.00;
  CV3 = registers.cellVoltage3.val;
  CV3 /= 1000;

  //    Serial.print( "Cell voltage 3: " );
  //    Serial.print( CV3 );
  //    Serial.println( "V" );
  delay(15);

  BQ4050.readReg(registers.cellVoltage4);
  CV4 = 0.00;
  CV4 = registers.cellVoltage1.val;
  CV4 /= 1000;

  //    Serial.print( "Cell voltage 4: " );
  //    Serial.print( CV4 );
  //    Serial.println( "V" );
  delay(15);

  BQ4050.readReg(registers.current);
  cr = 0.00;
  cr = registers.current.val;
  //    Serial.print( "Current: " );
  //    Serial.print( cr);
  //    Serial.println( "mA" );
  delay(15);
  u8g2.setFont(u8g2_font_unifont_t_chinese1); // use chinese2 for all the glyphs of "你好世界"
  u8g2.setFontDirection(0);
  u8g2.clearBuffer();
  u8g2.setCursor(0, 15);
  u8g2.print("電流:");
  //  cr = 0;
  //  cr = registers.current.val;
  u8g2.print(cr);
  //  cr = registers.current.val;
  u8g2.print("mA");
  //***********************************************************************************************
  u8g2.setCursor(0, 35);
  u8g2.print("一:"); //
                     //  CV1 = 0;
                     //  CV1 =  registers.cellVoltage1.val;
                     //  CV1 /= 1000;
  u8g2.print(CV1);   //
  u8g2.print("V");   //
  u8g2.print("二:"); //
                     //  CV2 = 0;
                     //  CV2 =  registers.cellVoltage2.val;
                     //  CV2 /= 1000;
  u8g2.print(CV2);   //
  u8g2.print("V");   //

  u8g2.setCursor(0, 60);
  u8g2.print("三:"); //
                     //  CV3 = 0;
                     //  CV3 =  registers.cellVoltage3.val;
                     //  CV3 /= 1000;
  u8g2.print(CV3);   //
  u8g2.print("V");   //
  u8g2.print("四:"); //
                     //  CV4 = 0;
                     //  CV4 =  registers.cellVoltage4.val;
                     //  CV4 /= 1000;
  u8g2.print(CV4);   //
  u8g2.print("V");   //
  u8g2.sendBuffer();
}

void loop()
{

  uint32_t currentMillis = millis();

  if (currentMillis - previousMillis > loopInterval)
  {
    previousMillis = currentMillis;
    /*   if (BQ4050.readReg( registers.relativeStateOfCharge ) ==false)
       {
     u8g2.clearBuffer();
     u8g2.setCursor(0,15);
     u8g2.print("不良");
     u8g2.setCursor(0, 30);
     u8g2.print("I2C連線失敗");

         Serial.println("*************************************************通信失敗*************************************");
        u8g2.sendBuffer();
        while(!BQ4050.readReg( registers.relativeStateOfCharge ));
         Lorro_BQ4050 BQ4050( BQ4050addr );
   //Wire.begin();
        BQ4050.deviceReset();
   //     delay(100);
   //     while(!BQ4050.deviceReset());
       } */

    // 2022-12-30 mark   counter +=1;
    //    Serial.println(counter);
    //  Lorro_BQ4050::Regt registers;
    //    if (BQ4050.readReg( registers.relativeStateOfCharge ) ==false)
    //    {

    //      Serial.println("*************************************************通信失敗*************************************");
    //    }

    Lcd_display();
    // 2022-12-30 mark   switch(counter)
    // 2022-12-30 mark   {
    // 2022-12-30 mark     case 1: Lcd_display(); break;
    // 2022-12-30 mark     case 2: Lcd_display1(); break;
    // 2022-12-30 mark     case 3: counter = 0; break;
    // 2022-12-30 mark     }
    /*
  if (counter <=2)  //顯示資料5秒變化一次
  {
  Lcd_display(); //顯示logo 電量以及電壓
  Serial.print(" <5");
  Serial.println(counter);
  } else if ((counter>2)&&(counter <=4))
  {
  Serial.print(" >5 <=10 ");
  Serial.println(counter);

      Lcd_display1(); //顯示每個電池電壓以及充電電流

  } else if (counter > 10) {

      counter = 0;

  }  */
    //    delay(1000);
  }
}
