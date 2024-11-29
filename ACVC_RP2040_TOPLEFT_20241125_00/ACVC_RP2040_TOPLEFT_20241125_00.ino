/*
******************************
程式設計：李進衛
開發板： Arduino Mega 2560 / Arduino DUE

日期：2024/11/08
機種： ACVC
基板： TOP LEFT PCB 右主機板
2024/11/07 定義IO port時發現RP2040 ADC不夠用並且IO port也不夠用實在有夠傷腦筋，要使用Arduino Mrga 2560 又發現他是5V系統
2024/11/10 Arduino DUE 到貨
2024/11/11 將請啊學改為Arduino DUE 雖然電壓不至於有問題但是不必要冒險
2024/11/10 在增加OLED 顯示Bar的演算法
2024/11/11 李進衛更改為Arduino DUE
2024/11/15 增加OLED顯示測試360度VR
Arduino DUE 工作頻率 83MHZ
2024/11/18 更改為RP2040因為Arduino DUE 和MEGA2560 與LED 驅動IC的動作很奇怪聯絡原廠的FAE給的source code參數也是跟我的程式是一樣的！！ 無言
2024/11/18 改為RP2040 LED/OLED測試OK！！
2024/11/22 發現到如果使用Arduino IDE 2.3.3 對於RP2040_PWM.h無法compiler 也無法產生PWM信號因此改為Arduino IDE 2.2.1
2024/11/22 發明新的觸控演算法

******************************
在矩陣形式的鍵盤掃描電路中，ROW（行）和COL（列）分別代表鍵盤矩陣的行和列。

•  ROW（行）：這些是鍵盤矩陣中的橫向線路。當進行掃描時，行線通常設定為輸出，並依次被拉低以檢測是否有按鍵被按下。

•  COL（列）：這些是鍵盤矩陣中的縱向線路。列線通常設定為輸入，並監測行線拉低時的電平變化，以確定具體哪一個按鍵被按下
   這種行列掃描方式可以有效地檢測到矩陣中每個按鍵的狀態，並且能夠支持多按鍵同時按下的情況。
*/
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_TinyUSB.h> //RP2040使用下載程式的USB 當作UART 傳輸資料
#include <Adafruit_ADS1X15.h>
#include "RP2040_PWM.h"
// #include <RotaryEncoder.h>
// Adafruit_ADS1115 ads;  /* Use this for the 16-bit version */
Adafruit_ADS1115 ads; /* Use this for the 12-bit version */

#define _PWM_LOGLEVEL_ 3
// #include <FreqCountRP2.h>

// 創建PWM實例
RP2040_PWM *PWM_Instance;

// 定義 OLED 尺寸和數量
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
// #include <Adafruit_TinyUSB.h> // 使用Arduino DUE 不用代入這個函數 RP2040使用下載程式的USB 當作UART 傳輸資料
#define OLED_RESET -1		// Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int rows = 7;									 // 橫向掃描線數目
const int cols = 4;									 // 縱向掃描線數目
const int rowPins[rows] = {7, 8, 9, 10, 11, 12, 13}; // 定義行引腳
const int colPins[cols] = {1, 2, 3, 6};				 // 定義列引腳
enum touchNum
{
	vr0,
	vr1,
	vr2,
	vr3,
	vr4,
	vr5,
	vr6,
	vr7,
	vr8,
	vr9,
	vr10,
	vr11,
	vr12,
	vr13,
	vr14,
	vr15
};
enum ModeStatus
{
	ALLLEDON,
	TOUCHTEST,
	VRTEST,
	KEYTEST,
	IDLE
};
enum icNUM
{
	ch0,
	ch1,
	ch2,
	ch3
};

bool keyState[cols][rows] = {false};	 // 按鍵狀態
bool lastKeyState[cols][rows] = {false}; // 上一次按鍵狀態
bool reading = 0;
bool vr9to15 = false; // 讀ADC的判定flag
bool ledMod = false;
unsigned long lastDebounceTime[cols][rows] = {0}; // 上一次去抖動時間
const unsigned long debounceDelay = 20;			  // 去抖動延遲時間
// 定義LED引腳
#define LED_PIN 25 // RP2040
// LED 閃爍變數
unsigned long ledLastToggleTime = 0;
unsigned long lastCheck = 0;
int checkInterval = 10;
int T_CNT1 = 0;
int T_CNT2 = 0;
int T_1HZ = 500/checkInterval;
int T_2HZ = 250/checkInterval;
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
// **設定 SNLED2735
#define SNLED2735_ADDRESS 0x77 // 很GY每一片機板都不一樣要記得使用I2C SCAN 確定ADDRESS 0x74 // **設定 SNLED2735 的 I2C 地址** 左板位址0x74 右板位址：0x75
#define TCH_SEND0 0
#define TCH_SEL0 14
#define TCH_SEL1 15
#define TCH_RET0 16
#define TCH_RET1 17
#define RST_LEDn_L 18 // LED 矩陣掃描IC 的 reset pin
#define TCH_RET2 19
#define TCH_RET3 20
#define MUXSEL0 21
#define MUXSEL1 22
#define MUXSEL2 26
#define CLK (27u)
#define DT (28u)
// #define POTSEL0  A5
// #define POTSEL1  A6
// #define POTSEL2  A7
// #define POTSEL3  A8
#define MAX_VOLTAGE 3.3
#define BAR_MAX_WIDTH SCREEN_WIDTH
#define bar_HEIGHT 13

// #define ALLLEDON 0
// #define TOUCHTEST 1
// #define VRTEST 2
// #define KEYTEST 3
// #define IDLE 4

#define ledon 1
#define ledoff 0
// #define threshold 60
// #define debugPin 0
bool keyonoff[28] = {false};   // 按鍵副程式旗標
bool TouchOK[16] = {false};	   // TouchOK旗標
bool TouchOffOK[16] = {false}; // Touch off OK旗標

bool retflag[4] = {false};		  // touch ret0~ret3 flag
byte MAIN_LEFT_LED_MAP[24] = {0}; // SNLED27351 LED 矩陣映射記憶體陣列開啟共24 byte
int keychecksum = 0;

// int counter = 0;
// int currentStateCLK;
bool lastStateCLK;
int MODE = 0;
int touchchecksum = 0;
int touch_SQN = 0;
unsigned long threshold = 100000; // 設定頻率高於5000Hz時視為觸摸
bool touchState = false;		  // 觸控狀態

int rawValue = 0;
int rawValue1 = 0;

float voltage = 0.0;
float voltage1 = 0.0;
// int currentStateSW;
// bool SWPressed = false;

//*************************************
// ENCODER
//*************************************
// RotaryEncoder *encoder = nullptr;
// encoder->tick(); // just call tick() to check the state

// #define	EncoderA 19
// #define EncoderB 20
// #define FS1 21
// #define FS2 22
//**************************************
//  定義巨集來存取和修改特定LED的位置
// 2024/11/03
//**************************************
/*
分解與解釋
1.	LED_MAP[(index) / 8]：
○	這個部分用於確定要修改 LED_MAP 中的哪一個 byte。因為每個 byte 有 8 個 bit，所以 index / 8 計算出 LED_MAP 中的第幾個 byte 需要被修改。
2.	(index) % 8：
○	這個部分計算出 index 在該 byte 中的具體 bit 位置。例如，index = 10 時，10 % 8 = 2，表示 LED_MAP[1] 中的第 2 個 bit。
3.	(1 << ((index) % 8))：
○	這段程式用來建立一個「位遮罩」，將 1 左移至指定的 bit 位置。例如，如果 index % 8 = 2，則 1 << 2 產生 00000100，用來操作 LED_MAP 中的第 2 個 bit。
4.	| (位或運算)：
○	這是一個位元運算，用來將指定的 bit 設為 1。LED_MAP[(index) / 8] | (1 << ((index) % 8)) 將 LED_MAP 中對應位置的 bit 設為 1（即開燈）。
5.	& ~ (位與運算和位反轉)：
○	~ 是位元反轉運算符，會將所有位元取反。例如，如果 1 << 2 是 00000100，那麼 ~(1 << 2) 是 11111011。
○	& 是位與運算，用來將指定的 bit 清為 0。LED_MAP[(index) / 8] & ~(1 << ((index) % 8)) 會將對應位置的 bit 設為 0（即關燈）。
6.	(state) ? ... : ... (條件運算子)：
○	這是一個三元條件運算子，根據 state 的值決定運算邏輯：
	如果 state 為 1，運算式為 LED_MAP[(index) / 8] | (1 << ((index) % 8))，將 bit 設為 1（開燈）。
	如果 state 為 0，運算式為 LED_MAP[(index) / 8] & ~(1 << ((index) % 8))，將 bit 設為 0（關燈）。
程式運作流程
1.	SET_LED(index, 1)：設定 index 對應的 LED 開啟。
○	計算 LED_MAP 中的對應 byte。
○	計算在該 byte 中的 bit 位置。
○	將對應的 bit 設為 1。
2.	SET_LED(index, 0)：設定 index 對應的 LED 關閉。
○	計算 LED_MAP 中的對應 byte。
○	計算在該 byte 中的 bit 位置。
○	將對應的 bit 設為 0。
這個巨集能夠高效地在 LED_MAP 中設定或清除單個 LED 的狀態。
使用範例：
// 定義LED_MAP陣列
byte LED_MAP[24]; // 總共對應192顆LED

// 定義LED名稱
#define led0 0
#define led1 1
#define led2 2
// ... 一直到需要的LED
#define led191 191

// 定義巨集來存取和修改特定LED的位置
#define SET_LED(index, state) (MAIN_LEFT_LED_MAP[(index) / 8] = (state) ? (MAIN_LEFT_LED_MAP[(index) / 8] | (1 << ((index) % 8))) : (MAIN_LEFT_LED_MAP[(index) / 8] & ~(1 << ((index) % 8))))
#define GET_LED(index) ((MAIN_LEFT_LED_MAP[(index) / 8] >> ((index) % 8)) & 1)

void setup() {
	Serial.begin(9600);

	// 設定led0為1 (點亮)
	SET_LED(led0, 1);
	Serial.println(GET_LED(led0)); // 應該輸出1

	// 設定led1為0 (熄滅)
	SET_LED(led1, 0);
	Serial.println(GET_LED(led1)); // 應該輸出0

	// 設定led5為1
	SET_LED(led5, 1);
	Serial.println(GET_LED(led5)); // 應該輸出1
}

void loop() {
	// 可在此放置其他需要操作LED的程式碼
}



*/
// 李進衛2024/10/27 星期日建立這個巨集
// 讓後繼者可以看PCB 背文輸入LED位置不用再使用結構方式定義bit
// #define SET_LED(index, state) (MAIN_LEFT_LED_MAP[(index) / 8] = (state) ? (MAIN_LEFT_LED_MAP[(index) / 8] | (1 << ((index) % 8))) : (MAIN_LEFT_LED_MAP[(index) / 8] & ~(1 << ((index) % 8))))
#define SET_LED(index, state) (MAIN_LEFT_LED_MAP[(index) / 8] = (state) ? (MAIN_LEFT_LED_MAP[(index) / 8] | (1 << ((index) % 8))) : (MAIN_LEFT_LED_MAP[(index) / 8] & ~(1 << ((index) % 8))))
#define GET_LED(index) ((MAIN_LEFT_LED_MAP[(index) / 8] >> ((index) % 8)) & 1)
// #define GET_LED(index) ((MAIN_LEFT_LED_MAP[(index) / 8] >> ((index) % 8)) & 1)
//  LED 定義
// CB1
#define LED180B 0
#define LED186B 1
#define LED192B 2
#define LED198B 3
#define LED204B 4
#define LED210B 5
#define LED215B 6
#define LED219B 7
#define LED225B 8
#define LED231B 9
#define LED237B 10
#define LED243B 11
#define LED248B 12
#define LED253B 13
#define LED258B 14
#define LED263B 15

// CB2
#define LED180G 16
#define LED186G 17
#define LED192G 18
#define LED198G 19
#define LED204G 20
#define LED210G 21
#define LED215G 22
#define LED219G 23
#define LED225G 24
#define LED231G 25
#define LED237G 26
#define LED243G 27
#define LED248G 28
#define LED253G 29
#define LED258G 30
#define LED263G 31

// CB3
#define LED180R 32
#define LED186R 33
#define LED192R 34
#define LED198R 35
#define LED204R 36
#define LED210R 37
#define LED215R 38
#define LED219R 39
#define LED225R 40
#define LED231R 41
#define LED237R 42
#define LED243R 43
#define LED248R 44
#define LED253R 45
#define LED258R 46
#define LED263R 47

// CB4
#define LED395B 48
#define LED396B 49
#define LED397B 50
#define LED398B 51
#define LED399B 52
#define LED400B 53
#define LED401B 54
#define LED402B 55
#define LED403B 56
#define LED404B 57
#define LED405B 58
#define LED406B 59
#define LED407B 60
#define LED408B 61
#define LED409B 62
#define LED410B 63

// CB5
#define LED395G 64
#define LED396G 65
#define LED397G 66
#define LED398G 67
#define LED399G 68
#define LED400G 69
#define LED401G 70
#define LED402G 71
#define LED403G 72
#define LED404G 73
#define LED405G 74
#define LED406G 75
#define LED407G 76
#define LED408G 77
#define LED409G 78
#define LED410G 79

// CB6
#define LED395R 80
#define LED396R 81
#define LED397R 82
#define LED398R 83
#define LED399R 84
#define LED400R 85
#define LED401R 86
#define LED402R 87
#define LED403R 88
#define LED404R 89
#define LED405R 90
#define LED406R 91
#define LED407R 92
#define LED408R 93
#define LED409R 94
#define LED410R 95

// CB7
#define LED86 96
#define LED92 97
#define LED98 98
#define LED103 99
#define LED107 100
#define LED112 101
#define LED117 102
#define LED122 103
#define LED128 104
#define LED135 105
#define LED142 106
#define LED148 107
#define LED153 108
#define LED160 109
#define LED167 110
#define LED174 111

// CB8
#define LED152 112
#define LED159 113
#define LED166 114
#define LED173 115
#define LED179 116
#define LED185 117
#define LED191 118
#define LED197 119
#define LED203 120
#define LED209 121
#define LED236 122
#define LED242 123
#define LED247 124
#define LED252 125
#define LED257 126
#define LED262 127

// CB9
// #define LED	128
// #define LED	129
// #define LED	130
// #define LED	131
// #define LED	132
// #define LED	133
#define LED2 134
#define LED5 135
#define LED8 136
#define LED11 137
#define LED14 138
#define LED17 139
#define LED20 140
#define LED81 141
#define LED82 142
#define LED264 143

// CB10
// #define LED	144
// #define LED	145
// #define LED	146
#define LED3 147
#define LED6 148
#define LED9 149
#define LED12 150
#define LED15 151
#define LED18 152
#define LED21 153
#define LED23 154
#define LED25 155
#define LED27 156
#define LED29 157
#define LED31 158
#define LED32 159

// CB11
// #define LED	160
// #define LED	161
#define LED1 162
#define LED4 163
#define LED7 164
#define LED10 165
#define LED13 166
#define LED16 167
#define LED19 168
#define LED22 169
#define LED24 170
#define LED26 171
#define LED28 172
#define LED30 173
#define LED336 174
#define LED337 175
//************************
//**** 計時定義
//************************
#define T10MS 10 / checkInterval
#define T20MS 20 / checkInterval
#define T30MS 30 / checkInterval
#define T40MS 40 / checkInterval
#define T50MS 50 / checkInterval
#define T60MS 60 / checkInterval
#define T70MS 70 / checkInterval
#define T80MS 80 / checkInterval
#define T90MS 90 / checkInterval
#define T100MS 100 / checkInterval
#define T200MS 200 / checkInterval
#define T300MS 300 / checkInterval
#define T400MS 400 / checkInterval
#define T500MS 500 / checkInterval
#define T1SEC 1000 / checkInterval
#define T2SEC 2000 / checkInterval
#define T3SEC 3000 / checkInterval
#define T5SEC 5000 / checkInterval
#define T1MIN (1000 * 60) / checkInterval

// 濾波設定
const int filterSize = 10; // 10;    // 移動平均濾波的樣本數[參數越大放開時越慢OFF]
float frequencyBuffer[filterSize] = {0};
int bufferIndex = 0;
float filteredFrequency = 0;

// 頻率判定閾值
const float touchThreshold = 50.0; // 頻率增加50%閾值
const float baseFrequency = 500.0; // PWM基準頻率

// 觸控狀態
bool isTouching = false;

// 測量頻率函數
float measureFrequency(int pin)
{
	unsigned long startTime = micros();
	unsigned long highTime = pulseIn(pin, HIGH);
	unsigned long lowTime = pulseIn(pin, LOW);
	unsigned long totalTime = highTime + lowTime;

	if (totalTime == 0)
		return 0;				  // 避免除以零
	return 1000000.0 / totalTime; // 頻率 = 1 / 週期
}

