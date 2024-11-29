/*
******************************************************
日期： 2024-11-25
程式設計： 李進衛
開發板： Raspberry pi pico RP2040 + 擴充板
開發環境： Arduino IDE 2.2.1
機種： ACVC
PCB ： MIDI
2024/11/27 最終版本

*******************************************************
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_TinyUSB.h> //RP2040使用下載程式的USB 當作UART 傳輸資料
#include "RP2040_PWM.h"

#define _PWM_LOGLEVEL_ 3

// 創建PWM實例
// RP2040_PWM *PWM_Instance;

// 定義 OLED 尺寸和數量
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
// #include <Adafruit_TinyUSB.h> // 使用Arduino DUE 不用代入這個函數 RP2040使用下載程式的USB 當作UART 傳輸資料
#define OLED_RESET -1		// Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define LED_PIN 25 // RP2040
// LED 閃爍變數
unsigned long ledLastToggleTime = 0;
unsigned long lastCheck = 0;
int checkInterval = 10;
int T_CNT1 = 0;
int T_CNT2 = 0;
int T_1HZ = 500 / checkInterval;
int T_2HZ = 250 / checkInterval;
int T_LED = 0;
int ledSqn = 0;
bool blink1hz = false;
bool blink2hz = false;
bool ledState = false;
const unsigned long ledInterval = 500; // 2Hz 閃爍，每次切換狀態間隔 500ms
float previousDutyCycle0 = 0;
float previousDutyCycle1 = 0;
float previousDutyCycle2 = 0;
float previousDutyCycle3 = 0;
unsigned long highTime0 = 0;
unsigned long lowTime0 = 0;
unsigned long highTime1 = 0;
unsigned long lowTime1 = 0;
unsigned long highTime2 = 0;
unsigned long lowTime2 = 0;
unsigned long highTime3 = 0;
unsigned long lowTime3 = 0;
unsigned long period0 = 0;
unsigned long period1 = 0;
unsigned long period2 = 0;
unsigned long period3 = 0;
float dutyCycle0 = 0;
float dutyCycle1 = 0;
float dutyCycle2 = 0;
float dutyCycle3 = 0;
bool touchOK = 0;
int counter = 0;
int key_SQN = 0;
#define MIDIOUTD 0
#define MIDIOUTC 1
#define MIDIOUTB 2
#define MIDIOUTA 3
#define MIDIIN2 6
#define MIDIIN1 7
#define KEY 8
#define T20MS 2
#define T60MS 6
#define T100MS 10
uint32_t PWM_Pins[] = {MIDIOUTD, MIDIOUTC, MIDIOUTB, MIDIOUTA};

#define NUM_OF_PINS (sizeof(PWM_Pins) / sizeof(uint32_t))

float dutyCycle[NUM_OF_PINS] = {50.0f, 50.0f, 50.0f, 50.0f};

float freq[] = {10000.0f, 10000.0f, 10000.0f, 10000.0f};
float measuredFrequency1 = 0;
float measuredFrequency2 = 0;

RP2040_PWM *PWM_Instance[NUM_OF_PINS];

// 測量頻率函數
float measureFrequency(int pin)
{
	unsigned long startTime = micros();
	unsigned long highTime = pulseIn(pin, HIGH, 500);
	unsigned long lowTime = pulseIn(pin, LOW, 500);
	unsigned long totalTime = highTime + lowTime;

	if (totalTime == 0)
		return 0;				  // 避免除以零
	return 1000000.0 / totalTime; // 頻率 = 1 / 週期
}

void NewCheckTime(void)
{
	unsigned long currenCheck = millis();
	if (currenCheck - lastCheck >= checkInterval)
	{
		lastCheck = currenCheck;
		if (T_CNT1 != 0)
			T_CNT1--;
		if (T_CNT2 != 0)
			T_CNT2--;
		if (T_LED != 0)
			T_LED--;
		if (T_1HZ != 0)
		{
			T_1HZ--;
		}
		else
		{
			T_1HZ = 500 / checkInterval;
			blink1hz ^= 1;
		}

		if (T_2HZ != 0)
		{
			T_2HZ--;
		}
		else
		{
			T_2HZ = 250 / checkInterval;
			blink2hz ^= 1;
		}
	}
}

//************************
// 處理 LED 以 2Hz 閃爍
//************************

void handleLEDBlink()
{
	unsigned long currentMillis = millis();
	if (currentMillis - ledLastToggleTime >= ledInterval)
	{
		ledLastToggleTime = currentMillis;
		ledState = !ledState; // 切換 LED 狀態
		digitalWrite(LED_PIN, ledState ? HIGH : LOW);
	}
}

void setup()
{
	Wire.begin();		   //(D2,D1);  //(0,2); //sda=0 | D3, scl=2 | D4
	Wire.setClock(100000); // 設定 i2c 的速率為 100 KHz RP2040 很重要不然會導致通信錯誤！！ 導線太長速度太快會導致資料錯誤！！！2024/11/03 by leecw
	Serial.begin(115200);
	//  for (uint8_t index = 0; index < NUM_OF_PINS; index++)
	//  {
	//    Serial.print(index);
	//    Serial.print("\t");
	//    Serial.print(PWM_Pins[index]);
	//    Serial.print("\t");
	//    Serial.print(freq[index]);
	//    Serial.print("\t\t");
	//    Serial.print(dutyCycle[index]);

	//    PWM_Instance[index] = new RP2040_PWM(PWM_Pins[index], freq[index], dutyCycle[index]);

	//    if (PWM_Instance[index])
	//    {
	//      PWM_Instance[index]->setPWM();

	//      uint32_t div = PWM_Instance[index]->get_DIV();
	//      uint32_t top = PWM_Instance[index]->get_TOP();

	//      Serial.print("\t\t");
	//     Serial.println(PWM_Instance[index]->getActualFreq());

	//      PWM_LOGDEBUG5("TOP =", top, ", DIV =", div, ", CPU_freq =", PWM_Instance[index]->get_freq_CPU());
	//    }
	//    else
	//    {
	//      Serial.println();
	//    }
	//  }

	pinMode(LED_PIN, OUTPUT);		// 工作LED閃爍 初始化
									//	pinMode(MIDIOUTD, OUTPUT); // MIDIOUTPUT PIN
									//	pinMode(MIDIOUTC, OUTPUT); // MIDIOUTPUT PIN
									//	pinMode(MIDIOUTB, OUTPUT); // MIDIOUTPUT PIN
									//	pinMode(MIDIOUTA, OUTPUT); // MIDIOUTPUT PIN
	pinMode(MIDIIN2, INPUT_PULLUP); // MIDIIN PIN
	pinMode(MIDIIN1, INPUT_PULLUP); // MIDIIN PIN
	pinMode(KEY, INPUT_PULLUP);		// key

	// 設定PWM信號
	//  analogWriteFrequency(TCH_SEND0, 100); // 設定頻率為1kHz
	//  analogWrite(pwmPin, 128);           // 設定佔空比50%
	// 初始化PWM實例，設定頻率為100Hz，佔空比為50%

	PWM_Instance[0] = new RP2040_PWM(MIDIOUTA, 1000, 0);
	PWM_Instance[0]->setPWM(MIDIOUTA, 10000, 50); // 頻率10KHZ duty cycel 50%

	PWM_Instance[1] = new RP2040_PWM(MIDIOUTB, 1000, 0);
	PWM_Instance[1]->setPWM(MIDIOUTB, 10000, 0); // 頻率10KHZ duty cycel 50%

	PWM_Instance[2] = new RP2040_PWM(MIDIOUTC, 10000, 0);
	PWM_Instance[2]->setPWM(MIDIOUTC, 10000, 0); // 頻率10KHZ duty cycel 50%

	PWM_Instance[3] = new RP2040_PWM(MIDIOUTD, 10000, 0);
	PWM_Instance[3]->setPWM(MIDIOUTD, 10000, 0); // 頻率10KHZ duty cycel 50%

	// SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
	if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
	{
		Serial.println(F("SSD1306 allocation failed"));
		for (;;)
			; // Don't proceed, loop forever
	}

	// Clear the buffer

	display.clearDisplay();
	display.clearDisplay();
	display.setTextSize(2);				 // Normal 1:1 pixel scale
	display.setTextColor(SSD1306_WHITE); // Draw white text
	display.setCursor(0, 0);			 // Start at top-left corner
	display.println(F("Yahorng PE"));
	display.display();
	delay(2000);
}

void keyprocess()
{
	switch (key_SQN)
	{
	case 0:
	{
		if (digitalRead(KEY) == 1 || T_CNT1 != 0)
			break;
		T_CNT1 = T60MS;
		key_SQN = 1;
		PWM_Instance[3]->setPWM(MIDIOUTD, 10000, 50); // 頻率10KHZ duty cycel 50%
		PWM_Instance[2]->setPWM(MIDIOUTC, 10000, 0);  // 頻率10KHZ duty cycel 0%
		PWM_Instance[0]->setPWM(MIDIOUTA, 10000, 0);  // 頻率10KHZ duty cycel 0%
		PWM_Instance[1]->setPWM(MIDIOUTB, 10000, 0);  // 頻率10KHZ duty cycel 0%

		break;
	}
	case 1:
	{
		if (T_CNT1 != 0)
		{
			break;
		}
		measuredFrequency1 = measureFrequency(MIDIIN1);
		//		float measuredFrequency2 = measureFrequency(MIDIIN2);

		if (measuredFrequency1 > 9000)
		{
			Serial.println("MIDIOUTD measuredFrequency1 is OK !!!");
			//	display.clearDisplay();
			display.clearDisplay();
			display.setTextSize(2);				 // Normal 1:1 pixel scale
			display.setTextColor(SSD1306_WHITE); // Draw white text
			display.setCursor(0, 0);			 // Start at top-left corner
			display.println(F("J1->J6 OK"));
			display.display();
		}
		else
		{
			Serial.println("MIDIOUTD measuredFrequency1 is Fail !!!");
			//	display.clearDisplay();
			display.clearDisplay();
			display.setTextSize(2);				 // Normal 1:1 pixel scale
			display.setTextColor(SSD1306_WHITE); // Draw white text
			display.setCursor(0, 0);			 // Start at top-left corner
			display.println(F("J1->J6 NG"));
			display.display();
		}

		measuredFrequency2 = measureFrequency(MIDIIN2); // J5
		if (measuredFrequency2 > 9000)
		{
			Serial.println("MIDIOUTC measuredFrequency2 is OK !!!");
			//	display.clearDisplay();
			//	display.clearDisplay();
			display.setTextSize(2);				 // Normal 1:1 pixel scale
			display.setTextColor(SSD1306_WHITE); // Draw white text
			display.setCursor(0, 17);			 // Start at top-left corner
			display.println(F("J1->J5 OK"));
			display.display();
		}
		else
		{

			Serial.println("MIDIOUTC measuredFrequency2 is Fail !!!");
			//	display.clearDisplay();
			//	display.clearDisplay();
			display.setTextSize(2);				 // Normal 1:1 pixel scale
			display.setTextColor(SSD1306_WHITE); // Draw white text
			display.setCursor(0, 17);			 // Start at top-left corner
			display.println(F("J1->J5 NG"));
			display.display();
		}
		PWM_Instance[3]->setPWM(MIDIOUTD, 10000, 0);  // 頻率10KHZ duty cycel 0%
		PWM_Instance[2]->setPWM(MIDIOUTC, 10000, 50); // 頻率10KHZ duty cycel 50%
		PWM_Instance[0]->setPWM(MIDIOUTA, 10000, 0);  // 頻率10KHZ duty cycel 0%
		PWM_Instance[1]->setPWM(MIDIOUTB, 10000, 0);  // 頻率10KHZ duty cycel 0%

		measuredFrequency1 = measureFrequency(MIDIIN1);

		if (measuredFrequency1 > 9000)
		{
			Serial.println("MIDIOUTD measuredFrequency1 is OK !!!");
			//	display.clearDisplay();
			//	display.clearDisplay();
			display.setTextSize(2);				 // Normal 1:1 pixel scale
			display.setTextColor(SSD1306_WHITE); // Draw white text
			display.setCursor(0, 34);			 // Start at top-left corner
			display.println(F("J2->J6 OK"));
			display.display();
		}
		else
		{
			Serial.println("MIDIOUTD measuredFrequency1 is Fail !!!");
			//	display.clearDisplay();
			//	display.clearDisplay();
			display.setTextSize(2);				 // Normal 1:1 pixel scale
			display.setTextColor(SSD1306_WHITE); // Draw white text
			display.setCursor(0, 34);			 // Start at top-left corner
			display.println(F("J2->J6 NG"));
			display.display();
		}

		measuredFrequency2 = measureFrequency(MIDIIN2); // J5
		if (measuredFrequency2 > 9000)
		{
			Serial.println("MIDIOUTC measuredFrequency2 is OK !!!");
			//	display.clearDisplay();
			//	display.clearDisplay();
			display.setTextSize(2);				 // Normal 1:1 pixel scale
			display.setTextColor(SSD1306_WHITE); // Draw white text
			display.setCursor(0, 48);			 // Start at top-left corner
			display.println(F("J2->J5 OK"));
			display.display();
		}
		else
		{

			Serial.println("MIDIOUTC measuredFrequency2 is Fail !!!");
			//	display.clearDisplay();
			//	display.clearDisplay();
			display.setTextSize(2);				 // Normal 1:1 pixel scale
			display.setTextColor(SSD1306_WHITE); // Draw white text
			display.setCursor(0, 49);			 // Start at top-left corner
			display.println(F("J2->5 NG"));
			display.display();
		}

		key_SQN = 2;
		while (!digitalRead(KEY))
			;
		T_CNT1 = T60MS;
		break;
	}
	case 2:
	{
		if (digitalRead(KEY) == 1 || T_CNT1 != 0)
		{
			break;
		}
		T_CNT1 = T60MS;
		key_SQN = 3;
		PWM_Instance[3]->setPWM(MIDIOUTD, 10000, 0);  // 頻率10KHZ duty cycel 0%
		PWM_Instance[2]->setPWM(MIDIOUTC, 10000, 0);  // 頻率10KHZ duty cycel 0%
		PWM_Instance[1]->setPWM(MIDIOUTB, 10000, 50); // 頻率10KHZ duty cycel 50%
		PWM_Instance[0]->setPWM(MIDIOUTA, 10000, 0);  // 頻率10KHZ duty cycel 0%
		break;
	case 3:
		if (T_CNT1 != 0)
		{
			break;
		}
		measuredFrequency1 = measureFrequency(MIDIIN1);
		if (measuredFrequency1 > 9000)
		{
			Serial.println("MIDIOUTB measuredFrequency1 is OK !!!");
			display.clearDisplay();
			display.setTextSize(2);				 // Normal 1:1 pixel scale
			display.setTextColor(SSD1306_WHITE); // Draw white text
			display.setCursor(0, 0);			 // Start at top-left corner
			display.println(F("J3->J6 OK"));
			display.display();
		}
		else
		{
			Serial.println("MIDIOUTB measuredFrequency1 is Fail !!!");
			display.clearDisplay();
			display.setTextSize(2);				 // Normal 1:1 pixel scale
			display.setTextColor(SSD1306_WHITE); // Draw white text
			display.setCursor(0, 0);			 // Start at top-left corner
			display.println(F("J3->6 NG"));
			display.display();
		}

		measuredFrequency2 = measureFrequency(MIDIIN2);
		if (measuredFrequency2 > 9000)
		{
			Serial.println("MIDIOUTA measuredFrequency2 is OK !!!");
			//	display.clearDisplay();
			display.setTextSize(2);				 // Normal 1:1 pixel scale
			display.setTextColor(SSD1306_WHITE); // Draw white text
			display.setCursor(0, 17);			 // Start at top-left corner
			display.println(F("J3->J5 OK"));
			display.display();
		}
		else
		{

			Serial.println("MIDIOUTA measuredFrequency2 is Fail !!!");
			//	display.clearDisplay();
			display.setTextSize(2);				 // Normal 1:1 pixel scale
			display.setTextColor(SSD1306_WHITE); // Draw white text
			display.setCursor(0, 17);			 // Start at top-left corner
			display.println(F("J3->J5 NG"));
			display.display();
		}

		PWM_Instance[3]->setPWM(MIDIOUTD, 10000, 0);  // 頻率10KHZ duty cycel 0%
		PWM_Instance[2]->setPWM(MIDIOUTC, 10000, 0);  // 頻率10KHZ duty cycel 0%
		PWM_Instance[1]->setPWM(MIDIOUTB, 10000, 0);  // 頻率10KHZ duty cycel 0%
		PWM_Instance[0]->setPWM(MIDIOUTA, 10000, 50); // 頻率10KHZ duty cycel 50%
		float measuredFrequency1 = measureFrequency(MIDIIN1);

		if (measuredFrequency1 > 9000)
		{
			Serial.println("MIDIOUTD measuredFrequency1 is OK !!!");
			//	display.clearDisplay();
			//	display.clearDisplay();
			display.setTextSize(2);				 // Normal 1:1 pixel scale
			display.setTextColor(SSD1306_WHITE); // Draw white text
			display.setCursor(0, 34);			 // Start at top-left corner
			display.println(F("J4->J6 OK"));
			display.display();
		}
		else
		{
			Serial.println("MIDIOUTD measuredFrequency1 is Fail !!!");
			//	display.clearDisplay();
			//	display.clearDisplay();
			display.setTextSize(2);				 // Normal 1:1 pixel scale
			display.setTextColor(SSD1306_WHITE); // Draw white text
			display.setCursor(0, 34);			 // Start at top-left corner
			display.println(F("J4->J6 NG"));
			display.display();
		}

		measuredFrequency2 = measureFrequency(MIDIIN2); // J5
		if (measuredFrequency2 > 9000)
		{
			Serial.println("MIDIOUTC measuredFrequency2 is OK !!!");
			//	display.clearDisplay();
			//	display.clearDisplay();
			display.setTextSize(2);				 // Normal 1:1 pixel scale
			display.setTextColor(SSD1306_WHITE); // Draw white text
			display.setCursor(0, 48);			 // Start at top-left corner
			display.println(F("J4->J5 OK"));
			display.display();
		}
		else
		{

			Serial.println("MIDIOUTC measuredFrequency2 is Fail !!!");
			//	display.clearDisplay();
			//	display.clearDisplay();
			display.setTextSize(2);				 // Normal 1:1 pixel scale
			display.setTextColor(SSD1306_WHITE); // Draw white text
			display.setCursor(0, 49);			 // Start at top-left corner
			display.println(F("J4->5 NG"));
			display.display();
		}

		key_SQN = 0;
		while (!digitalRead(KEY))
			;
		T_CNT1 = T60MS;
		break;
	}
	default:
		break;
	}
}

/*
***************************
***** Main Loop
李進衛2024/11/25 建立
***************************
*/
void loop()
{
	handleLEDBlink(); // 工作燈
	NewCheckTime();	  // 系統時間計算
	keyprocess();
}
