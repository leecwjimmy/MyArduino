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
  無延遲閃爍

   開啟和關閉連接到數位引腳的發光二極體 (LED)，
   不使用delay()函數。 這意味著其他代碼可以在
   同時，不會被 LED 程式碼中斷。

   電路：
   - 使用板載 LED。
   - 注意：大多數 Arduino 都有一個可以控制的板載 LED。 在 UNO、MEGA 上
     零它連接到數位引腳 13，在 MKR1000 的引腳 6 上。 LED_BUILTIN
     設定為正確的 LED 引腳，與使用哪個板無關。
     如果您想知道板載 LED 連接到您的哪個引腳
     Arduino 型號，請在以下位置查看您的主機板的技術規格：
     https://www.arduino.cc/en/Main/Products

   創立於 2005 年
   作者：大衛·A·梅利斯
   2010 年 2 月 8 日修改
   作者：保羅‧斯托夫雷根
   2013 年 11 月 11 日修改
   作者：史考特‧費茲傑拉
   2017 年 1 月 9 日修改
   作者：阿圖羅·瓜達盧皮

   此範例程式碼屬於公共領域。

   https://www.arduino.cc/en/Tutorial/BuiltInExamples/BlinkWithoutDelay
本著技術共享精神本程式碼歡迎取用與改進當你們有什麼發現或者建議，請與我分享你們的喜悅
In the spirit of technology sharing, this code is welcome to be accessed and improved.
If you have any discoveries or suggestions, please share your joy with me.
2023/12/19 初版
2023/12/19 First edition
2023/12/19 作者李進衛
2023/12/19 Author Lee Chin wei
2023/12/19 使用Arduino UNO
2023/12/19 Using Arduino UNO
2022/12/20 新增加OLED 程式
2022/12/20 Newly added OLED program
2022/12/21 新增VR debounce 時間5ms 取樣一次改為20ms取樣一次
2022/12/21 Added VR debounce time 5ms sampling time changed to 20ms sampling time
每20ms讀取VR的電壓一次-->將讀取結果與上一次讀取結果比對如果一樣就不進行電壓換換與更新OLED的內容，如果不一樣
表示電壓值被改變就進行電壓和MIDI值換算，並且更新OLED的內容。
Read the VR voltage every 20ms --> Compare the reading result with the last reading result.
If they are the same, do not perform voltage replacement and update the OLED content.
If they are different,If the voltage value is changed,
the voltage and MIDI values ​​are converted, and the content of the OLED is updated.
2023/12/22 IQC 吳東陽要求電壓降為小數點兩位數，由於改為輸出CSV檔案格式因此保留小數點第四位
2023/12/22 IQC 要求數據丟到PC 讓他不用抄數據
2023/12/23 新增加按鍵，請邱盈繼幫忙焊接按鍵
2023/12/13 修正判斷式與運算(ADC值*電壓)/(1023)
治具整理:鄭民杰
2023/12/22 IQC Wu Dongyang requires that the voltage drop be to two decimal places.
 Since the output is changed to CSV file format, the fourth decimal place is retained.
2023/12/22 IQC requires the data to be thrown to the PC so that he does not have to copy the data
2023/12/23 New buttons are added, please ask Qiu Yingji to help solder the buttons
2023/12/13 Modified judgment formula and operation (ADC value * voltage)/(1023)
Fixture arrangement: Zheng Minjie
2023/12/24 增加abs絕對值判斷，以及好與壞的判斷。
*/
#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
// constants won't change. Used here to set a pin number:
const int ledPin = LED_BUILTIN; // the number of the LED pin

// Variables will change:
int ledState = LOW; // ledState used to set the LED
#define P_VR A0
const int P_key = 7;
#define TBASE 5
float voltage = 0.000;
#define T250MS 250 / TBASE
#define T20MS 20 / TBASE
#define vref_val 3.3030   // 開發板的參考電壓使用SANWA PC7000三用表量測
#define debounce_delay 10 // debounce time
#define Press 0
#define Release 1
#define midi_central_value 64
int workCounter = T250MS;
int vrdebounce = T20MS;
int analogValue = 0;
int old_analogValue = 0;
int MIDI = 0;
// String S_MIDI;
// String S_VOLTAGE;
String Result;
// String SS_VOLTAGE;
// String SS_MIDI;
int absvalu = 0;
// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
// 一般來說，對於保存時間的變數應該使用“unsigned long”
// 該值很快就會變得太大而無法儲存 int
unsigned long previousMillis = 0; // will store last time LED was updated