// 更新濾波緩衝區
void updateFrequencyBuffer(float newFrequency)
{
	frequencyBuffer[bufferIndex] = newFrequency;
	bufferIndex = (bufferIndex + 1) % filterSize; // 循環緩衝
}

// 計算移動平均頻率
float calculateFilteredFrequency()
{
	float sum = 0;
	for (int i = 0; i < filterSize; i++)
	{
		sum += frequencyBuffer[i];
	}
	return sum / filterSize;
}

//**************************
// 選擇要控制的 4051 多工器控制3bits
//**************************
void selectVR(int vrIndex)
{

	digitalWrite(MUXSEL0, vrIndex & 0x01);
	digitalWrite(MUXSEL1, (vrIndex >> 1) & 0x01);
	digitalWrite(MUXSEL2, (vrIndex >> 2) & 0x01);
}

//*******************************
// 選擇要控制的 74LVC139 多工器 控制2bits
//*******************************
void selectTOUCH(int touchIndex)
{

	digitalWrite(TCH_SEL0, touchIndex & 0x01);
	digitalWrite(TCH_SEL1, (touchIndex >> 1) & 0x01);
}

void I2C_test(void)
{
	byte error, address;
	int nDevices;

	//  SerialUSB.println("Scanning...");
	Serial.println("Scanning...");

	nDevices = 0;
	for (address = 1; address < 127; address++)
	{

		Wire.beginTransmission(address);
		error = Wire.endTransmission();

		if (error == 0)
		{
			Serial.print("I2C device found at address 0x");
			if (address < 16)
				Serial.print("0");
			Serial.print(address, HEX);
			Serial.println("  !");

			nDevices++;
		}
		else if (error == 4)
		{
			Serial.print("Unknow error at address 0x");
			if (address < 16)
				Serial.print("0");
			Serial.println(address, HEX);
		}
	}
	if (nDevices == 0)
		Serial.println("No I2C devices found\n");
	else
		Serial.println("done\n");

	//  delay(1000);           // wait 5 seconds for next scan
}

//************************************************
// I2C 寫入多個位元組資料的函數，使用陣列作為輸入
// 程式設計：李進衛
// 日期： 2024-11-04
// 功能： 將每一個LED的狀態傳入SNLED27351 中
//************************************************
void I2C_W_LED_UPDATE(byte chip_addr, byte start_reg_addr, byte end_reg_addr, byte data_array[])
{
	Wire.beginTransmission(chip_addr);
	Wire.write(start_reg_addr);
	// 假設 start_reg_addr 到 end_reg_addr 的範圍內共 (end_reg_addr - start_reg_addr + 1) 個位置
	for (byte i = 0; i <= (end_reg_addr - start_reg_addr); i++)
	{
		Wire.write(data_array[i]); // 從 data_array 中寫入資料
	}
	byte Err3 = Wire.endTransmission();
	if (Err3 != 0)
	{
		Serial.print("Device at address 0x");
		Serial.print(chip_addr, HEX);
		Serial.println(" is not responding.");
	}
}

// I2C 寫入兩個位元組資料的函數
void I2C_W_2BYTE(byte chip_addr, byte reg_addr, byte data)
{
	Wire.beginTransmission(chip_addr);
	Wire.write(reg_addr);
	Wire.write(data);
	byte Err = Wire.endTransmission();
	if (Err != 0)
	{
		Serial.print("Device at address 0x");
		Serial.print(chip_addr, HEX);
		Serial.println(" is not responding.");
		//    Serial.println("Send data Error");
	}
}

// I2C 寫入多個位元組資料的函數
void I2C_W_NBYTE(byte chip_addr, byte start_reg_addr, byte end_reg_addr, byte data)
{
	Wire.beginTransmission(chip_addr);
	Wire.write(start_reg_addr);
	for (byte i = start_reg_addr; i <= end_reg_addr; i++)
	{
		Wire.write(data);
	}
	byte Err2 = Wire.endTransmission();
	if (Err2 != 0)
	{
		Serial.print("Device at address 0x");
		Serial.print(chip_addr, HEX);
		Serial.println(" is not responding.");
	}
}

// 讀取 LED 暫存器的函數
void readLEDRegister()
{
	I2C_W_2BYTE(SNLED2735_ADDRESS, 0xFD, 0x00); // 選擇 LED 控制頁面 (Page 0)

	Wire.beginTransmission(SNLED2735_ADDRESS);
	Wire.write(0x00);			 // LED 暫存器的起始地址
	Wire.endTransmission(false); // 發送重複起始條件

	Wire.requestFrom(SNLED2735_ADDRESS, 24); // 讀取 24 個位元組的資料
	for (int i = 0; i < 24; i++)
	{
		byte data = Wire.read();
		// 處理讀取到的資料，例如：列印每個 LED 的開路狀態
		Serial.print("LED LED Register ");
		Serial.print(i, HEX);
		Serial.print(": ");
		Serial.println(data, BIN);
	}
}

// 讀取 LED 開路暫存器的函數
void readOpenRegister()
{
	I2C_W_2BYTE(SNLED2735_ADDRESS, 0xFD, 0x00); // 選擇 LED 控制頁面 (Page 0)

	Wire.beginTransmission(SNLED2735_ADDRESS);
	Wire.write(0x18);			 // LED 開路暫存器的起始地址
	Wire.endTransmission(false); // 發送重複起始條件

	Wire.requestFrom(SNLED2735_ADDRESS, 24); // 讀取 24 個位元組的資料
	for (int i = 0; i < 24; i++)
	{
		byte data = Wire.read();
		// 處理讀取到的資料，例如：列印每個 LED 的開路狀態
		Serial.print("LED Open Register ");
		Serial.print(i, HEX);
		Serial.print(": ");
		Serial.println(data, BIN);
	}
}

// 讀取 LED 短路暫存器的函數
void readShortRegister()
{
	I2C_W_2BYTE(SNLED2735_ADDRESS, 0xFD, 0x00); // 選擇 LED 控制頁面 (Page 0)

	Wire.beginTransmission(SNLED2735_ADDRESS);
	Wire.write(0x30);			 // LED 短路暫存器的起始地址
	Wire.endTransmission(false); // 發送重複起始條件// 維持通訊以便接下來讀取

	Wire.requestFrom(SNLED2735_ADDRESS, 24); // 讀取 24 個位元組的資料
	for (int i = 0; i < 24; i++)
	{
		byte data = Wire.read();
		// 處理讀取到的資料，例如：列印每個 LED 的短路狀態
		Serial.print("LED Short Register ");
		Serial.print(i, HEX);
		Serial.print(": ");
		Serial.println(data, BIN);
	}
}
/*
10ms


*/
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
		if (T_1HZ !=0)
		{
			T_1HZ--;
		} else {
			T_1HZ = 500/checkInterval;
			blink1hz ^= 1;
		}

		if (T_2HZ != 0)
		{
			T_2HZ--;
		} else {
			T_2HZ = 250/checkInterval;
			blink2hz ^= 1;
		}	

	}
}

void clear_ledbuff(void)
{
	// Led buff 為 0
	for (int i = 0; i < 24; i++)
	{
		MAIN_LEFT_LED_MAP[i] = 0;
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
		//	Serial.println("ACVR_RP2040_TOPLEFT_20241118_01.ino");
	}
}

void All_ledon(void)
{
	I2C_W_2BYTE(SNLED2735_ADDRESS, 0xFD, 0x03);		  // 選擇功能頁面 (Page 3)
	I2C_W_2BYTE(SNLED2735_ADDRESS, 0x00, 0x01);		  // 設定 LED 驅動器為一般模式//關閉模式
	I2C_W_2BYTE(SNLED2735_ADDRESS, 0xFD, 0x00);		  // 選擇 LED 控制頁面 (Page 0)
	I2C_W_NBYTE(SNLED2735_ADDRESS, 0x00, 0x17, 0xFF); // 清除 LED 控制暫存器 0x00~0x47 的資料
}

void All_ledoff(void)
{
	I2C_W_2BYTE(SNLED2735_ADDRESS, 0xFD, 0x03);		  // 選擇功能頁面 (Page 3)
	I2C_W_2BYTE(SNLED2735_ADDRESS, 0x00, 0x01);		  // 設定 LED 驅動器為一般模式//關閉模式}
	I2C_W_2BYTE(SNLED2735_ADDRESS, 0xFD, 0x00);		  // 選擇 LED 控制頁面 (Page 0)
	I2C_W_NBYTE(SNLED2735_ADDRESS, 0x00, 0x17, 0x00); // 清除 LED 控制暫存器 0x00~0x47 的資料
}

// 定義36個任務函數
void task0()
{

	Serial.println("Task 0 executed LED OPEN TEST!!");
	if (keyonoff[0] == 0)
	{
		Serial.println("Task 0 ON!!");
		SET_LED(LED180R, ledon);
		SET_LED(LED180G, ledon);
		SET_LED(LED180B, ledon);
		SET_LED(LED395R, ledon);
		SET_LED(LED395G, ledon);
		SET_LED(LED395B, ledon);
		SET_LED(LED86, ledon);
		SET_LED(LED92, ledon);
		keyonoff[0] = 1;
		vr9to15 = 0;
		if (MODE == VRTEST)
		{
			selectVR(vr0); //
			MODE = VRTEST;
		}
		if (MODE == ALLLEDON)
		{
			ledMod = true;
		}
	}
	else
	{
		Serial.println("Task 0 OFF!!");
		SET_LED(LED180R, ledoff);
		SET_LED(LED180G, ledoff);
		SET_LED(LED180B, ledoff);
		SET_LED(LED395R, ledoff);
		SET_LED(LED395G, ledoff);
		SET_LED(LED395B, ledoff);
		SET_LED(LED86, ledoff);
		SET_LED(LED92, ledoff);
		keyonoff[0] = 0;
		if (MODE == VRTEST)
		{
			display.clearDisplay();
			display.setTextSize(1);				 // Normal 1:1 pixel scale
			display.setTextColor(SSD1306_WHITE); // Draw white text
			display.setCursor(0, 50);			 // Start at top-left corner
			display.println(F("VR0 OK"));
			display.display();
		}
	}
}
void task1()
{
	Serial.println("Task 1 executed LED SHORT TEST !!");

	if (keyonoff[1] == 0)
	{
		Serial.println("Task 1 ON!!");
		SET_LED(LED186R, ledon);
		SET_LED(LED186G, ledon);
		SET_LED(LED186B, ledon);
		SET_LED(LED396R, ledon);
		SET_LED(LED396G, ledon);
		SET_LED(LED396B, ledon);
		SET_LED(LED98, ledon);
		SET_LED(LED103, ledon);
		keyonoff[1] = 1;
		vr9to15 = 0;
		if (MODE == VRTEST)
		{
			selectVR(vr1); //
			MODE = VRTEST;
		}
	}
	else
	{
		Serial.println("Task 1 OFF!!");
		SET_LED(LED186R, ledoff);
		SET_LED(LED186G, ledoff);
		SET_LED(LED186B, ledoff);
		SET_LED(LED396R, ledoff);
		SET_LED(LED396G, ledoff);
		SET_LED(LED396B, ledoff);
		SET_LED(LED98, ledoff);
		SET_LED(LED103, ledoff);
		keyonoff[1] = 0;
		if (MODE == VRTEST)
		{
			display.clearDisplay();
			display.setTextSize(1);				 // Normal 1:1 pixel scale
			display.setTextColor(SSD1306_WHITE); // Draw white text
			display.setCursor(0, 50);			 // Start at top-left corner
			display.println(F("VR0 OK"));
			display.display();
		}
	}
}
void task2()
{
	Serial.println("Task 2 executed All LED ON !!");
	if (keyonoff[2] == 0)
	{
		Serial.println("Task 2 ON!!");
		SET_LED(LED192R, ledon);
		SET_LED(LED192G, ledon);
		SET_LED(LED192B, ledon);
		SET_LED(LED397R, ledon);
		SET_LED(LED397G, ledon);
		SET_LED(LED397B, ledon);
		SET_LED(LED107, ledon);
		SET_LED(LED112, ledon);
		keyonoff[2] = 1;
		vr9to15 = 0;
		if (MODE == VRTEST)
		{
			selectVR(vr2); //
			MODE = VRTEST;
		}
	}
	else
	{
		Serial.println("Task 2 OFF!!");
		SET_LED(LED192R, ledoff);
		SET_LED(LED192G, ledoff);
		SET_LED(LED192B, ledoff);
		SET_LED(LED397R, ledoff);
		SET_LED(LED397G, ledoff);
		SET_LED(LED397B, ledoff);
		SET_LED(LED107, ledoff);
		SET_LED(LED112, ledoff);
		keyonoff[2] = 0;
		if (MODE == VRTEST)
		{
			display.clearDisplay();
			display.setTextSize(1);				 // Normal 1:1 pixel scale
			display.setTextColor(SSD1306_WHITE); // Draw white text
			display.setCursor(0, 50);			 // Start at top-left corner
			display.println(F("VR0 OK"));
			display.display();
			//			MODE = IDLE;
		}
	}
}
void task3()
{
	Serial.println("Task 3 executed All LED OFF !!");
	if (keyonoff[3] == 0)
	{
		SET_LED(LED198R, ledon);
		SET_LED(LED198G, ledon);
		SET_LED(LED198B, ledon);
		SET_LED(LED398R, ledon);
		SET_LED(LED398G, ledon);
		SET_LED(LED398B, ledon);
		SET_LED(LED117, ledon);
		SET_LED(LED122, ledon);
		keyonoff[3] = 1;

		vr9to15 = 0;
		if (MODE == VRTEST)
		{
			selectVR(vr3); //
			MODE = VRTEST;
		}
	}
	else
	{
		SET_LED(LED198R, ledoff);
		SET_LED(LED198G, ledoff);
		SET_LED(LED198B, ledoff);
		SET_LED(LED398R, ledoff);
		SET_LED(LED398G, ledoff);
		SET_LED(LED398B, ledoff);
		SET_LED(LED117, ledoff);
		SET_LED(LED122, ledoff);
		keyonoff[3] = 0;
	}
}
void task4()
{
	Serial.println("Task 4 executed I2C test !! ");
	if (keyonoff[4] == 0)
	{
		SET_LED(LED204R, ledon);
		SET_LED(LED204G, ledon);
		SET_LED(LED204B, ledon);
		SET_LED(LED399R, ledon);
		SET_LED(LED399G, ledon);
		SET_LED(LED399B, ledon);
		SET_LED(LED128, ledon);
		SET_LED(LED135, ledon);
		keyonoff[4] = 1;
		vr9to15 = 0;
		if (MODE == VRTEST)
		{
			selectVR(vr4); //
			MODE = VRTEST;
		}
	}
	else
	{
		SET_LED(LED204R, ledoff);
		SET_LED(LED204G, ledoff);
		SET_LED(LED204B, ledoff);
		SET_LED(LED399R, ledoff);
		SET_LED(LED399G, ledoff);
		SET_LED(LED399B, ledoff);
		SET_LED(LED128, ledoff);
		SET_LED(LED135, ledoff);
		keyonoff[4] = 0;
	}
}
void task5()
{
	Serial.println("Task 5 executed");
	if (keyonoff[5] == 0)
	{
		SET_LED(LED225R, ledon);
		SET_LED(LED225G, ledon);
		SET_LED(LED225B, ledon);
		SET_LED(LED403R, ledon);
		SET_LED(LED403G, ledon);
		SET_LED(LED403B, ledon);
		SET_LED(LED152, ledon);
		SET_LED(LED159, ledon);
		keyonoff[5] = 1;
		vr9to15 = 1;
		if (MODE == VRTEST)
		{
			selectVR(vr0); //
			MODE = VRTEST;
		}
	}
	else
	{
		SET_LED(LED225R, ledoff);
		SET_LED(LED225G, ledoff);
		SET_LED(LED225B, ledoff);
		SET_LED(LED403R, ledoff);
		SET_LED(LED403G, ledoff);
		SET_LED(LED403B, ledoff);
		SET_LED(LED152, ledoff);
		SET_LED(LED159, ledoff);
		keyonoff[5] = 0;
	}
}

