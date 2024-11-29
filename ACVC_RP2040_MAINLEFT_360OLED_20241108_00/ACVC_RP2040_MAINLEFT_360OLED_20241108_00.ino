/*
**********************************************
程式設計: 李進衛
日期: 2024/11/07
開發板: YD RP-2040
開發系統: Arduino


***********************************************
*/

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_TinyUSB.h> //RP2040使用下載程式的USB 當作UART 傳輸資料
#define _PWM_LOGLEVEL_        3
#include "RP2040_PWM.h"
//#include <FreqCountRP2.h>

// 創建PWM實例
RP2040_PWM* PWM_Instance;

// 定義 OLED 尺寸和數量
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

#define TL_nOCS0 2
#define TL_nORST 3
#define TL_OD_C	 4
#define TL_OSCK	 5
#define TL_OSDI  6 
#define TL_SEND  7

// 定義LED引腳
#define LED_PIN 25 //25 Raspberry  pi pico

// 觸控控制

#define TL_RET 26  // input 
#define TL_PHA 27	// input 
#define TL_PHB 28

float previousDutyCycle = 0;
const float threshold = 70.0; // 70%的變化閾值 避免環境誤動作

// LED 閃爍變數
unsigned long ledLastToggleTime = 0;
bool ledState = false;
const unsigned long ledInterval = 500; // 2Hz 閃爍，每次切換狀態間隔 500ms
bool reading = 0;
bool touchOK = 0;
#define MAX_VOLTAGE 3.3
#define PI 3.14159

// 中心點和指針長度設定
//#define CENTER_X (SCREEN_WIDTH / 2)
//#define CENTER_Y (SCREEN_HEIGHT / 2)
//#define POINTER_LENGTH 14  // 指針長度
#define MAX_VOLTAGE 3.3
#define BAR_MAX_WIDTH SCREEN_WIDTH
#define bar_HEIGHT 13


// 定義OLED diaaplay物件，Reset 和 CS 不定義有外部使用者自行控制 使用軟體SPI 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, TL_OSDI /*&SPI*/,TL_OSCK /*CLK*/,TL_OD_C/*DC*/, TL_nORST /*OLED_RESET*/, TL_nOCS0/*OLED_CS0*/);

void check_vr(void)
{
	if (touchOK == 0)
		return;
 // 讀取VR電壓
  int rawValue = analogRead(TL_PHA);
  float voltage = (rawValue / 1024.0) * MAX_VOLTAGE;

  int rawValue1 = analogRead(TL_PHB);
  float voltage1 = (rawValue1 / 1024.0) * MAX_VOLTAGE;

  // 計算長條圖寬度
  int barWidth = map(voltage * 100, 0, MAX_VOLTAGE * 100, 0, BAR_MAX_WIDTH);
 int barWidth1 = map(voltage1 * 100, 0, MAX_VOLTAGE * 100, 0, BAR_MAX_WIDTH);
  // 更新OLED顯示
  display.clearDisplay();
  if (voltage > 0) {
    display.fillRect(0, 0, barWidth, bar_HEIGHT, SSD1306_WHITE);
    display.fillRect(0, 20, barWidth1, bar_HEIGHT, SSD1306_WHITE);

  } else {
    display.fillRect(SCREEN_WIDTH - barWidth, 0, barWidth, bar_HEIGHT, SSD1306_WHITE);
     display.fillRect(SCREEN_WIDTH - barWidth1, 20, barWidth1, bar_HEIGHT, SSD1306_WHITE);
 }
//   display.setTextSize(1);             // Normal 1:1 pixel scale
//  display.setTextColor(SSD1306_WHITE);        // Draw white text
//  display.setCursor(0,20);             // Start at top-left corner
//  display.println(voltage); 

  display.display();

  delay(50);  // 適度延遲
	
	
}