// constants won't change:
// 常數不會改變：
const long interval = 5; // interval at which to blink (milliseconds)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);
// U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);
// U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);   // All Boards without Reset of the Display
void setup()
{

  Serial.begin(115200);
  Serial.println("VR20231224_OLED_LOG.ino");
  // set the digital pin as output:
  pinMode(ledPin, OUTPUT);
  pinMode(P_VR, INPUT);
  pinMode(P_key, INPUT_PULLUP);
  u8g2.begin();
  u8g2.enableUTF8Print(); // enable UTF8 support for the Arduino print() function
}

/*
Main loop : 5ms
任務:
1. 偵測按鍵是否被按下
2. 工作LED 2HZ閃爍
3. 當按鍵被按下讀取可變電阻電壓
4. 計算電壓值
5. 將ADC值以及電壓顯示在OLED 並且將數據傳送到PC端
Main loop: 5ms
Task:
1. Detect whether the button is pressed
2. Working LED flashes at 2HZ
3. Read the variable resistor voltage when the button is pressed
4. Calculate voltage value
5. Display the ADC value and voltage on the OLED and transmit the data to the PC
*/
void loop()
{
  // here is where you'd put code that needs to be running all the time.
  // check to see if it's time to blink the LED; that is, if the difference
  // between the current time and last time you blinked the LED is bigger than
  // the interval at which you want to blink the LED.
  // 這裡是放置需要一直運行的程式碼的地方。
  // 檢查 LED 是否閃爍； 也就是說，如果差異
  // 目前時間和上次閃爍 LED 之間的時間大於
  // LED 閃爍的時間間隔。

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval)
  {
    // save the last time you blinked the LED
    previousMillis = currentMillis;
    //    vrdebounce--; //vr debounce time is 20ms
    //    if (vrdebounce == 0)
    //    {
    //      vrdebounce = T20MS;
    //      if (analogValue != old_analogValue)
    if (digitalRead(P_key) == Press)
    {
      delay(debounce_delay); // Debounce time
      analogValue = analogRead(P_VR);
      voltage = analogValue;
      old_analogValue = analogValue;
      //        Serial.println("ADC:" + String(voltage));
      int shiftvalue = analogValue >> 3; // 右移3bit
      absvalu = abs(64 - shiftvalue);
      if (absvalu <= 3)
      {
        Result = "OK";
      }
      else
      {
        Result = "NG";
      }
      String S_MIDI = "MIDI==>" + String(shiftvalue, HEX);
      String SS_MIDI = String(shiftvalue, HEX);
      String DEC_MIDI = String(shiftvalue, DEC);
      //        Serial.println("MIDI==>" + S_MIDI);

      //  MIDI = map(analogValue,0,1024,0,127);
      voltage = (voltage * 3.3063) / 1023.00;
      //      String S_VOLTAGE = String(voltage, 4) + "V";
      //      String SS_VOLTAGE = String(voltage, 4);
      //        Serial.println("VR voltage:" + S_VOLTAGE + "V");

      u8g2.clearBuffer();              // clear the internal memory
      u8g2.setFont(u8g2_font_cu12_tr); // u8g2_font_unifont_t_chinese2); //u8g2_font_ncenB08_tr);	// choose a suitable font
      u8g2.setCursor(0, 15);
      u8g2.print(S_MIDI); // 顯示MIDI值
      u8g2.setCursor(0, 30);
      u8g2.print(voltage, 4); // 顯示電壓值
      u8g2.print("V");        // write something to the internal memory
      u8g2.setCursor(0, 45);
      u8g2.print(Result); // 2023/12/25增加顯示測試結果

      u8g2.sendBuffer();        // transfer internal memory to the display
                                //  Serial.println("VR MIDI Value:" + String(MIDI));
                                //  MIDI = MAP(analoagValue,0,1024,0,127);
                                //    Serial.print(voltage);
                                //    Serial.println("V");
                                //        if (digitalRead(P_key) ==0)
                                //        {
                                // 輸出CSV檔案格式好方便將log file匯入excel分析，Arduino c 似乎不接受太長的字串所以分多筆送出
                                // The output CSV file format is very convenient for importing the log file into excel
                                // for analysis. Arduino c does not seem to accept too long strings, so it is sent in multiple strokes.
      Serial.print(voltage, 4); // 送出量測電壓值
      Serial.print(",");
      Serial.print(SS_MIDI);        // 送出16進制MIDI
      Serial.print(",");            // SS_VOLTAGE + "," + SS_MIDI );
      Serial.print(DEC_MIDI + ","); // 送出10進制MIDI
      Serial.print(absvalu);        // 送出與中心電壓MIDI值的差距絕對值
      Serial.println("," + Result); // 輸出CSV檔案格式好方便將log file匯入excel分析
      //        }

      while (digitalRead(P_key) == Press)
        ;                    // wiat key reless
      delay(debounce_delay); // debounce time
      //      }
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