void task6()
{
	Serial.println("Task 6 executed");
	if (keyonoff[6] == 0)
	{
		SET_LED(LED248R, ledon);
		SET_LED(LED248G, ledon);
		SET_LED(LED248B, ledon);
		SET_LED(LED407R, ledon);
		SET_LED(LED407G, ledon);
		SET_LED(LED407B, ledon);
		SET_LED(LED203, ledon);
		SET_LED(LED209, ledon);
		keyonoff[6] = 1;
		vr9to15 = 1;
		if (MODE == VRTEST)
		{
			selectVR(vr4); //
			MODE = VRTEST;
		}
	}
	else
	{
		SET_LED(LED248R, ledoff);
		SET_LED(LED248G, ledoff);
		SET_LED(LED248B, ledoff);
		SET_LED(LED407R, ledoff);
		SET_LED(LED407G, ledoff);
		SET_LED(LED407B, ledoff);
		SET_LED(LED203, ledoff);
		SET_LED(LED209, ledoff);
		keyonoff[6] = 0;
	}
}
void task7()
{
	Serial.println("Task 7 executed");
	if (keyonoff[7] == 0)
	{
		SET_LED(LED8, ledon);
		SET_LED(LED22, ledon);
		//		All_ledon();
		keyonoff[7] = 1;
	}
	else
	{
		SET_LED(LED8, ledoff);
		SET_LED(LED22, ledoff);
		//		All_ledoff();
		keyonoff[7] = 0;
	}
}
void task8()
{
	Serial.println("Task 8 executed");
	if (keyonoff[8] == 0)
	{
		//		SET_LED(LED120, ledon);
		//		SET_LED(LED121, ledon);
		//		All_ledon();
		keyonoff[8] = 1;
	}
	else
	{
		//		SET_LED(LED120, ledoff);
		//		SET_LED(LED121, ledoff);
		//		All_ledoff();
		keyonoff[8] = 0;
	}
}
void task9()
{
	Serial.println("Task 9 executed");
	if (keyonoff[9] == 0)
	{
		SET_LED(LED19, ledon);
		//		SET_LED(LED113, ledon);
		//		All_ledon();
		keyonoff[9] = 1;
	}
	else
	{
		SET_LED(LED19, ledoff);
		keyonoff[9] = 0;
	}
}
void task10()
{
	Serial.println("Task 10 executed");
	if (keyonoff[10] == 0)
	{
		SET_LED(LED15, ledon);
		keyonoff[10] = 1;
	}
	else
	{
		SET_LED(LED15, ledoff);
		keyonoff[10] = 0;
	}
}
void task11()
{
	Serial.println("Task 11 executed");
	if (keyonoff[11] == 0)
	{
		SET_LED(LED210R, ledon);
		SET_LED(LED210G, ledon);
		SET_LED(LED210B, ledon);
		SET_LED(LED400R, ledon);
		SET_LED(LED400G, ledon);
		SET_LED(LED400B, ledon);
		SET_LED(LED142, ledon);
		SET_LED(LED148, ledon);
		keyonoff[11] = 1;
		vr9to15 = 0;
		if (MODE == VRTEST)
		{
			selectVR(vr5); //
			MODE = VRTEST;
		}
	}
	else
	{
		SET_LED(LED210R, ledoff);
		SET_LED(LED210G, ledoff);
		SET_LED(LED210B, ledoff);
		SET_LED(LED400R, ledoff);
		SET_LED(LED400G, ledoff);
		SET_LED(LED400B, ledoff);
		SET_LED(LED142, ledoff);
		SET_LED(LED148, ledoff);
		keyonoff[11] = 0;
	}
}
void task12()
{ // 空接
	Serial.println("Task 12 executed");
	if (keyonoff[12] == 0)
	{

		SET_LED(LED231R, ledon);
		SET_LED(LED231G, ledon);
		SET_LED(LED231B, ledon);
		SET_LED(LED404R, ledon);
		SET_LED(LED404G, ledon);
		SET_LED(LED404B, ledon);
		SET_LED(LED166, ledon);
		SET_LED(LED173, ledon);
		keyonoff[12] = 1;
		vr9to15 = 1;
		if (MODE == VRTEST)
		{
			selectVR(vr1); //
			MODE = VRTEST;
		}
	}
	else
	{

		SET_LED(LED231R, ledoff);
		SET_LED(LED231G, ledoff);
		SET_LED(LED231B, ledoff);
		SET_LED(LED404R, ledoff);
		SET_LED(LED404G, ledoff);
		SET_LED(LED404B, ledoff);
		SET_LED(LED166, ledoff);
		SET_LED(LED173, ledoff);
		keyonoff[12] = 0;
	}
}
void task13()
{
	Serial.println("Task 13 executed");
	if (keyonoff[13] == 0)
	{
		SET_LED(LED253R, ledon);
		SET_LED(LED253G, ledon);
		SET_LED(LED253B, ledon);
		SET_LED(LED408R, ledon);
		SET_LED(LED408G, ledon);
		SET_LED(LED408B, ledon);
		SET_LED(LED236, ledon);
		SET_LED(LED242, ledon);
		keyonoff[13] = 1;
		vr9to15 = 1;
		if (MODE == VRTEST)
		{
			selectVR(vr5); //
			MODE = VRTEST;
		}
	}
	else
	{
		SET_LED(LED253R, ledoff);
		SET_LED(LED253G, ledoff);
		SET_LED(LED253B, ledoff);
		SET_LED(LED408R, ledoff);
		SET_LED(LED408G, ledoff);
		SET_LED(LED408B, ledoff);
		SET_LED(LED236, ledoff); //
		SET_LED(LED242, ledoff); //
		keyonoff[13] = 0;
	}
}
void task14()
{
	Serial.println("Task 14 executed");
	if (keyonoff[14] == 0)
	{
		SET_LED(LED82, ledon);
		keyonoff[14] = 1;
	}
	else
	{
		SET_LED(LED82, ledoff);
		keyonoff[14] = 0;
	}
}
void task15()
{
	Serial.println("Task 15 executed");
	if (keyonoff[15] == 0)
	{
		SET_LED(LED264, ledon);
		//		SET_LED(LED213, ledon);
		//		SET_LED(LED125, ledon);
		//		All_ledon();
		keyonoff[15] = 1;
	}
	else
	{
		SET_LED(LED264, ledoff);
		//		SET_LED(LED213, ledoff);
		//		SET_LED(LED125, ledoff);
		//		All_ledoff();
		keyonoff[15] = 0;
	}
}
void task16()
{
	Serial.println("Task 16 executed");
	if (keyonoff[16] == 0)
	{
		SET_LED(LED13, ledon);
		SET_LED(LED16, ledon);
		//		All_ledon();
		keyonoff[16] = 1;
	}
	else
	{
		SET_LED(LED13, ledoff);
		SET_LED(LED16, ledoff);
		//		All_ledoff();
		keyonoff[16] = 0;
	}
}
void task17()
{
	Serial.println("Task 17 executed");
	if (keyonoff[17] == 0)
	{
		SET_LED(LED2, ledon);
		SET_LED(LED5, ledon);
		keyonoff[17] = 1;
	}
	else
	{
		SET_LED(LED2, ledoff);
		SET_LED(LED5, ledoff);
		keyonoff[17] = 0;
	}
}
void task18()
{
	Serial.println("Task 18 executed");
	if (keyonoff[18] == 0)
	{
		SET_LED(LED215R, ledon);
		SET_LED(LED215G, ledon);
		SET_LED(LED215B, ledon);
		SET_LED(LED401R, ledon);
		SET_LED(LED401G, ledon);
		SET_LED(LED401B, ledon);
		SET_LED(LED153, ledon);
		SET_LED(LED160, ledon);
		keyonoff[18] = 1;
		vr9to15 = 0;
		if (MODE == VRTEST)
		{
			selectVR(vr6); //
			MODE = VRTEST;
		}
	}
	else
	{
		SET_LED(LED215R, ledoff);
		SET_LED(LED215G, ledoff);
		SET_LED(LED215B, ledoff);
		SET_LED(LED401R, ledoff);
		SET_LED(LED401G, ledoff);
		SET_LED(LED401B, ledoff);
		SET_LED(LED153, ledoff);
		SET_LED(LED160, ledoff);
		keyonoff[18] = 0;
		vr9to15 = 1;
		if (MODE == VRTEST)
		{
			selectVR(vr2); //
			MODE = VRTEST;
		}
	}
}
void task19()
{
	Serial.println("Task 19 executed");
	if (keyonoff[19] == 0)
	{
		SET_LED(LED237R, ledon);
		SET_LED(LED237G, ledon);
		SET_LED(LED237B, ledon);
		SET_LED(LED405R, ledon);
		SET_LED(LED405G, ledon);
		SET_LED(LED405B, ledon);
		SET_LED(LED179, ledon);
		SET_LED(LED185, ledon);
		keyonoff[19] = 1;
		vr9to15 = 1;
		if (MODE == VRTEST)
		{
			selectVR(vr2); //
			MODE = VRTEST;
		}
	}
	else
	{
		SET_LED(LED237R, ledoff);
		SET_LED(LED237G, ledoff);
		SET_LED(LED237B, ledoff);
		SET_LED(LED405R, ledoff);
		SET_LED(LED405G, ledoff);
		SET_LED(LED405B, ledoff);
		SET_LED(LED179, ledoff);
		SET_LED(LED185, ledoff);
		keyonoff[19] = 0;
	}
}
void task20()
{
	Serial.println("Task 20 executed");
	if (keyonoff[20] == 0)
	{
		SET_LED(LED258R, ledon);
		SET_LED(LED258G, ledon);
		SET_LED(LED258B, ledon);
		SET_LED(LED409R, ledon);
		SET_LED(LED409G, ledon);
		SET_LED(LED409B, ledon);
		SET_LED(LED247, ledon);
		SET_LED(LED252, ledon);
		//		All_ledon();
		keyonoff[20] = 1;
		vr9to15 = 1;
		if (MODE == VRTEST)
		{
			selectVR(vr6); //
			MODE = VRTEST;
		}
	}
	else
	{
		SET_LED(LED258R, ledoff);
		SET_LED(LED258G, ledoff);
		SET_LED(LED258B, ledoff);
		SET_LED(LED409R, ledoff);
		SET_LED(LED409G, ledoff);
		SET_LED(LED409B, ledoff);
		SET_LED(LED247, ledoff);
		SET_LED(LED252, ledoff);
		//		All_ledoff();
		keyonoff[20] = 0;
	}
}
void task21()
{
	Serial.println("Task 21 executed");
	if (keyonoff[21] == 0)
	{
		SET_LED(LED17, ledon);
		//		SET_LED(LED119, ledon);
		//		All_ledon();
		keyonoff[21] = 1;
	}
	else
	{
		SET_LED(LED17, ledoff);
		//		SET_LED(LED119, ledoff);
		//		All_ledoff();
		keyonoff[21] = 0;
	}
}
void task22()
{
	Serial.println("Task 22 executed");
	if (keyonoff[22] == 0)
	{
		SET_LED(LED11, ledon);
		//		SET_LED(LED100, ledon);
		//		SET_LED(LED105, ledon);
		//		All_ledon();
		keyonoff[22] = 1;
	}
	else
	{
		SET_LED(LED11, ledoff);
		//		SET_LED(LED100, ledoff);
		//		SET_LED(LED105, ledoff);
		//		All_ledoff();
		keyonoff[22] = 0;
	}
}
void task23()
{
	Serial.println("Task 23 executed");
	if (keyonoff[23] == 0)
	{
		SET_LED(LED20, ledon);
		SET_LED(LED81, ledon);
		//		SET_LED(LED207, ledon);
		//		All_ledon();
		keyonoff[23] = 1;
	}
	else
	{
		SET_LED(LED20, ledoff);
		SET_LED(LED81, ledoff);
		//		SET_LED(LED207, ledoff);
		//		All_ledoff();
		keyonoff[23] = 0;
	}
}
void task24()
{ // 空接
	Serial.println("Task 24 executed");
	if (keyonoff[24] == 0)
	{
		SET_LED(LED14, ledon);
		//		SET_LED(LED318, ledon);
		//		SET_LED(LED207, ledon);
		//		All_ledon();
		keyonoff[24] = 1;
	}
	else
	{
		SET_LED(LED14, ledoff);
		//		SET_LED(LED318, ledoff);
		//		SET_LED(LED207, ledoff);
		//		All_ledoff();
		keyonoff[24] = 0;
	}
}
void task25()
{
	Serial.println("Task 25 executed");
	if (keyonoff[25] == 0)
	{
		SET_LED(LED219R, ledon);
		SET_LED(LED219G, ledon);
		SET_LED(LED219B, ledon);
		SET_LED(LED402R, ledon);
		SET_LED(LED402G, ledon);
		SET_LED(LED402B, ledon);
		SET_LED(LED167, ledon);
		SET_LED(LED174, ledon);
		keyonoff[25] = 1;
		vr9to15 = 0;
		if (MODE == VRTEST)
		{
			selectVR(vr7); //
			MODE = VRTEST;
		}
	}
	else
	{
		SET_LED(LED219R, ledoff);
		SET_LED(LED219G, ledoff);
		SET_LED(LED219B, ledoff);
		SET_LED(LED402R, ledoff);
		SET_LED(LED402G, ledoff);
		SET_LED(LED402B, ledoff);
		SET_LED(LED167, ledoff);
		SET_LED(LED174, ledoff);
		keyonoff[25] = 0;
	}
}
void task26()
{
	Serial.println("Task 26 executed");
	if (keyonoff[26] == 0)
	{
		SET_LED(LED243R, ledon);
		SET_LED(LED243G, ledon);
		SET_LED(LED243B, ledon);
		SET_LED(LED406R, ledon);
		SET_LED(LED406G, ledon);
		SET_LED(LED406B, ledon);
		SET_LED(LED191, ledon);
		SET_LED(LED197, ledon);
		//		SET_LED(LED233, ledon);
		//		SET_LED(LED155, ledon);
		//		SET_LED(LED347, ledon);
		//		All_ledon();
		keyonoff[26] = 1;
		vr9to15 = 1;
		if (MODE == VRTEST)
		{
			selectVR(vr3); //
			MODE = VRTEST;
		}
	}
	else
	{
		SET_LED(LED243R, ledoff);
		SET_LED(LED243G, ledoff);
		SET_LED(LED243B, ledoff);
		SET_LED(LED406R, ledoff);
		SET_LED(LED406G, ledoff);
		SET_LED(LED406B, ledoff);
		SET_LED(LED191, ledoff);
		SET_LED(LED197, ledoff);
		//		SET_LED(LED233, ledoff);
		//		SET_LED(LED155, ledoff);
		//		SET_LED(LED347, ledoff);
		//		All_ledoff();
		keyonoff[26] = 0;
	}
}
void task27()
{
	Serial.println("Task 27 executed");
	if (keyonoff[27] == 0)
	{
		SET_LED(LED263B, ledon);
		SET_LED(LED263R, ledon);
		SET_LED(LED263G, ledon);

		SET_LED(LED410B, ledon);
		SET_LED(LED410R, ledon);
		SET_LED(LED410G, ledon);
		SET_LED(LED257, ledon);
		SET_LED(LED262, ledon);
		//		All_ledon();
		keyonoff[27] = 1;
		vr9to15 = 1;
		if (MODE == VRTEST)
		{
			selectVR(vr7); //
			MODE = VRTEST;
			
		}
	}
	else
	{
		SET_LED(LED263B, ledoff);
		SET_LED(LED263R, ledoff);
		SET_LED(LED263G, ledoff);
		SET_LED(LED410B, ledoff);
		SET_LED(LED257, ledoff);
		SET_LED(LED410R, ledoff);
		SET_LED(LED410G, ledoff);
		SET_LED(LED262, ledoff);
		//		All_ledoff();
		keyonoff[27] = 0;
		if (MODE == VRTEST)
		{
//			selectVR(vr7); //
			MODE = TOUCHTEST;
			touch_SQN = 0;
		}		
	}
}
void task28()
{
	Serial.println("Task 28 executed");
	if (keyonoff[28] == 0)
	{
		//		SET_LED(LED239, ledon);
		//		SET_LED(LED340, ledon);
		//		SET_LED(LED123, ledon);
		//		SET_LED(LED341, ledon);
		//		All_ledon();
		keyonoff[28] = 1;
	}
	else
	{
		//		SET_LED(LED239, ledoff);
		//		SET_LED(LED340, ledoff);
		//		SET_LED(LED123, ledoff);
		//		SET_LED(LED341, ledoff);
		//		All_ledoff();
		keyonoff[28] = 0;
	}
}
void task29()
{
	Serial.println("Task 29 executed");
	if (keyonoff[29] == 0)
	{
		//		SET_LED(LED245, ledon);
		//		SET_LED(LED350, ledon);
		//		SET_LED(LED88, ledon);
		//		SET_LED(LED93, ledon);
		//		All_ledon();
		keyonoff[29] = 1;
	}
	else
	{
		//		SET_LED(LED245, ledoff);
		//		SET_LED(LED350, ledoff);
		//		SET_LED(LED88, ledoff);
		//		SET_LED(LED93, ledoff);
		//		All_ledoff();
		keyonoff[29] = 0;
	}
}
void task30()
{
	Serial.println("Task 30 executed");
	if (keyonoff[30] == 0)
	{
		//		SET_LED(LED261, ledon);
		//		SET_LED(LED238, ledon);
		//		SET_LED(LED232, ledon);
		//		All_ledon();
		keyonoff[30] = 1;
	}
	else
	{
		//		SET_LED(LED261, ledoff);
		//		SET_LED(LED238, ledoff);
		//		SET_LED(LED232, ledoff);
		//		All_ledoff();
		keyonoff[30] = 0;
	}
}
void task31()
{
	Serial.println("Task 31 executed");
	if (keyonoff[31] == 0)
	{
		//		SET_LED(LED211, ledon);
		//		SET_LED(LED205, ledon);
		//		SET_LED(LED251, ledon);
		//		All_ledon();
		keyonoff[31] = 1;
	}
	else
	{
		//		SET_LED(LED211, ledoff);
		//		SET_LED(LED205, ledoff);
		//		SET_LED(LED251, ledoff);
		//		All_ledoff();
		keyonoff[31] = 0;
	}
}
void task32()
{
	Serial.println("Task 32 executed");
	if (keyonoff[32] == 0)
	{
		//		SET_LED(LED327, ledon);
		//		SET_LED(LED150, ledon);
		//		SET_LED(LED131, ledon);
		//		SET_LED(LED136, ledon);
		//		All_ledon();
		keyonoff[32] = 1;
	}
	else
	{
		//		SET_LED(LED327, ledoff);
		//		SET_LED(LED150, ledoff);
		//		SET_LED(LED131, ledoff);
		//		SET_LED(LED136, ledoff);
		//		All_ledoff();
		keyonoff[32] = 0;
	}
}
void task33()
{
	Serial.println("Task 33 executed");
	if (keyonoff[33] == 0)
	{
		//		SET_LED(LED270, ledon);
		//		SET_LED(LED118, ledon);
		//		SET_LED(LED342, ledon);
		//		All_ledon();
		keyonoff[33] = 1;
	}
	else
	{
		//		SET_LED(LED270, ledoff);
		//		SET_LED(LED118, ledoff);
		//		SET_LED(LED342, ledoff);
		//		All_ledoff();
		keyonoff[33] = 0;
	}
}
void task34()
{
	Serial.println("Task 34 executed");
	if (keyonoff[34] == 0)
	{
		//		SET_LED(LED274, ledon);
		//		SET_LED(LED348, ledon);
		//		All_ledon();
		keyonoff[34] = 1;
	}
	else
	{
		//		SET_LED(LED274, ledoff);
		//		SET_LED(LED348, ledoff);
		//		All_ledoff();
		keyonoff[34] = 0;
	}
}
void task35()
{
	Serial.println("Task 35 executed");
	if (keyonoff[35] == 0)
	{
		//		SET_LED(LED278, ledon);
		//		SET_LED(LED345, ledon);
		//		SET_LED(LED346, ledon);
		//		All_ledon();
		keyonoff[35] = 1;
	}
	else
	{
		//		SET_LED(LED278, ledoff);
		//		SET_LED(LED345, ledoff);
		//		SET_LED(LED346, ledoff);
		//		All_ledoff();
		keyonoff[35] = 0;
	}
}