void  check_touch(void)
{
	if (touchOK ==1)
		return;
// 測量高電平和低電平的持續時間
unsigned long highTime = pulseIn(TL_RET, HIGH);
unsigned long lowTime = pulseIn(TL_RET, LOW);

// 計算總周期和佔空比
unsigned long period = highTime + lowTime;
float dutyCycle = (highTime / (float)period) * 100.0;

Serial.print("Duty Cycle: ");
Serial.print(dutyCycle);
Serial.println(" %");

// 檢查佔空比變化是否超過閾值
if (abs(dutyCycle - previousDutyCycle) > threshold) {
	
  display.clearDisplay();
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.println(F("Touch ON!!"));
  display.display();
	touchOK =1;
}  //else
//{
 // display.clearDisplay();
//  display.setTextSize(1);             // Normal 1:1 pixel scale
//  display.setTextColor(SSD1306_WHITE);        // Draw white text
//  display.setCursor(0,0);             // Start at top-left corner
//  display.println(F("Touch OFF!!"));
  
//  display.display();
	
	
//}

previousDutyCycle = dutyCycle;	
	
}

void setup() {
  // setting serial port for debug
  Serial.begin(115200);
  Serial.println("Power On initial");
  Serial.println("ACVC_RP2040_MAINLEFT_360OLED_20241107_00.ino");
  // 初始化引腳
  pinMode(TL_nOCS0, OUTPUT);
  pinMode(TL_nORST, OUTPUT);
//  pinMode(OSEL2, OUTPUT);
  pinMode(TL_OD_C, OUTPUT);
  pinMode(TL_OSCK, OUTPUT);
  pinMode(TL_OSDI, OUTPUT);
  pinMode(TL_SEND, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
// 初始化PWM實例，設置頻率為100Hz，佔空比為50%
PWM_Instance = new RP2040_PWM(TL_SEND, 100, 50);
//PWM_Instance->setPWM(TL_SEND, 100, 50);
//}
  pinMode(TL_RET, INPUT_PULLUP);
//  pinMode(TL_PHA, INPUT_PULLUP);
//  pinMode(TL_PHB, INPUT_PULLUP);
//	FreqCountRP2.begin(inputPin, 100); // 初始化頻率計數器，設置為100Hz 
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
//  display.begin(SSD1306_SWITCHCAPVCC);
  display.clearDisplay();  
  display.display();
  display.fillScreen(SSD1306_WHITE); // 填滿畫面
  display.display();
  delay(1000);
  display.clearDisplay(); // 清除畫面
  display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE); // 畫邊框
  display.display();
  delay(1000);
  display.clearDisplay(); // 清除畫面
  display.display();
  PWM_Instance->setPWM(TL_SEND, 100, 50);
  PWM_Instance->setPWM(0, 1000, 50);  
  
}

//************************
// 處理 LED 以 2Hz 閃爍
//************************

void handleLEDBlink() {
  unsigned long currentMillis = millis();
  if (currentMillis - ledLastToggleTime >= ledInterval) {
    ledLastToggleTime = currentMillis;
    ledState = !ledState; 				// 切換 LED 狀態
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
  }
}

void Touch_read(void)
{
	
//	digitalWrite(TL_SEND,HIGH);
int adcValue = analogRead(TL_RET); // 讀取ADC0的值
float voltage = adcValue * (3.3 / 4095.0); // 將ADC值轉換為電壓
Serial.print("ADC Value: ");
Serial.print(adcValue);
Serial.print(" - Voltage: ");
Serial.println(voltage);
delay(500);	
//	digitalRead(TL_RET);
/*	
	if (reading ==1)
	{
		Serial.print("VR is Touch");
  display.clearDisplay();

  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.println(F("VR Touch !!"));
  display.display();
  
	} else{
		
  display.clearDisplay();

  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.println(F("VR OFF !!"));
  display.display();
		
	} */
}
/*
***************************
***** Main Loop
***************************
*/
void loop() {


	handleLEDBlink(); //工作燈
	check_touch();
	check_vr();
//	Touch_read();


/*
int adcValue = analogRead(TL_PHA); // 讀取ADC0的值
float voltage = (adcValue * 3.3) / 1024; //4095.0); // 將ADC值轉換為電壓
	delay(1);
int adcValueB = analogRead(TL_PHB); // 讀取ADC0的值
float voltageB = (adcValueB * 3.3) / 1024; //4095.0); // 將ADC值轉換為電壓
display.clearDisplay();
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0,10);             // Start at top-left corner
  display.println(voltage);

  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0,20);             // Start at top-left corner
  display.println(voltageB);
  display.display();	
	delay(1);
	*/
}