void task36()
{
	Serial.println("Task 36 executed");
	if (keyonoff[36] == 0)
	{
		//		SET_LED(LED282, ledon);
		//		SET_LED(LED345, ledon);
		//		SET_LED(LED346, ledon);
		//		All_ledon();
		keyonoff[36] = 1;
	}
	else
	{
		//		SET_LED(LED282, ledoff);
		//		SET_LED(LED345, ledoff);
		//		SET_LED(LED346, ledoff);
		//		All_ledoff();
		keyonoff[36] = 0;
	}
}

void task37()
{
	Serial.println("Task 37 executed");
	if (keyonoff[37] == 0)
	{
		//		SET_LED(LED286, ledon);
		//		SET_LED(LED345, ledon);
		//		SET_LED(LED346, ledon);
		//		All_ledon();
		keyonoff[37] = 1;
	}
	else
	{
		//		SET_LED(LED286, ledoff);
		//		SET_LED(LED345, ledoff);
		//		SET_LED(LED346, ledoff);
		//		All_ledoff();
		keyonoff[37] = 0;
	}
}

void task38()
{
	Serial.println("Task 38 executed");
	if (keyonoff[38] == 0)
	{
		//		SET_LED(LED228, ledon);
		//		SET_LED(LED194, ledon);
		//		SET_LED(LED193, ledon);
		keyonoff[38] = 1;
	}
	else
	{
		//		SET_LED(LED228, ledoff);
		//		SET_LED(LED194, ledoff);
		//		SET_LED(LED193, ledoff);
		keyonoff[38] = 0;
	}
}

void task39()
{
	Serial.println("Task 39 executed");
	if (keyonoff[39] == 0)
	{
		//		SET_LED(LED195, ledon);
		//		SET_LED(LED345, ledon);
		//		SET_LED(LED346, ledon);
		keyonoff[39] = 1;
	}
	else
	{
		//		SET_LED(LED195, ledoff);
		//		SET_LED(LED345, ledoff);
		//		SET_LED(LED346, ledoff);
		keyonoff[39] = 0;
	}
}

void task40()
{
	Serial.println("Task 40 executed");
	if (keyonoff[40] == 0)
	{
		//		SET_LED(LED284, ledon);
		//		SET_LED(LED345, ledon);
		//		SET_LED(LED346, ledon);
		keyonoff[40] = 1;
	}
	else
	{
		//		SET_LED(LED284, ledoff);
		//		SET_LED(LED345, ledoff);
		//		SET_LED(LED346, ledoff);
		keyonoff[40] = 0;
	}
}

void task41()
{
	Serial.println("Task 41 executed");
	if (keyonoff[41] == 0)
	{
		Serial.println("Task 41 key on !!");
		//		SET_LED(LED290, ledon);
		//		SET_LED(LED345, ledon);
		//		SET_LED(LED346, ledon);
		//		All_ledon();
		keyonoff[41] = 1;
	}
	else
	{
		Serial.println("Task 41 key off !!");
		//		SET_LED(LED290, ledoff);
		//		SET_LED(LED345, ledoff);
		//		SET_LED(LED346, ledoff);
		//	All_ledoff();
		keyonoff[41] = 0;
	}
}

void task42()
{
	Serial.println("Task 42 executed");
	if (keyonoff[42] == 0)
	{
		Serial.println("Task 42 key on !!");
		//		SET_LED(LED244, ledon);
		//		SET_LED(LED345, ledon);
		//		SET_LED(LED346, ledon);
		keyonoff[42] = 1;
	}
	else
	{
		Serial.println("Task 42 key off !!");
		//		SET_LED(LED244, ledoff);
		//		SET_LED(LED345, ledoff);
		//		SET_LED(LED346, ledoff);
		keyonoff[42] = 0;
	}
}

void task43()
{
	Serial.println("Task 43 executed");
	if (keyonoff[43] == 0)
	{
		Serial.println("Task 43 key on !!");
		//		SET_LED(LED294, ledon);
		//		SET_LED(LED345, ledon);
		//		SET_LED(LED346, ledon);
		//		All_ledon();
		keyonoff[43] = 1;
	}
	else
	{
		Serial.println("Task 42 key off !!");
		//		SET_LED(LED294, ledoff);
		//		SET_LED(LED345, ledoff);
		//		SET_LED(LED346, ledoff);
		//		All_ledoff();
		keyonoff[43] = 0;
	}
}

void task44()
{
	Serial.println("Task 44 executed");
	if (keyonoff[44] == 0)
	{
		Serial.println("Task 44 key on !!");
		//		SET_LED(LED240, ledon);
		//		SET_LED(LED345, ledon);
		//		SET_LED(LED346, ledon);
		keyonoff[44] = 1;
	}
	else
	{
		Serial.println("Task 44 key off !!");
		//		SET_LED(LED240, ledoff);
		//		SET_LED(LED345, ledoff);
		//		SET_LED(LED346, ledoff);
		keyonoff[44] = 0;
	}
}

void task45()
{
	Serial.println("Task 45 executed");
	if (keyonoff[45] == 0)
	{
		Serial.println("Task 45 key on !!");
		//		SET_LED(LED222, ledon);
		//		SET_LED(LED345, ledon);
		//		SET_LED(LED346, ledon);
		keyonoff[45] = 1;
	}
	else
	{
		Serial.println("Task 45 key off !!");
		//		SET_LED(LED222, ledoff);
		//		SET_LED(LED345, ledoff);
		//		SET_LED(LED346, ledoff);
		keyonoff[45] = 0;
	}
}

void task46()
{
	Serial.println("Task 46 executed");
	if (keyonoff[46] == 0)
	{
		Serial.println("Task 46 key on !!");
		//		SET_LED(LED175, ledon);
		//		SET_LED(LED176, ledon);
		//		SET_LED(LED177, ledon);
		keyonoff[46] = 1;
	}
	else
	{
		Serial.println("Task 46 key off !!");
		//		SET_LED(LED175, ledoff);
		//		SET_LED(LED176, ledoff);
		//		SET_LED(LED177, ledoff);
		keyonoff[46] = 0;
	}
}

void task47()
{
	Serial.println("Task 47 executed");
	if (keyonoff[47] == 0)
	{
		Serial.println("Task 47 key on !!");
		//		SET_LED(LED181, ledon);
		//		SET_LED(LED182, ledon);
		//		SET_LED(LED183, ledon);
		keyonoff[47] = 1;
	}
	else
	{
		Serial.println("Task 47 key off !!");
		//		SET_LED(LED181, ledoff);
		//		SET_LED(LED182, ledoff);
		//		SET_LED(LED183, ledoff);
		keyonoff[47] = 0;
	}
}

void touch_task0()
{
	Serial.println("touch 0  !!");
	TouchOK[vr0] ^= true; // 將TouchOK 反向
	if (TouchOK[vr0])
	{
		Serial.println("touch 0 On !!");
		//		touchchecksum ++;
	}
	else
	{

		Serial.println("touch 0 Off !!");
		TouchOffOK[vr0] = true;
		//		touchchecksum ++;
	}
}

void touch_task1()
{

	Serial.println("touch 1  !!");
	TouchOK[vr1] ^= true; // 將TouchOK 反向
	if (TouchOK[vr1])
	{
		Serial.println("touch 1 On !!");
	}
	else
	{

		Serial.println("touch 1 Off !!");
		TouchOffOK[vr1] = true;
	}
}
void touch_task2()
{

	Serial.println("touch 2  !!");
	TouchOK[vr2] ^= true; // 將TouchOK 反向
	if (TouchOK[vr2])
	{
		Serial.println("touch 2 On !!");
	}
	else
	{

		Serial.println("touch 2 Off !!");
		TouchOffOK[vr2] = true;
	}
}
void touch_task3()
{

	Serial.println("touch 3  !!");
	TouchOK[vr3] ^= true; // 將TouchOK 反向
	if (TouchOK[vr3])
	{
		Serial.println("touch 3 On !!");
	}
	else
	{

		Serial.println("touch 3 Off !!");
		TouchOffOK[vr3] = 1;
	}
}
void touch_task4()
{

	Serial.println("touch 4 !!");
	TouchOK[vr4] ^= true; // 將TouchOK 反向
	if (TouchOK[vr4])
	{
		Serial.println("touch 4 On !!");
	}
	else
	{

		Serial.println("touch 0 Off !!");
		TouchOffOK[vr4] = true;
	}
}
void touch_task5()
{

	Serial.println("touch 5 !!");
	TouchOK[vr5] ^= true; // 將TouchOK 反向
	if (TouchOK[vr5])
	{
		Serial.println("touch 5 On !!");
		touchchecksum++;
	}
	else
	{

		Serial.println("touch 5 Off !!");
		TouchOffOK[vr5] = true;
	}
}
void touch_task6()
{

	Serial.println("touch 6 !!");
	TouchOK[vr6] ^= true; // 將TouchOK 反向
	if (TouchOK[vr6])
	{
		Serial.println("touch 6 On !!");
	}
	else
	{

		Serial.println("touch 6 Off !!");
		TouchOffOK[vr6] = true;
	}
}
void touch_task7()
{

	Serial.println("touch 7 !!");
	TouchOK[vr7] ^= true; // 將TouchOK 反向
	if (TouchOK[vr7])
	{
		Serial.println("touch 7 On !!");
	}
	else
	{

		Serial.println("touch 7 Off !!");
		TouchOffOK[vr7] = true;
	}
}
void touch_task8()
{

	Serial.println("touch 8 !!");
	TouchOK[vr8] ^= true; // 將TouchOK 反向
	if (TouchOK[vr8])
	{
		Serial.println("touch 8 On !!");
	}
	else
	{

		Serial.println("touch 8 Off !!");
		TouchOffOK[vr8] = true;
	}
}
void touch_task9()
{

	Serial.println("touch 9 !!");
	TouchOK[9] ^= true; // 將TouchOK 反向
	if (TouchOK[vr9])
	{
		Serial.println("touch 9 On !!");
		//		touchchecksum ++;
	}
	else
	{

		Serial.println("touch 9 Off !!");
		TouchOffOK[vr9] = true;
	}
}
void touch_task10()
{

	Serial.println("touch 10  !!");
	TouchOK[vr10] ^= true; // 將TouchOK 反向
	if (TouchOK[vr10])
	{
		Serial.println("touch 10 On !!");
	}
	else
	{

		Serial.println("touch 10 Off !!");
		TouchOffOK[vr10] = true;
	}
}
void touch_task11()
{

	Serial.println("touch 11 !!");
	TouchOK[vr11] ^= true; // 將TouchOK 反向
	if (TouchOK[vr11])
	{
		Serial.println("touch 11 On !!");
	}
	else
	{

		Serial.println("touch 11 Off !!");
		TouchOffOK[vr11] = true;
	}
}
void touch_task12()
{

	Serial.println("touch 12 !!");
	TouchOK[vr12] ^= true; // 將TouchOK 反向
	if (TouchOK[vr12])
	{
		Serial.println("touch 12 On !!");
	}
	else
	{

		Serial.println("touch 12 Off !!");
		TouchOffOK[vr12] = true;
	}
}
void touch_task13()
{

	Serial.println("touch 13 !!");
	TouchOK[vr13] ^= true; // 將TouchOK 反向
	if (TouchOK[vr13])
	{
		Serial.println("touch 13 On !!");
	}
	else
	{

		Serial.println("touch 13 Off !!");
		TouchOffOK[vr13] = true;
	}
}
void touch_task14()
{

	Serial.println("touch 14 !!");
	TouchOK[vr14] ^= true; // 將TouchOK 反向
	if (TouchOK[vr14])
	{
		Serial.println("touch 14 On !!");
	}
	else
	{

		Serial.println("touch 14 Off !!");
		TouchOffOK[vr14] = true;
	}
}
void touch_task15()
{

	Serial.println("touch 15 !!");
	TouchOK[vr15] ^= true; // 將TouchOK 反向
	if (TouchOK[vr15])
	{
		Serial.println("touch 15 On !!");
	}
	else
	{

		Serial.println("touch 15 Off !!");
		TouchOffOK[vr15] = true;
	}
}

void Encode_process(void)
{

	// 快速檢測CLK和DT的狀態
	bool currentStateCLK = digitalRead(CLK);
	bool currentStateDT = digitalRead(DT);

	// 判斷方向
	if (currentStateCLK != lastStateCLK)
	{
		if (currentStateDT != currentStateCLK)
		{
			counter--;
		}
		else
		{
			counter++;
		}

		// 限制計數範圍（如果需要）
		counter = (counter + 160) % 160;
	}

	// 更新上一個CLK狀態
	lastStateCLK = currentStateCLK;

	/*
		currentStateCLK = digitalRead(CLK);
		delayMicroseconds(500);
	  if (currentStateCLK != lastStateCLK && currentStateCLK == HIGH) {
		if (digitalRead(DT) != currentStateCLK) {
		  counter--;
		} else {
		  counter++;
		}
		counter = (counter + 160) % 160;
		Serial.print("Position: ");
		Serial.println(counter);
	  }


	  lastStateCLK = currentStateCLK;
	 */
	switch (counter)
	{
	case 0:
		clear_ledbuff();
		SET_LED(LED31, ledon);
		break;
	case 1:
		clear_ledbuff();
		SET_LED(LED27, ledon);
		break;
	case 2:
		clear_ledbuff();
		SET_LED(LED23, ledon);
		break;
	case 3:
		clear_ledbuff();
		SET_LED(LED18, ledon); // 3
		break;
	case 4:
		clear_ledbuff();
		SET_LED(LED9, ledon); // 4
		break;
	case 5:
		clear_ledbuff();
		SET_LED(LED3, ledon); // 5
		break;
	case 6:
		clear_ledbuff();
		SET_LED(LED7, ledon); // 6
		break;
	case 7:
		clear_ledbuff();
		SET_LED(LED1, ledon); // 7
		break;
	case 8:
		clear_ledbuff();
		SET_LED(LED32, ledon); // 8
		break;
	case 9:
		clear_ledbuff();
		SET_LED(LED29, ledon); // 9
		break;
	case 10:
		clear_ledbuff();
		SET_LED(LED25, ledon); // 10
		break;
	case 11:
		clear_ledbuff();
		SET_LED(LED21, ledon); // 11
		break;
	case 12:
		clear_ledbuff();
		SET_LED(LED12, ledon); // 12
		break;
	case 13:
		clear_ledbuff();
		SET_LED(LED6, ledon); // 13
		break;
	case 14:
		clear_ledbuff();
		SET_LED(LED10, ledon); // 14
		break;
	case 15:
		clear_ledbuff();
		SET_LED(LED4, ledon); // 15
		break;
	case 16:

		clear_ledbuff();
		SET_LED(LED2, ledon); //
		break;
	case 17:
		clear_ledbuff();
		SET_LED(LED5, ledon); //
		break;
	case 18:
		clear_ledbuff();
		SET_LED(LED13, ledon); //
		break;
	case 19:
		clear_ledbuff();
		SET_LED(LED16, ledon); //
		break;
	case 20:
		clear_ledbuff();
		SET_LED(LED15, ledon); //
		break;
	case 21:
		clear_ledbuff();
		SET_LED(LED19, ledon); //
		break;
	case 22:
		clear_ledbuff();
		SET_LED(LED14, ledon); //
		break;
	case 23:
		clear_ledbuff();
		SET_LED(LED20, ledon); //
		break;
	case 24:
		clear_ledbuff();
		SET_LED(LED81, ledon); //
		break;
	case 25:
		clear_ledbuff();
		SET_LED(LED82, ledon); //
		break;
	case 26:
		clear_ledbuff();
		SET_LED(LED11, ledon); //
		break;
	case 27:
		clear_ledbuff();
		SET_LED(LED17, ledon); //
		break;
	case 28:
		clear_ledbuff();
		SET_LED(LED264, ledon); //
		break;
	case 29:
		clear_ledbuff();
		SET_LED(LED8, ledon); //
		break;
	case 30:
		clear_ledbuff();
		SET_LED(LED22, ledon); //
		break;
		//********************************************
	case 31:
		clear_ledbuff();
		SET_LED(LED180B, ledon); //
		break;
	case 32:
		clear_ledbuff();
		SET_LED(LED186B, ledon); //
		break;
	case 33:
		clear_ledbuff();
		SET_LED(LED192B, ledon); //
		break;
	case 34:
		clear_ledbuff();
		SET_LED(LED198B, ledon); //
		break;
	case 35:
		clear_ledbuff();
		SET_LED(LED204B, ledon); //
		break;

	case 36:
		clear_ledbuff();
		SET_LED(LED210B, ledon); //
		break;
	case 37:
		//		SET_LED(LED180B,  ledoff);	//
		clear_ledbuff();
		SET_LED(LED215B, ledon); //
		break;
	case 38:
		//		SET_LED(LED180B,  ledoff);	//
		clear_ledbuff();
		SET_LED(LED219B, ledon); //
		break;

	case 39:
		//		SET_LED(LED180B,  ledoff);	//
		clear_ledbuff();
		SET_LED(LED225B, ledon); //
		break;

	case 40:
		//		SET_LED(LED180B,  ledoff);	//
		clear_ledbuff();
		SET_LED(LED231B, ledon); //
		break;
	case 41:
		//		SET_LED(LED180B,  ledoff);	//
		clear_ledbuff();
		SET_LED(LED237B, ledon); //
		break;

	case 42:
		//		SET_LED(LED180B,  ledoff);	//
		clear_ledbuff();
		SET_LED(LED243B, ledon); //
		break;

	case 43:
		//		SET_LED(LED180B,  ledoff);	//
		clear_ledbuff();
		SET_LED(LED248B, ledon); //
		break;

	case 44:
		//		SET_LED(LED180B,  ledoff);	//
		clear_ledbuff();
		SET_LED(LED253B, ledon); //
		break;

	case 45:
		//		SET_LED(LED180B,  ledoff);	//
		clear_ledbuff();
		SET_LED(LED258B, ledon); //
		break;

	case 46:
		//		SET_LED(LED180B,  ledoff);	//
		clear_ledbuff();
		SET_LED(LED263B, ledon); //
		break;

		//************************************************
	case 47:
		//		SET_LED(LED22,  ledoff);	//
		clear_ledbuff();
		SET_LED(LED180G, ledon); //
		break;
	case 48:
		clear_ledbuff();
		SET_LED(LED186G, ledon); //
		break;
	case 49:
		clear_ledbuff();
		SET_LED(LED192G, ledon); //
		break;
	case 50:
		clear_ledbuff();
		SET_LED(LED198G, ledon); //
		break;
	case 51:
		clear_ledbuff();
		SET_LED(LED204G, ledon); //
		break;

	case 52:
		clear_ledbuff();
		SET_LED(LED210G, ledon); //
		break;
	case 53:
		clear_ledbuff();
		SET_LED(LED215G, ledon); //
		break;
	case 54:
		clear_ledbuff();
		SET_LED(LED219G, ledon); //
		break;

	case 55:
		clear_ledbuff();
		SET_LED(LED225G, ledon); //
		break;

	case 56:
		clear_ledbuff();
		SET_LED(LED231G, ledon); //
		break;
	case 57:
		clear_ledbuff();
		SET_LED(LED237G, ledon); //
		break;

	case 58:
		clear_ledbuff();
		SET_LED(LED243G, ledon); //
		break;

	case 59:
		clear_ledbuff();
		SET_LED(LED248G, ledon); //
		break;

	case 60:
		clear_ledbuff();
		SET_LED(LED253G, ledon); //
		break;

	case 61:
		clear_ledbuff();
		SET_LED(LED258G, ledon); //
		break;

	case 62:
		clear_ledbuff();
		SET_LED(LED263G, ledon); //
		break;

		//******************************************
	case 63:
		clear_ledbuff();
		SET_LED(LED180R, ledon); //
		break;
	case 64:
		clear_ledbuff();
		SET_LED(LED186R, ledon); //
		break;
	case 65:
		clear_ledbuff();
		SET_LED(LED192R, ledon); //
		break;
	case 66:
		clear_ledbuff();
		SET_LED(LED198R, ledon); //
		break;
	case 67:
		clear_ledbuff();
		SET_LED(LED204R, ledon); //
		break;

	case 68:
		clear_ledbuff();
		SET_LED(LED210R, ledon); //
		break;
	case 69:
		clear_ledbuff();
		SET_LED(LED215R, ledon); //
		break;
	case 70:
		clear_ledbuff();
		SET_LED(LED219R, ledon); //
		break;

	case 71:
		clear_ledbuff();
		SET_LED(LED225R, ledon); //
		break;

	case 72:
		clear_ledbuff();
		SET_LED(LED231R, ledon); //
		break;
	case 73:
		clear_ledbuff();
		SET_LED(LED237R, ledon); //
		break;

	case 74:
		clear_ledbuff();
		SET_LED(LED243R, ledon); //
		break;

	case 75:
		clear_ledbuff();
		SET_LED(LED248R, ledon); //
		break;

	case 76:
		clear_ledbuff();
		SET_LED(LED253R, ledon); //
		break;

	case 77:
		clear_ledbuff();
		SET_LED(LED258R, ledon); //
		break;

	case 78:
		clear_ledbuff();
		SET_LED(LED263R, ledon); //
		break;
		//***********************************************
		//***********************************************
	case 79:
		//		SET_LED(LED22,  ledoff);	//
		clear_ledbuff();
		SET_LED(LED395B, ledon); //
		break;
	case 80:
		//		SET_LED(LED180B,  ledoff);	//
		clear_ledbuff();
		SET_LED(LED396B, ledon); //
		break;
	case 81:
		//		SET_LED(LED180B,  ledoff);	//
		clear_ledbuff();
		SET_LED(LED397B, ledon); //
		break;
	case 82:
		//		SET_LED(LED180B,  ledoff);	//
		clear_ledbuff();
		SET_LED(LED398B, ledon); //
		break;
	case 83:
		//		SET_LED(LED180B,  ledoff);	//
		clear_ledbuff();
		SET_LED(LED399B, ledon); //
		break;

	case 84:
		//		SET_LED(LED180B,  ledoff);	//
		clear_ledbuff();
		SET_LED(LED400B, ledon); //
		break;
	case 85:
		//		SET_LED(LED180B,  ledoff);	//
		clear_ledbuff();
		SET_LED(LED401B, ledon); //
		break;
	case 86:
		//		SET_LED(LED180B,  ledoff);	//
		clear_ledbuff();
		SET_LED(LED402B, ledon); //
		break;

	case 87:
		//		SET_LED(LED180B,  ledoff);	//
		clear_ledbuff();
		SET_LED(LED403B, ledon); //
		break;

	case 88:
		clear_ledbuff();
		SET_LED(LED404B, ledon); //
		break;
	case 89:
		clear_ledbuff();
		SET_LED(LED405B, ledon); //
		break;

	case 90:
		clear_ledbuff();
		SET_LED(LED406B, ledon); //
		break;

	case 91:
		clear_ledbuff();
		SET_LED(LED407B, ledon); //
		break;

	case 92:
		clear_ledbuff();
		SET_LED(LED408B, ledon); //
		break;

	case 93:
		clear_ledbuff();
		SET_LED(LED409B, ledon); //
		break;

	case 94:
		clear_ledbuff();
		SET_LED(LED410B, ledon); //
		break;

		//************************************************
	case 95:
		clear_ledbuff();
		SET_LED(LED395G, ledon); //
		break;
	case 96:
		clear_ledbuff();
		SET_LED(LED396G, ledon); //
		break;
	case 97:
		clear_ledbuff();
		SET_LED(LED397G, ledon); //
		break;
	case 98:
		clear_ledbuff();
		SET_LED(LED398G, ledon); //
		break;
	case 99:
		clear_ledbuff();
		SET_LED(LED399G, ledon); //
		break;

	case 100:
		clear_ledbuff();
		SET_LED(LED400G, ledon); //
		break;
	case 101:
		clear_ledbuff();
		SET_LED(LED401G, ledon); //
		break;
	case 102:
		clear_ledbuff();
		SET_LED(LED402G, ledon); //
		break;

	case 103:
		clear_ledbuff();
		SET_LED(LED403G, ledon); //
		break;

	case 104:
		clear_ledbuff();
		SET_LED(LED404G, ledon); //
		break;
	case 105:
		clear_ledbuff();
		SET_LED(LED405G, ledon); //
		break;

	case 106:
		clear_ledbuff();
		SET_LED(LED406G, ledon); //
		break;

	case 107:
		clear_ledbuff();
		SET_LED(LED407G, ledon); //
		break;

	case 108:
		clear_ledbuff();
		SET_LED(LED408G, ledon); //
		break;

	case 109:
		clear_ledbuff();
		SET_LED(LED409G, ledon); //
		break;

	case 110:
		clear_ledbuff();
		SET_LED(LED410G, ledon); //
		break;

		//******************************************

	case 112:
		clear_ledbuff();
		SET_LED(LED395R, ledon); //
		break;
	case 113:
		clear_ledbuff();
		SET_LED(LED396R, ledon); //
		break;
	case 114:
		clear_ledbuff();
		SET_LED(LED397R, ledon); //
		break;
	case 115:
		clear_ledbuff();
		SET_LED(LED398R, ledon); //
		break;
	case 116:
		clear_ledbuff();
		SET_LED(LED399R, ledon); //
		break;

	case 117:
		clear_ledbuff();
		SET_LED(LED400R, ledon); //
		break;
	case 118:
		clear_ledbuff();
		SET_LED(LED401R, ledon); //
		break;
	case 119:
		clear_ledbuff();
		SET_LED(LED402R, ledon); //
		break;

	case 120:
		clear_ledbuff();
		SET_LED(LED403R, ledon); //
		break;

	case 121:
		clear_ledbuff();
		SET_LED(LED404R, ledon); //
		break;
	case 122:
		clear_ledbuff();
		SET_LED(LED405R, ledon); //
		break;

	case 123:
		clear_ledbuff();
		SET_LED(LED406R, ledon); //
		break;

	case 124:
		clear_ledbuff();
		SET_LED(LED407R, ledon); //
		break;

	case 125:
		clear_ledbuff();
		SET_LED(LED408R, ledon); //
		break;

	case 126:
		clear_ledbuff();
		SET_LED(LED409R, ledon); //
		break;

	case 127:
		clear_ledbuff();
		SET_LED(LED410R, ledon); //
		break;
		//***********************************************************************
		//***********************************************************************
	case 128:
		clear_ledbuff();
		SET_LED(LED86, ledon); //
		break;

	case 129:
		clear_ledbuff();
		SET_LED(LED92, ledon); //
		break;
	case 130:
		clear_ledbuff();
		SET_LED(LED98, ledon); //
		break;
	case 131:
		clear_ledbuff();
		SET_LED(LED103, ledon); //
		break;
	case 132:
		clear_ledbuff();
		SET_LED(LED107, ledon); //
		break;
	case 133:
		clear_ledbuff();
		SET_LED(LED112, ledon); //
		break;

	case 134:
		clear_ledbuff();
		SET_LED(LED117, ledon); //
		break;
	case 135:
		clear_ledbuff();
		SET_LED(LED122, ledon); //
		break;
	case 136:
		clear_ledbuff();
		SET_LED(LED128, ledon); //
		break;
	case 137:
		clear_ledbuff();
		SET_LED(LED135, ledon); //
		break;
	case 138:
		clear_ledbuff();
		SET_LED(LED142, ledon); //
		break;
	case 139:
		clear_ledbuff();
		SET_LED(LED148, ledon); //
		break;
	case 140:
		clear_ledbuff();
		SET_LED(LED153, ledon); //
		break;
	case 141:
		clear_ledbuff();
		SET_LED(LED160, ledon); //
		break;
	case 142:
		clear_ledbuff();
		SET_LED(LED167, ledon); //
		break;
	case 143:
		clear_ledbuff();
		SET_LED(LED174, ledon); //
		break;
	case 144:
		clear_ledbuff();
		SET_LED(LED152, ledon); //
		break;
	case 145:
		clear_ledbuff();
		SET_LED(LED159, ledon); //
		break;

	case 146:
		clear_ledbuff();
		SET_LED(LED166, ledon); //
		break;
	case 147:
		clear_ledbuff();
		SET_LED(LED173, ledon); //
		break;
	case 148:
		clear_ledbuff();
		SET_LED(LED179, ledon); //
		break;
	case 149:
		clear_ledbuff();
		SET_LED(LED185, ledon); //
		break;
	case 150:
		clear_ledbuff();
		SET_LED(LED191, ledon); //
		break;
	case 151:
		clear_ledbuff();
		SET_LED(LED197, ledon); //
		break;
	case 152:
		clear_ledbuff();
		SET_LED(LED203, ledon); //
		break;
	case 153:
		clear_ledbuff();
		SET_LED(LED209, ledon); //
		break;
	case 154:
		clear_ledbuff();
		SET_LED(LED236, ledon); //
		break;
	case 155:
		clear_ledbuff();
		SET_LED(LED242, ledon); //
		break;
	case 156:
		clear_ledbuff();
		SET_LED(LED247, ledon); //
		break;
	case 157:
		clear_ledbuff();
		SET_LED(LED252, ledon); //
		break;
	case 158:
		clear_ledbuff();
		SET_LED(LED257, ledon); //
		break;
	case 159:
		clear_ledbuff();
		SET_LED(LED262, ledon); //
		break;
	}
}

// 建立Touch 函數指標位元
void (*touch_tasks[])() = {
	touch_task0, touch_task1, touch_task2, touch_task3, touch_task4, touch_task5, touch_task6, touch_task7, touch_task8,
	touch_task9, touch_task10, touch_task11, touch_task12, touch_task13, touch_task14, touch_task15};

// 建立函數指標位元
void (*tasks[])() = {
	task0, task1, task2, task3, task4, task5, task6, task7, task8,
	task9, task10, task11, task12, task13, task14, task15, task16,
	task17, task18, task19, task20, task21, task22, task23, task24,
	task25, task26, task27, task28, task29, task30, task31, task32,
	task33, task34, task35, task36, task37, task38, task39, task40,
	task41, task42, task43, task44, task45, task46, task47};

void setup()
{
	Wire.begin();		   //(D2,D1);  //(0,2); //sda=0 | D3, scl=2 | D4
	Wire.setClock(100000); // 設定 i2c 的速率為 100 KHz RP2040 很重要不然會導致通信錯誤！！ 導線太長速度太快會導致資料錯誤！！！2024/11/03 by leecw
	ads.begin(0x48);
	//  Serial.println("Failed to initialize ADS.");
	//  while (1);
	// }

	Serial.begin(115200);

	pinMode(LED_PIN, OUTPUT);	// 工作LED閃爍 初始化
	pinMode(TCH_SEND0, OUTPUT); // 工作LED閃爍 初始化
								// 設定PWM信號
								//  analogWriteFrequency(TCH_SEND0, 100); // 設定頻率為1kHz
								//  analogWrite(pwmPin, 128);           // 設定佔空比50%
								// 初始化PWM實例，設定頻率為100Hz，佔空比為50%
	PWM_Instance = new RP2040_PWM(TCH_SEND0, 100, 0);
	PWM_Instance->setPWM(TCH_SEND0, 500, 50); // 頻率100HZ duty cycel 50%

	//	analogWrite(TCH_SEND0,128);
	pinMode(MUXSEL0, OUTPUT);		 //
	pinMode(MUXSEL1, OUTPUT);		 //
	pinMode(MUXSEL2, OUTPUT);		 //
	pinMode(TCH_SEL0, OUTPUT);		 //
	pinMode(TCH_SEL1, OUTPUT);		 //
	pinMode(TCH_RET0, INPUT_PULLUP); //
	pinMode(TCH_RET1, INPUT_PULLUP); //
	pinMode(TCH_RET2, INPUT_PULLUP); //
	pinMode(TCH_RET3, INPUT_PULLUP); //

	pinMode(CLK, INPUT); //
	pinMode(DT, INPUT);	 //
						 //  digitalWrite(CLK, HIGH);
						 //  digitalWrite(DT, HIGH);

	//  pinMode(debugPin, OUTPUT); // 工作LED閃爍 初始化
	//	digitalWrite(debugPin, HIGH);
	//	pinMode(FS1, OUTPUT);
	//	pinMode(FS2, OUTPUT);
	//	digitalWrite(FS1, LOW);
	//	digitalWrite(FS2, LOW);
	for (int i = 0; i < cols; i++)
	{
		pinMode(colPins[i], OUTPUT);
		digitalWrite(colPins[i], HIGH);
	}
	for (int i = 0; i < rows; i++)
	{
		pinMode(rowPins[i], INPUT_PULLUP);
	}

	//***********************************
	// Encoder 中斷啟動
	//***********************************
	// 初始化上一次的CLK狀態
	lastStateCLK = digitalRead(CLK);
	// 設定中斷：當CLK信號改變時觸發
	attachInterrupt(digitalPinToInterrupt(CLK), Encode_process, CHANGE);

	//  pinMode(CLK, INPUT_PULLUP); // 設定A腳為輸入並啟用內部上拉電阻
	//  pinMode(DT, INPUT_PULLUP); // 設定B腳為輸入並啟用內部上拉電阻
	//  attachInterrupt(digitalPinToInterrupt(EncoderA), updateEncoder, CHANGE); // 當A腳變化時觸發中斷

	// 對SNLES27351 reset
	pinMode(RST_LEDn_L, OUTPUT);
	digitalWrite(RST_LEDn_L, LOW);
	delay(30);
	digitalWrite(RST_LEDn_L, HIGH);
	delay(30); // SNLED27351 datasheet 規定reset 之後必須間隔20ms以上才能動作

	// 設定 LED 驅動器的初始值，包括功能頁面、LED 控制頁面、PWM 頁面和電流頁面
	I2C_W_2BYTE(SNLED2735_ADDRESS, 0xFD, 0x03); // 選擇功能頁面 (Page 3)
	I2C_W_2BYTE(SNLED2735_ADDRESS, 0x00, 0x01); // 設定 LED 驅動器為一般模式//關閉模式
	I2C_W_2BYTE(SNLED2735_ADDRESS, 0x13, 0xAA); // 設定內部通道下拉/上拉電阻
	I2C_W_2BYTE(SNLED2735_ADDRESS, 0x14, 0x00); // 選擇掃描相位為 CB1~CB12
	I2C_W_2BYTE(SNLED2735_ADDRESS, 0x15, 0x04); // 設定 PWM 延遲相位啟用
	I2C_W_2BYTE(SNLED2735_ADDRESS, 0x16, 0xC0); // 設定 CA/CB 通道擺率控制啟用
												//  I2C_W_2BYTE(SNLED2735_ADDRESS, 0x17, 0xC0);  // 設定 OPEN/SHORT 偵測啟用

	I2C_W_2BYTE(SNLED2735_ADDRESS, 0x1A, 0x00); // 設定 Iref 模式禁用

	I2C_W_2BYTE(SNLED2735_ADDRESS, 0xFD, 0x00);		  // 選擇 LED 控制頁面 (Page 0)
	I2C_W_NBYTE(SNLED2735_ADDRESS, 0x00, 0x47, 0x00); // 清除 LED 控制暫存器 0x00~0x47 的資料

	I2C_W_2BYTE(SNLED2735_ADDRESS, 0xFD, 0x01);		  // 選擇 PWM 頁面 (Page 1)
	I2C_W_NBYTE(SNLED2735_ADDRESS, 0x00, 0xBF, 0xAF); // 清除 PWM 暫存器 0x00~0xBF 的資料  這裡要設0x80才會亮這個很重要！！！！2024/11/03 by leecw

	I2C_W_2BYTE(SNLED2735_ADDRESS, 0xFD, 0x04);		  // 選擇電流頁面 (Page 4)
													  //  I2C_W_NBYTE(SNLED2735_ADDRESS, 0x00, 0x0B, 0x80);  // 設定 CCS 暫存器 Addr. 0x00~0x0B = 20mA
													  //  I2C_W_NBYTE(SNLED2735_ADDRESS, 0x00, 0x0B, 0x40);  // 設定 CCS 暫存器 Addr. 0x00~0x0B = 10mA (64*0.157)
	I2C_W_NBYTE(SNLED2735_ADDRESS, 0x00, 0x0B, 0x40); // 2024/11/13 改為0xcc 設定 CCS 暫存器 Addr. 0x00~0x0B = 5mA (31*0.157)
													  //********************************
													  // 為了測試LED驅動IC所寫的程式碼
	//********************************
	// while(true)
	//{
	// All_ledon();
	// readLEDRegister();
	// delay(2000);
	//  All_ledoff();
	//  readLEDRegister();
	// delay(2000);
	//}
	/*
	  I2C_W_2BYTE(SNLED2735_ADDRESS, 0xFD, 0x03);  // 選擇功能頁面 (Page 3)
	  I2C_W_2BYTE(SNLED2735_ADDRESS, 0x17, 0xC0);  // 設定 OPEN/SHORT 偵測啟用
	  I2C_W_2BYTE(SNLED2735_ADDRESS, 0xFD, 0x00);  // 選擇 LED 控制頁面 (Page 0)
	  I2C_W_NBYTE(SNLED2735_ADDRESS, 0x18, 0x27, 0xFF);  // 設定偵測OPEN LED 控制暫存器 0x18~2F 的資料為enable
	  readOpenRegister();
	  I2C_W_2BYTE(SNLED2735_ADDRESS, 0xFD, 0x00);  // 選擇 LED 控制頁面 (Page 0)
	  I2C_W_NBYTE(SNLED2735_ADDRESS, 0x30, 0x47, 0xFF);  // 設定偵測short LED 控制暫存器 0x30~47 的資料enable
	  readShortRegister();
	  I2C_W_2BYTE(SNLED2735_ADDRESS, 0xFD, 0x03);  // 選擇功能頁面 (Page 3)
	  I2C_W_2BYTE(SNLED2735_ADDRESS, 0x17, 0x00);  // 設定 OPEN/SHORT 偵測關閉
		I2C_W_2BYTE(SNLED2735_ADDRESS, 0xFD, 0x00);  // 選擇 LED 控制頁面 (Page 0)
	  I2C_W_NBYTE(SNLED2735_ADDRESS, 0x18, 0x47, 0x00);  // open/short disable
	*/
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
	display.setTextSize(1);				 // Normal 1:1 pixel scale
	display.setTextColor(SSD1306_WHITE); // Draw white text
	display.setCursor(0, 0);			 // Start at top-left corner
	display.println(F("Yahorng PE !!"));
	display.display();
	delay(2000);
	//  All_ledoff();
}

void pwm_turnon()
{
	PWM_Instance->setPWM(TCH_SEND0, 500, 50); // 頻率500HZ duty cycel 50%
}

void pwm_turnoff()
{
	PWM_Instance->setPWM(TCH_SEND0, 500, 0); // 頻率500HZ duty cycel 0%
}

bool check_ret0()
{

	//	selectTOUCH(ch0);
	//	delay(1);
	//	pwm_turnon();

	// 1. 測量頻率
	float measuredFrequency = measureFrequency(TCH_RET0);
	Serial.print("measuredFrequency is :");
	Serial.println(measuredFrequency);
	// 2. 更新濾波器
	updateFrequencyBuffer(measuredFrequency);

	// 3. 計算移動平均值
	filteredFrequency = calculateFilteredFrequency();

	// 4. 根據濾波後的頻率判定觸控狀態
	if (filteredFrequency > baseFrequency * (1 + touchThreshold / 100.0))
	{
		if (!isTouching)
		{
			isTouching = true;
			//      touchOn();
			SET_LED(LED180G, ledon);
			Serial.println("VR Touch On !!");
		}
	}
	else
	{
		if (isTouching)
		{
			isTouching = false;
			//      touchOff();
			SET_LED(LED180G, ledoff);
			Serial.println("VR Touch Off !!");
		}
	}

	delay(10); // 可根據實際需求調整
	return isTouching;
	/*
		pwm_turnon();
	  delay(1);
		bool result0 = 0;
	   lowTime0 = pulseIn(TCH_RET0, LOW);
		 highTime0 = pulseIn(TCH_RET0, HIGH);
	  unsigned long pulseDuration = highTime0 + lowTime0;                      //pulseIn(TCH_RET0, HIGH,10000);
	  unsigned long frequency = 1000000 / pulseDuration;  //(2 * pulseDuration); // 計算頻率
	 // Serial.print("VR test frequency: ");
	 // Serial.println(frequency);
	  if (frequency > 200000 ) {
		touchState = 1;
	//	Serial.println("Touch ON !!");
			Serial.print("VR frequency: ");
			Serial.println(frequency);

			// 等待信號恢復到1kHz
		while (frequency > 1000) {
		  lowTime0 = pulseIn(TCH_RET0, LOW,15000);
		  highTime0 = pulseIn(TCH_RET0, HIGH,15000);
		  if (lowTime0 !=0 && highTime0 !=0)
		  {
		  pulseDuration = highTime0 + lowTime0;
		  frequency = 1000000.0 / pulseDuration;
		  Serial.print("while : ");
		  Serial.println(frequency);
		  }
		}
			Serial.print("exir while :");
			Serial.println(frequency);
			pwm_turnoff();
	//	  analogWrite(TCH_SEND0,0);
		  result0 = 1;

	//	return result;
	  } else if (frequency <= 100000 ) {
		touchState = 0;
	//	Serial.println("Touch OFF !!");
	//	analogWrite(TCH_SEND0,0);
			pwm_turnoff();
		result0 = 0;
	//	return result;

	  }
		return result0; */
}

bool check_ret1(void)
{

	//	selectTOUCH(ch0);
	//	delay(1);
	//	pwm_turnon();

	// 1. 測量頻率
	float measuredFrequency = measureFrequency(TCH_RET1);
	Serial.print("measuredFrequency is :");
	Serial.println(measuredFrequency);
	// 2. 更新濾波器
	updateFrequencyBuffer(measuredFrequency);

	// 3. 計算移動平均值
	filteredFrequency = calculateFilteredFrequency();

	// 4. 根據濾波後的頻率判定觸控狀態
	if (filteredFrequency > baseFrequency * (1 + touchThreshold / 100.0))
	{
		if (!isTouching)
		{
			isTouching = true;
			//      touchOn();
			SET_LED(LED180G, ledon);
			Serial.println("VR Touch On !!");
		}
	}
	else
	{
		if (isTouching)
		{
			isTouching = false;
			//      touchOff();
			SET_LED(LED180G, ledoff);
			Serial.println("VR Touch Off !!");
		}
	}

	delay(10); // 可根據實際需求調整
	return isTouching;

	/*
	  delay(5);
		bool result1 = 0;
	   lowTime1 = pulseIn(TCH_RET1, LOW,15000);
		 highTime1 = pulseIn(TCH_RET1, HIGH,15000);
	  unsigned long pulseDuration = highTime1 + lowTime1;                      //pulseIn(TCH_RET0, HIGH,10000);
	  unsigned long frequency = 1000000 / pulseDuration;  //(2 * pulseDuration); // 計算頻率
	  Serial.print("VR test frequency: ");
	 Serial.println(frequency);
	  if (frequency > 200000 ) {
		touchState = 1;
	//	Serial.println("Touch ON !!");
			Serial.print("VR frequency: ");
			Serial.println(frequency);

			// 等待信號恢復到1kHz
		while (frequency > 1000) {
		  lowTime1 = pulseIn(TCH_RET1, LOW,15000);
		  highTime1 = pulseIn(TCH_RET1, HIGH,15000);
		  if (lowTime1 !=0 && highTime1 !=0)
		  {
		  pulseDuration = highTime1 + lowTime1;
		  frequency = 1000000.0 / pulseDuration;
		  Serial.print("while : ");
		  Serial.println(frequency);
		  }
		}
			Serial.print("exir while :");
			Serial.println(frequency);
	//	  analogWrite(TCH_SEND0,0);
			pwm_turnoff();
		  result1 = 1;

	//	return result;
	  } else if (frequency <= 100000 ) {
		touchState = 0;
	//	Serial.println("Touch OFF !!");
	//	analogWrite(TCH_SEND0,0);
			pwm_turnoff();
		result1 = 0;
	//	return result;

	  }
		return result1;	 */
}

bool check_ret2(void)
{

	selectTOUCH(ch0);
	delay(1);
	pwm_turnon();

	// 1. 測量頻率
	float measuredFrequency = measureFrequency(TCH_RET2);
	Serial.print("measuredFrequency is :");
	Serial.println(measuredFrequency);
	// 2. 更新濾波器
	updateFrequencyBuffer(measuredFrequency);

	// 3. 計算移動平均值
	filteredFrequency = calculateFilteredFrequency();

	// 4. 根據濾波後的頻率判定觸控狀態
	if (filteredFrequency > baseFrequency * (1 + touchThreshold / 100.0))
	{
		if (!isTouching)
		{
			isTouching = true;
			//      touchOn();
			SET_LED(LED180G, ledon);
			Serial.println("VR Touch On !!");
		}
	}
	else
	{
		if (isTouching)
		{
			isTouching = false;
			//      touchOff();
			SET_LED(LED180G, ledoff);
			Serial.println("VR Touch Off !!");
		}
	}

	delay(10); // 可根據實際需求調整
	return isTouching;

	/*
	  delay(5);
		bool result2 = 0;
	   lowTime2 = pulseIn(TCH_RET2, LOW);
		 highTime2 = pulseIn(TCH_RET2, HIGH);
	  unsigned long pulseDuration = highTime2 + lowTime2;                      //pulseIn(TCH_RET0, HIGH,10000);
	  unsigned long frequency = 1000000 / pulseDuration;  //(2 * pulseDuration); // 計算頻率
	 // Serial.print("VR test frequency: ");
	 // Serial.println(frequency);
	  if (frequency > 200000 ) {
		touchState = 1;
	//	Serial.println("Touch ON !!");
			Serial.print("VR frequency: ");
			Serial.println(frequency);

			// 等待信號恢復到1kHz
		while (frequency > 1000) {
		  lowTime2 = pulseIn(TCH_RET2, LOW,15000);
		  highTime2 = pulseIn(TCH_RET2, HIGH,15000);
		  if (lowTime2 !=0 && highTime2 !=0)
		  {
		  pulseDuration = highTime2 + lowTime2;
		  frequency = 1000000.0 / pulseDuration;
		  Serial.print("while : ");
		  Serial.println(frequency);
		  }
		}
			Serial.print("exir while :");
			Serial.println(frequency);
	//	  analogWrite(TCH_SEND0,0);
			pwm_turnoff();
		  result2 = 1;

	//	return result;
	  } else if (frequency <= 100000 ) {
		touchState = 0;
	//	Serial.println("Touch OFF !!");
	//	analogWrite(TCH_SEND0,0);
			pwm_turnoff();
		result2 = 0;
	//	return result;

	  }
		return result2;	*/
}

bool check_ret3(void)
{
	selectTOUCH(ch0);
	delay(1);
	pwm_turnon();

	// 1. 測量頻率
	float measuredFrequency = measureFrequency(TCH_RET3);
	Serial.print("measuredFrequency is :");
	Serial.println(measuredFrequency);
	// 2. 更新濾波器
	updateFrequencyBuffer(measuredFrequency);

	// 3. 計算移動平均值
	filteredFrequency = calculateFilteredFrequency();

	// 4. 根據濾波後的頻率判定觸控狀態
	if (filteredFrequency > baseFrequency * (1 + touchThreshold / 100.0))
	{
		if (!isTouching)
		{
			isTouching = true;
			//      touchOn();
			SET_LED(LED180G, ledon);
			Serial.println("VR Touch On !!");
		}
	}
	else
	{
		if (isTouching)
		{
			isTouching = false;
			//      touchOff();
			SET_LED(LED180G, ledoff);
			Serial.println("VR Touch Off !!");
		}
	}

	delay(10); // 可根據實際需求調整
	return isTouching;

	/*
	  delay(5);
		bool result3 = 0;
	   lowTime3 = pulseIn(TCH_RET3, LOW);
		 highTime3 = pulseIn(TCH_RET3, HIGH);
	  unsigned long pulseDuration = highTime3 + lowTime3;                      //pulseIn(TCH_RET0, HIGH,10000);
	  unsigned long frequency = 1000000 / pulseDuration;  //(2 * pulseDuration); // 計算頻率
	 // Serial.print("VR test frequency: ");
	 // Serial.println(frequency);
	  if (frequency > 200000 ) {
		touchState = 1;
	//	Serial.println("Touch ON !!");
			Serial.print("VR frequency: ");
			Serial.println(frequency);

			// 等待信號恢復到1kHz
		while (frequency > 1000) {
		  lowTime3 = pulseIn(TCH_RET3, LOW,15000);
		  highTime3 = pulseIn(TCH_RET3, HIGH,15000);
		  if (lowTime3 !=0 && highTime3 !=0)
		  {
		  pulseDuration = highTime3 + lowTime3;
		  frequency = 1000000.0 / pulseDuration;
		  Serial.print("while : ");
		  Serial.println(frequency);
		  }
		}
			Serial.print("exir while :");
			Serial.println(frequency);
		//	analogWrite(TCH_SEND0,0);
			pwm_turnoff();
			result3 = 1;

	//	return result;
	  } else if (frequency <= 100000 ) {
		touchState = 0;
	//	Serial.println("Touch OFF !!");
	//	analogWrite(TCH_SEND0,0);
			pwm_turnoff();
		result3 = 0;
	//	return result;

	  }
		return result3;	*/
}

void executeTouchTask(int Touchcol)
{
	switch (Touchcol)
	{
	case 0:
		touch_task0();
		break;
	case 1:
		touch_task1();
		break;
	case 2:
		touch_task2();
		break;
	case 3:
		touch_task3();
		break;
	case 4:
		touch_task4();
		break;
	case 5:
		touch_task5();
		break;
	case 6:
		touch_task6();
		break;
	case 7:
		touch_task7();
		break;
	case 8:
		touch_task8();
		break;
	case 9:
		touch_task9();
		break;
	case 10:
		touch_task10();
		break;
	case 11:
		touch_task11();
		break;
	case 12:
		touch_task12();
		break;
	case 13:
		touch_task13();
		break;
	case 14:
		touch_task14();
		break;
	case 15:
		touch_task15();
		break;
	}
}
/*
*****************************************
***** 2024/11/22 李進衛建立
***** 觸控演算法重新改寫
*****************************************

 */
void tch_vr0()
{
	if (T_CNT1 != 0)
		return;
	T_CNT1 = 10 / checkInterval; // delay 10ms
	selectTOUCH(ch0);
	pwm_turnon();
	// 1. 測量頻率
	float measuredFrequency = measureFrequency(TCH_RET0);
	Serial.print("measuredFrequency vr0 is :");
	Serial.println(measuredFrequency);
	// 2. 更新濾波器
	updateFrequencyBuffer(measuredFrequency);

	// 3. 計算移動平均值
	filteredFrequency = calculateFilteredFrequency();

	// 4. 根據濾波後的頻率判定觸控狀態
	if (filteredFrequency > baseFrequency * (1 + touchThreshold / 100.0))
	{
		if (!isTouching)
		{
			isTouching = true;
			//      touchOn();
			SET_LED(LED395G, ledon);
			Serial.println("VR0 Touch On !!");
		}
	}
	else
	{
		if (isTouching)
		{
			isTouching = false;
			//      touchOff();
			SET_LED(LED180R, ledoff);
			SET_LED(LED395G, ledoff);
			Serial.println("VR0 Touch Off !!");
			touch_SQN = vr1;
		}
	}
}

//*************************************************************
void tch_vr1()
{
	if (T_CNT1 != 0)
		return;
	T_CNT1 = 10 / checkInterval; // delay 10ms
	selectTOUCH(ch1);
	pwm_turnon();
	// 1. 測量頻率
	float measuredFrequency = measureFrequency(TCH_RET0);
	Serial.print("measuredFrequency vr1 is :");
	Serial.println(measuredFrequency);
	// 2. 更新濾波器
	updateFrequencyBuffer(measuredFrequency);

	// 3. 計算移動平均值
	filteredFrequency = calculateFilteredFrequency();

	// 4. 根據濾波後的頻率判定觸控狀態
	if (filteredFrequency > baseFrequency * (1 + touchThreshold / 100.0))
	{
		if (!isTouching)
		{
			isTouching = true;
			//      touchOn();
			SET_LED(LED396G, ledon);
			Serial.println("VR1 Touch On !!");
		}
	}
	else
	{
		if (isTouching)
		{
			isTouching = false;
			//      touchOff();
			SET_LED(LED186R, ledoff);
			SET_LED(LED396G, ledoff);
			Serial.println("VR Touch1 Off !!");
			touch_SQN = vr2;
		}
	}
}

//*************************************************************
void tch_vr2()
{
	if (T_CNT1 != 0)
		return;
	T_CNT1 = 10 / checkInterval; // delay 10ms
	selectTOUCH(ch2);
	pwm_turnon();
	// 1. 測量頻率
	float measuredFrequency = measureFrequency(TCH_RET0);
	Serial.print("measuredFrequency vr2 is :");
	Serial.println(measuredFrequency);
	// 2. 更新濾波器
	updateFrequencyBuffer(measuredFrequency);

	// 3. 計算移動平均值
	filteredFrequency = calculateFilteredFrequency();

	// 4. 根據濾波後的頻率判定觸控狀態
	if (filteredFrequency > baseFrequency * (1 + touchThreshold / 100.0))
	{
		if (!isTouching)
		{
			isTouching = true;
			//      touchOn();
			SET_LED(LED397G, ledon);
			Serial.println("VR2 Touch On !!");
		}
	}
	else
	{
		if (isTouching)
		{
			isTouching = false;
			//      touchOff();
			SET_LED(LED192R, ledoff);
			SET_LED(LED397G, ledoff);
			Serial.println("VR2 Touch Off !!");
			touch_SQN = vr3;
		}
	}
}
//*************************************************************
void tch_vr3()
{
	if (T_CNT1 != 0)
		return;
	T_CNT1 = 10 / checkInterval; // delay 10ms
	selectTOUCH(ch3);
	pwm_turnon();
	// 1. 測量頻率
	float measuredFrequency = measureFrequency(TCH_RET0);
	Serial.print("measuredFrequency vr3 is :");
	Serial.println(measuredFrequency);
	// 2. 更新濾波器
	updateFrequencyBuffer(measuredFrequency);

	// 3. 計算移動平均值
	filteredFrequency = calculateFilteredFrequency();

	// 4. 根據濾波後的頻率判定觸控狀態
	if (filteredFrequency > baseFrequency * (1 + touchThreshold / 100.0))
	{
		if (!isTouching)
		{
			isTouching = true;
			//      touchOn();
			SET_LED(LED398G, ledon);
			Serial.println("VR3 Touch On !!");
		}
	}
	else
	{
		if (isTouching)
		{
			isTouching = false;
			//      touchOff();
			SET_LED(LED198R, ledoff);
			SET_LED(LED398G, ledoff);
			Serial.println("VR3 Touch Off !!");
			touch_SQN = vr4;
		}
	}
}
//*************************************************************
void tch_vr4()
{
	if (T_CNT1 != 0)
		return;
	T_CNT1 = 10 / checkInterval; // delay 10ms
	selectTOUCH(ch0);
	pwm_turnon();
	// 1. 測量頻率
	float measuredFrequency = measureFrequency(TCH_RET1);
	Serial.print("measuredFrequency vr4 is :");
	Serial.println(measuredFrequency);
	// 2. 更新濾波器
	updateFrequencyBuffer(measuredFrequency);

	// 3. 計算移動平均值
	filteredFrequency = calculateFilteredFrequency();

	// 4. 根據濾波後的頻率判定觸控狀態
	if (filteredFrequency > baseFrequency * (1 + touchThreshold / 100.0))
	{
		if (!isTouching)
		{
			isTouching = true;
			//      touchOn();
			SET_LED(LED399G, ledon);
			Serial.println("VR4 Touch On !!");
		}
	}
	else
	{
		if (isTouching)
		{
			isTouching = false;
			//      touchOff();
			SET_LED(LED204R, ledoff);
			SET_LED(LED399G, ledoff);
			Serial.println("VR4 Touch Off !!");
			touch_SQN = vr5;
		}
	}
}
//*************************************************************
void tch_vr5()
{
	if (T_CNT1 != 0)
		return;
	T_CNT1 = 10 / checkInterval; // delay 10ms
	selectTOUCH(ch1);
	pwm_turnon();
	// 1. 測量頻率
	float measuredFrequency = measureFrequency(TCH_RET1);
	Serial.print("measuredFrequency vr5 is :");
	Serial.println(measuredFrequency);
	// 2. 更新濾波器
	updateFrequencyBuffer(measuredFrequency);

	// 3. 計算移動平均值
	filteredFrequency = calculateFilteredFrequency();

	// 4. 根據濾波後的頻率判定觸控狀態
	if (filteredFrequency > baseFrequency * (1 + touchThreshold / 100.0))
	{
		if (!isTouching)
		{
			isTouching = true;
			//      touchOn();
			SET_LED(LED400G, ledon);
			Serial.println("VR5 Touch On !!");
		}
	}
	else
	{
		if (isTouching)
		{
			isTouching = false;
			//      touchOff();
			SET_LED(LED210R, ledoff);
			SET_LED(LED400G, ledoff);
			Serial.println("VR5 Touch Off !!");
			touch_SQN = vr6;
		}
	}
}
//*************************************************************
void tch_vr6()
{
	if (T_CNT1 != 0)
		return;
	T_CNT1 = 10 / checkInterval; // delay 10ms
	selectTOUCH(ch2);
	pwm_turnon();
	// 1. 測量頻率
	float measuredFrequency = measureFrequency(TCH_RET1);
	Serial.print("measuredFrequency vr4 is :");
	Serial.println(measuredFrequency);
	// 2. 更新濾波器
	updateFrequencyBuffer(measuredFrequency);

	// 3. 計算移動平均值
	filteredFrequency = calculateFilteredFrequency();

	// 4. 根據濾波後的頻率判定觸控狀態
	if (filteredFrequency > baseFrequency * (1 + touchThreshold / 100.0))
	{
		if (!isTouching)
		{
			isTouching = true;
			//      touchOn();
			SET_LED(LED401G, ledon);
			Serial.println("VR6 Touch On !!");
		}
	}
	else
	{
		if (isTouching)
		{
			isTouching = false;
			//      touchOff();
			SET_LED(LED215R, ledoff);
			SET_LED(LED401G, ledoff);
			Serial.println("VR6 Touch Off !!");
			touch_SQN = vr7;
		}
	}
}

//*************************************************************
void tch_vr7()
{
	if (T_CNT1 != 0)
		return;
	T_CNT1 = 10 / checkInterval; // delay 10ms
	selectTOUCH(ch3);
	pwm_turnon();
	// 1. 測量頻率
	float measuredFrequency = measureFrequency(TCH_RET1);
	Serial.print("measuredFrequency vr7 is :");
	Serial.println(measuredFrequency);
	// 2. 更新濾波器
	updateFrequencyBuffer(measuredFrequency);

	// 3. 計算移動平均值
	filteredFrequency = calculateFilteredFrequency();

	// 4. 根據濾波後的頻率判定觸控狀態
	if (filteredFrequency > baseFrequency * (1 + touchThreshold / 100.0))
	{
		if (!isTouching)
		{
			isTouching = true;
			//      touchOn();
			SET_LED(LED402G, ledon);
			Serial.println("VR7 Touch On !!");
		}
	}
	else
	{
		if (isTouching)
		{
			isTouching = false;
			//      touchOff();
			SET_LED(LED219R, ledoff);
			SET_LED(LED402G, ledoff);
			Serial.println("VR7 Touch Off !!");
			touch_SQN = vr8;
		}
	}
}

//*************************************************************
void tch_vr8()
{
	if (T_CNT1 != 0)
		return;
	T_CNT1 = 10 / checkInterval; // delay 10ms
	selectTOUCH(ch0);
	pwm_turnon();
	// 1. 測量頻率
	float measuredFrequency = measureFrequency(TCH_RET2);
	Serial.print("measuredFrequency vr8 is :");
	Serial.println(measuredFrequency);
	// 2. 更新濾波器
	updateFrequencyBuffer(measuredFrequency);

	// 3. 計算移動平均值
	filteredFrequency = calculateFilteredFrequency();

	// 4. 根據濾波後的頻率判定觸控狀態
	if (filteredFrequency > baseFrequency * (1 + touchThreshold / 100.0))
	{
		if (!isTouching)
		{
			isTouching = true;
			//      touchOn();
			SET_LED(LED403G, ledon);
			Serial.println("VR8 Touch On !!");
		}
	}
	else
	{
		if (isTouching)
		{
			isTouching = false;
			//      touchOff();
			SET_LED(LED225R, ledoff);
			SET_LED(LED403G, ledoff);
			Serial.println("VR8 Touch Off !!");
			touch_SQN = vr9;
		}
	}
}
//**************************************************************
void tch_vr9()
{
	if (T_CNT1 != 0)
		return;
	T_CNT1 = 10 / checkInterval; // delay 10ms
	selectTOUCH(ch1);
	pwm_turnon();
	// 1. 測量頻率
	float measuredFrequency = measureFrequency(TCH_RET2);
	Serial.print("measuredFrequency vr9 is :");
	Serial.println(measuredFrequency);
	// 2. 更新濾波器
	updateFrequencyBuffer(measuredFrequency);

	// 3. 計算移動平均值
	filteredFrequency = calculateFilteredFrequency();

	// 4. 根據濾波後的頻率判定觸控狀態
	if (filteredFrequency > baseFrequency * (1 + touchThreshold / 100.0))
	{
		if (!isTouching)
		{
			isTouching = true;
			//      touchOn();
			SET_LED(LED404G, ledon);
			Serial.println("VR9 Touch On !!");
		}
	}
	else
	{
		if (isTouching)
		{
			isTouching = false;
			//      touchOff();
			SET_LED(LED231R, ledoff);
			SET_LED(LED404G, ledoff);
			Serial.println("VR9 Touch Off !!");
			touch_SQN = vr10;
		}
	}
}

//**************************************************************
void tch_vr10()
{
	if (T_CNT1 != 0)
		return;
	T_CNT1 = 10 / checkInterval; // delay 10ms
	selectTOUCH(ch2);
	pwm_turnon();
	// 1. 測量頻率
	float measuredFrequency = measureFrequency(TCH_RET2);
	Serial.print("measuredFrequency vr10 is :");
	Serial.println(measuredFrequency);
	// 2. 更新濾波器
	updateFrequencyBuffer(measuredFrequency);

	// 3. 計算移動平均值
	filteredFrequency = calculateFilteredFrequency();

	// 4. 根據濾波後的頻率判定觸控狀態
	if (filteredFrequency > baseFrequency * (1 + touchThreshold / 100.0))
	{
		if (!isTouching)
		{
			isTouching = true;
			//      touchOn();
			SET_LED(LED405G, ledon);
			Serial.println("VR10 Touch On !!");
		}
	}
	else
	{
		if (isTouching)
		{
			isTouching = false;
			//      touchOff();
			SET_LED(LED237R, ledoff);
			SET_LED(LED405G, ledoff);
			Serial.println("VR10 Touch Off !!");
			touch_SQN = vr11;
		}
	}
}
//**************************************************************
void tch_vr11()
{
	if (T_CNT1 != 0)
		return;
	T_CNT1 = 10 / checkInterval; // delay 10ms
	selectTOUCH(ch3);
	pwm_turnon();
	// 1. 測量頻率
	float measuredFrequency = measureFrequency(TCH_RET2);
	Serial.print("measuredFrequency vr11 is :");
	Serial.println(measuredFrequency);
	// 2. 更新濾波器
	updateFrequencyBuffer(measuredFrequency);

	// 3. 計算移動平均值
	filteredFrequency = calculateFilteredFrequency();

	// 4. 根據濾波後的頻率判定觸控狀態
	if (filteredFrequency > baseFrequency * (1 + touchThreshold / 100.0))
	{
		if (!isTouching)
		{
			isTouching = true;
			//      touchOn();
			SET_LED(LED406G, ledon);
			Serial.println("VR11 Touch On !!");
		}
	}
	else
	{
		if (isTouching)
		{
			isTouching = false;
			//      touchOff();
			SET_LED(LED243R, ledoff);
			SET_LED(LED406G, ledoff);
			Serial.println("VR11 Touch Off !!");
			touch_SQN = vr12;
		}
	}
}

//**************************************************************
void tch_vr12()
{
	if (T_CNT1 != 0)
		return;
	T_CNT1 = 10 / checkInterval; // delay 10ms
	selectTOUCH(ch0);
	pwm_turnon();
	// 1. 測量頻率
	float measuredFrequency = measureFrequency(TCH_RET3);
	Serial.print("measuredFrequency vr12 is :");
	Serial.println(measuredFrequency);
	// 2. 更新濾波器
	updateFrequencyBuffer(measuredFrequency);

	// 3. 計算移動平均值
	filteredFrequency = calculateFilteredFrequency();

	// 4. 根據濾波後的頻率判定觸控狀態
	if (filteredFrequency > baseFrequency * (1 + touchThreshold / 100.0))
	{
		if (!isTouching)
		{
			isTouching = true;
			//      touchOn();
			SET_LED(LED407G, ledon);
			Serial.println("VR12 Touch On !!");
		}
	}
	else
	{
		if (isTouching)
		{
			isTouching = false;
			//      touchOff();
			SET_LED(LED248R, ledoff);
			SET_LED(LED407G, ledoff);
			Serial.println("VR12 Touch Off !!");
			touch_SQN = vr13;
		}
	}
}

//**************************************************************
void tch_vr13()
{
	if (T_CNT1 != 0)
		return;
	T_CNT1 = 10 / checkInterval; // delay 10ms
	selectTOUCH(ch1);
	pwm_turnon();
	// 1. 測量頻率
	float measuredFrequency = measureFrequency(TCH_RET3);
	Serial.print("measuredFrequency vr13 is :");
	Serial.println(measuredFrequency);
	// 2. 更新濾波器
	updateFrequencyBuffer(measuredFrequency);

	// 3. 計算移動平均值
	filteredFrequency = calculateFilteredFrequency();

	// 4. 根據濾波後的頻率判定觸控狀態
	if (filteredFrequency > baseFrequency * (1 + touchThreshold / 100.0))
	{
		if (!isTouching)
		{
			isTouching = true;
			//      touchOn();
			SET_LED(LED408G, ledon);
			Serial.println("VR13 Touch On !!");
		}
	}
	else
	{
		if (isTouching)
		{
			isTouching = false;
			//      touchOff();
			SET_LED(LED253R, ledoff);
			SET_LED(LED408G, ledoff);
			Serial.println("VR13 Touch Off !!");
			touch_SQN = vr14;
		}
	}
}

//**************************************************************
void tch_vr14()
{
	if (T_CNT1 != 0)
		return;
	T_CNT1 = 10 / checkInterval; // delay 10ms
	selectTOUCH(ch2);
	pwm_turnon();
	// 1. 測量頻率
	float measuredFrequency = measureFrequency(TCH_RET3);
	Serial.print("measuredFrequency vr14 is :");
	Serial.println(measuredFrequency);
	// 2. 更新濾波器
	updateFrequencyBuffer(measuredFrequency);

	// 3. 計算移動平均值
	filteredFrequency = calculateFilteredFrequency();

	// 4. 根據濾波後的頻率判定觸控狀態
	if (filteredFrequency > baseFrequency * (1 + touchThreshold / 100.0))
	{
		if (!isTouching)
		{
			isTouching = true;
			//      touchOn();
			SET_LED(LED409G, ledon);
			Serial.println("VR14 Touch On !!");
		}
	}
	else
	{
		if (isTouching)
		{
			isTouching = false;
			//      touchOff();
			SET_LED(LED258R, ledoff);
			SET_LED(LED409G, ledoff);
			Serial.println("VR14 Touch Off !!");
			touch_SQN = vr15;
		}
	}
}

//**************************************************************
void tch_vr15()
{
	if (T_CNT1 != 0)
		return;
	T_CNT1 = 10 / checkInterval; // delay 10ms
	selectTOUCH(ch3);
	pwm_turnon();
	// 1. 測量頻率
	float measuredFrequency = measureFrequency(TCH_RET3);
	Serial.print("measuredFrequency vr15 is :");
	Serial.println(measuredFrequency);
	// 2. 更新濾波器
	updateFrequencyBuffer(measuredFrequency);

	// 3. 計算移動平均值
	filteredFrequency = calculateFilteredFrequency();

	// 4. 根據濾波後的頻率判定觸控狀態
	if (filteredFrequency > baseFrequency * (1 + touchThreshold / 100.0))
	{
		if (!isTouching)
		{
			isTouching = true;
			//      touchOn();
			SET_LED(LED410G, ledon);
			Serial.println("VR15 Touch On !!");
		}
	}
	else
	{
		if (isTouching)
		{
			isTouching = false;
			//      touchOff();
			SET_LED(LED263R, ledoff);
			SET_LED(LED410G, ledoff);
			Serial.println("VR15 Touch Off !!");
			touch_SQN = vr0;
			MODE = ALLLEDON;
		}
	}
}

/*
***************************************
scan touch
2024/11/22 程式設計:李進衛
2024/11/22 重新改寫觸控演算法
***************************************
*/

void touch_scan()
{
	//	if (touchOK)
	//		return;
	switch (touch_SQN)
	{
	case vr0:
	SET_LED(LED180R, blink1hz);
		tch_vr0();
		break;
	case vr1:
	SET_LED(LED186R, blink1hz);
		tch_vr1();
		break;

	case vr2:
	SET_LED(LED192R, blink1hz);
		tch_vr2();
		break;

	case vr3:
	SET_LED(LED198R, blink1hz);
		tch_vr3();
		break;

	case vr4:
	SET_LED(LED204R, blink1hz);
		tch_vr4();
		break;
	case vr5:
	SET_LED(LED210R, blink1hz);
		tch_vr5();
		break;

	case vr6:
	SET_LED(LED215R, blink1hz);
		tch_vr6();
		break;

	case vr7:
	SET_LED(LED219R, blink1hz);
		tch_vr7();
		break;
	case vr8:
	SET_LED(LED225R, blink1hz);
		tch_vr8();
		break;

	case vr9:
	SET_LED(LED231R, blink1hz);
		tch_vr9();
		break;

	case vr10:
	SET_LED(LED237R, blink1hz);
		tch_vr10();
		break;

	case vr11:
	SET_LED(LED243R, blink1hz);
		tch_vr11();
		break;

	case vr12:
	SET_LED(LED248R, blink1hz);
		tch_vr12();
		break;

	case vr13:
	SET_LED(LED253R, blink1hz);
		tch_vr13();
		break;

	case vr14:
	SET_LED(LED258R, blink1hz);
		tch_vr14();
		break;

	case vr15:
	SET_LED(LED263R, blink1hz);
		tch_vr15();
		break;
	}
}

void led_update()
{
	if (MODE == ALLLEDON || MODE == IDLE) // debug 2024/11/15
		return;

	I2C_W_2BYTE(SNLED2735_ADDRESS, 0xFD, 0x03);							// 選擇功能頁面 (Page 3)
	I2C_W_2BYTE(SNLED2735_ADDRESS, 0x00, 0x01);							// 設定 LED 驅動器為一般模式//關閉模式
	I2C_W_2BYTE(SNLED2735_ADDRESS, 0xFD, 0x00);							// 選擇 LED 控制頁面 (Page 0)
	I2C_W_LED_UPDATE(SNLED2735_ADDRESS, 0x00, 0x17, MAIN_LEFT_LED_MAP); // 更新LED狀態
																		//	readLEDRegister(); //讀出暫存器的資料確定是否有填入？
}

//*********************************
//***** 按鍵掃描
//***** 程式設計： 李進衛
//**********************************

void key_proce()
{
	for (int i = 0; i < cols; i++)
	{
		digitalWrite(colPins[i], LOW);
		for (int j = 0; j < rows; j++)
		{
			bool reading = !digitalRead(rowPins[j]);
			if (reading != lastKeyState[i][j])
			{
				lastDebounceTime[i][j] = millis();
			}
			if ((millis() - lastDebounceTime[i][j]) > debounceDelay)
			{ // 彈跳時間OK
				if (reading != keyState[i][j])
				{							  // 假如讀進來的值與按鍵的狀態不同就更新按鍵的狀態
					keyState[i][j] = reading; // 更新按鍵狀態
					if (keyState[i][j])
					{					   // 假如按鍵的狀態是ON
						executeTask(i, j); // 執行對應的程式
					}
				}
			}
			lastKeyState[i][j] = reading;
		}
		digitalWrite(colPins[i], HIGH);
		delayMicroseconds(300); // PORT 切換時要稍微delay 300uS 一下否則會誤判key code 2024/11/01 李進衛增加
	}
}

//************************************
//***** 計算VR的BAR長度
//***** 程式設計：李進衛 2024/11/2 建立
//************************************
/*
  int16_t adc0, adc1, adc2, adc3;
  float volts0, volts1, volts2, volts3;

  adc0 = ads.readADC_SingleEnded(0);
  adc1 = ads.readADC_SingleEnded(1);
  adc2 = ads.readADC_SingleEnded(2);
  adc3 = ads.readADC_SingleEnded(3);

  volts0 = ads.computeVolts(adc0);
  volts1 = ads.computeVolts(adc1);
  volts2 = ads.computeVolts(adc2);
  volts3 = ads.computeVolts(adc3);


*/
/*
  adc0 = ads.readADC_SingleEnded(0);
  adc1 = ads.readADC_SingleEnded(1);
  adc2 = ads.readADC_SingleEnded(2);
  adc3 = ads.readADC_SingleEnded(3);

  volts0 = ads.computeVolts(adc0);
  volts1 = ads.computeVolts(adc1);
  volts2 = ads.computeVolts(adc2);
  volts3 = ads.computeVolts(adc3);


*/

void vr_check(void)
{
	// 讀取VR電壓
	if (!vr9to15)
	{
		rawValue = ads.readADC_SingleEnded(0); // analogRead(POTSEL0);
		//		rawValue = rawValue>>6;
		voltage = ads.computeVolts(rawValue); //(rawValue / 1024.0) * MAX_VOLTAGE;

		rawValue1 = ads.readADC_SingleEnded(1); // analogRead(POTSEL1);
		//		rawValue1 = rawValue1 >>6;
		voltage1 = ads.computeVolts(rawValue1); //(rawValue1 / 1024.0) * MAX_VOLTAGE;
	}
	else
	{
		rawValue = ads.readADC_SingleEnded(2); // analogRead(POTSEL2);
		//		rawValue = rawValue>>6;

		voltage = ads.computeVolts(rawValue); //(rawValue / 1024.0) * MAX_VOLTAGE;

		rawValue1 = ads.readADC_SingleEnded(3); // analogRead(POTSEL3);
		//		rawValue1 = rawValue1 >>6;
		voltage1 = ads.computeVolts(rawValue1); //(rawValue1 / 1024.0) * MAX_VOLTAGE;
	}

	// 計算長條圖寬度
	int barWidth = map(voltage * 100, 0, MAX_VOLTAGE * 100, 0, BAR_MAX_WIDTH);
	int barWidth1 = map(voltage1 * 100, 0, MAX_VOLTAGE * 100, 0, BAR_MAX_WIDTH);
	// 更新OLED顯示
	display.clearDisplay();

	display.fillRect(0, 0, barWidth, bar_HEIGHT, SSD1306_WHITE);
	display.fillRect(0, 20, barWidth1, bar_HEIGHT, SSD1306_WHITE);
	display.setTextSize(1);				 // Normal 1:1 pixel scale
	display.setTextColor(SSD1306_WHITE); // Draw white text
	display.setCursor(0, 35);			 // Start at top-left corner
	display.print("Vol:");
	display.print(voltage, 4);
	display.println("V");
	display.setCursor(0, 50); // Start at top-left corner
	display.print("Vol1:");
	display.print(voltage1, 4);
	display.println("V");
	display.display();
	delay(20); // 適度延遲
}

void ledTest(void)
{
	switch (ledSqn)
	{
	case 0:
		All_ledon();	  // 所有LED開啟
		T_LED = 500 / 10; // 設定T_LED值 delay 5sec
		ledSqn = 1;		  // 將ledSqn設定為1
		Serial.println("Led mode Sqn 1");
		break;
	case 1:
		if (ledMod == true)
		{
			ledSqn = 2;
		}
		//				if (T_LED ==0) // 如果T_LED不為0時間還沒有到，退出switch語句
		//				{
		//					All_ledoff();
		//					T_LED = 500/10; //delay 500ms
		Serial.println("Led mode Sqn 2 led off");
		//				}
		break;
	case 2:
		//				if (T_LED ==0 )
		//				{
		//					ledSqn = 3;
		//				}
		MODE = VRTEST;
		ledSqn = 0;
		break;
		//		case 3:
		//				if (ledMod == true)
		//				{
		//					MODE = TOUCHTEST;
		//					ledSqn =0;
		//				} else{ ledSqn = 0;}
		//				break;
		//		case 4:
		//				Encode_process();
		//				break;
	default:
		break;
	}
}
/* 
*****************************
***** 李進衛建立 2024/11/22
***** debug 調校觸控參數
*****************************
*/
void touch_debug()
{
	selectTOUCH(ch0);
	delay(1);
	pwm_turnon();

	// 1. 測量頻率
	float measuredFrequency = measureFrequency(TCH_RET0);
	Serial.print("measuredFrequency is :");
	Serial.println(measuredFrequency);
	// 2. 更新濾波器
	updateFrequencyBuffer(measuredFrequency);

	// 3. 計算移動平均值
	filteredFrequency = calculateFilteredFrequency();

	// 4. 根據濾波後的頻率判定觸控狀態
	if (filteredFrequency > baseFrequency * (1 + touchThreshold / 100.0))
	{
		if (!isTouching)
		{
			isTouching = true;
			//      touchOn();
			SET_LED(LED180G, ledon);
			Serial.println("VR Touch On !!");
		}
	}
	else
	{
		if (isTouching)
		{
			isTouching = false;
			//      touchOff();
			SET_LED(LED180G, ledoff);
			Serial.println("VR Touch Off !!");
		}
	}

	delay(10); // 可根據實際需求調整
			   /*
				   selectTOUCH(ch0);
				   delay(1);
				   pwm_turnon();
				   bool result0 = 0;
				  lowTime0 = pulseIn(TCH_RET0, LOW);
				  Serial.print("Duty cycel - :");
				  Serial.println(lowTime0);
					highTime0 = pulseIn(TCH_RET0, HIGH);
				  Serial.print("Duty cycel + :");
				  Serial.println(highTime0);
				 unsigned long pulseDuration = highTime0 + lowTime0;                      //pulseIn(TCH_RET0, HIGH,10000);
				 unsigned long frequency = 1000000 / pulseDuration;  //(2 * pulseDuration); // 計算頻率
				 Serial.print("VR test frequency: ");
				 Serial.println(frequency);
				   delay(500); */
}
/*
***************************
***** Main Loop
李進衛2024/10/27 建立
***************************
*/
void loop()
{
	handleLEDBlink(); // 工作燈
	led_update();	  // led 的狀態更新
	key_proce();
	NewCheckTime(); // 系統時間計算
	//	Encode_process();
	//	touch_debug();
//	touch_scan();		// 2024/11/22 更新觸控演算法
	switch (MODE)
	{
	case ALLLEDON:
		ledTest();
		break;
	case TOUCHTEST:
		touch_scan();
		break;

	case VRTEST:
		vr_check();
		break;
	case KEYTEST:
		break;
	case IDLE:
		break;
	}
}

/*
****************************************************************************************************************
計算任務索引的原因
在這個程式中，我們有一個6x6的鍵盤矩陣，總共有36個按鍵。每個按鍵對應一個特定的任務函數。
為了方便管理和調用這些任務函數，我們使用一個函數指針數組 tasks，其中每個元素都是一個指向特定任務函數的指針。
如何計算任務索引
我們需要一個方法來將每個按鍵的位置（行和列）映射到這個函數指針數組中的一個索引。這樣，我們就可以根據按鍵的位置來調用對應的任務函數。
假設我們有一個6x6的矩陣，行和列的索引範圍都是0到5。對於每個按鍵，我們可以使用以下公式計算它在函數指針數組中的索引：
這個公式的作用是將二維的行和列索引轉換為一維的數組索引。
具體例子
一個具體的例子來理解這個公式：
假設我們有一個按鍵在第2行第3列（row = 2, col = 3），我們可以計算它的任務索引如下：
taskIndex=2×6+3=12+3=15
這意味著這個按鍵對應的任務函數在 tasks 數組中的第15個位置。
為什麼這樣計算有效
這樣計算的好處是：
簡單明瞭：只需要一個公式就可以將二維索引轉換為一維索引。
高效：計算過程中只涉及簡單的乘法和加法運算。
易於管理：可以方便地管理和調用36個不同的任務函數。
李進衛2024/11/02星期六建立
****************************************************************************************************************
*/

void executeTask(int col, int row)
{
	int taskIndex = col * rows + row; // 計算任務索引
									  //  int taskIndex = col * cols + row; // 計算任務索引
	Serial.print("taskIndex: ");
	Serial.println(taskIndex);
	if (taskIndex >= 0 && taskIndex < 28)
	{
		tasks[taskIndex](); // 呼叫對應的任務函數
	}
}
