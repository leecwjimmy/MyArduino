/*
******************************
程式設計：李進衛
開發板： RP-2040
日期：2024/11/05
******************************

*/
#include <Wire.h>
#include <Adafruit_TinyUSB.h> //RP2040使用下載程式的USB 當作UART 傳輸資料

const int rows = 6;
const int cols = 6;
const int rowPins[rows] = {2, 3, 6, 7, 8, 9}; // 定義行引腳
const int colPins[cols] = {10, 11, 12, 13, 14, 15}; // 定義列引腳

bool keyState[rows][cols] = {false}; // 按鍵狀態
bool lastKeyState[rows][cols] = {false}; // 上一次按鍵狀態
unsigned long lastDebounceTime[rows][cols] = {0}; // 上一次去抖動時間
const unsigned long debounceDelay = 20; // 去抖動延遲時間
// 定義LED引腳
#define LED_PIN 25 //Raspberry  pi pico
// LED 閃爍變數
unsigned long ledLastToggleTime = 0;
bool ledState = false;
const unsigned long ledInterval = 500; // 2Hz 閃爍，每次切換狀態間隔 500ms
// **設定 SNLED2735
#define SNLED2735_ADDRESS 0x74 // **設定 SNLED2735 的 I2C 地址**
#define RST_LEDn_L 16  //reset pin
#define ledon 1
#define ledoff 0
#define debugPin 0
bool keyonoff[36] = {0}; //按鍵副程式旗標
byte MAIN_LEFT_LED_MAP[24] = {0}; // SNLED27351 LED 矩陣映射記憶體陣列開啟共24 byte

//**************************************
// 定義巨集來存取和修改特定LED的位置 
//2024/11/03
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
這個巨集能夠高效地在 LED_MAP 中設置或清除單個 LED 的狀態。
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

    // 設置led0為1 (點亮)
    SET_LED(led0, 1);
    Serial.println(GET_LED(led0)); // 應該輸出1

    // 設置led1為0 (熄滅)
    SET_LED(led1, 0);
    Serial.println(GET_LED(led1)); // 應該輸出0

    // 設置led5為1
    SET_LED(led5, 1);
    Serial.println(GET_LED(led5)); // 應該輸出1
}

void loop() {
    // 可在此放置其他需要操作LED的程式碼
}



*/

#define SET_LED(index, state) (MAIN_LEFT_LED_MAP[(index) / 8] = (state) ? (MAIN_LEFT_LED_MAP[(index) / 8] | (1 << ((index) % 8))) : (MAIN_LEFT_LED_MAP[(index) / 8] & ~(1 << ((index) % 8))))
#define GET_LED(index) ((MAIN_LEFT_LED_MAP[(index) / 8] >> ((index) % 8)) & 1)

// LED 定義
#define LED33 0
#define LED36 1
#define LED39 2
#define LED42 3
#define LED45 4
#define LED48 5
#define LED51 6
#define LED54 7
#define LED34 16
#define LED37 17
#define LED40 18
#define LED43 19
#define LED46 20
#define LED49 21
#define LED52 22
#define LED55 23
#define LED61 24
#define LED75 25
#define LED77 26
#define LED79 27

#define LED35 32
#define LED38 33
#define LED41 34
#define LED44 35
#define LED47 36
#define LED50 37
#define LED53 38 
#define LED56 39
#define LED74 40
#define LED76 41
#define LED78 42
#define LED80 43
#define LED83 44

#define LED345 48
#define LED346 49
#define LED348 50
#define LED349 51
#define LED350 52
#define LED351 53
#define LED123 54
#define LED130 55
#define LED137 56
#define LED143 57
#define LED145 58
#define LED150 59
#define LED347 60
#define LED155 61
#define LED162 62
#define LED169 63

#define LED84 64
#define LED88 65
#define LED93 66
#define LED339 67
#define LED340 68
#define LED341 69
#define LED342 70
#define LED118 71
#define LED124 72
#define LED129 73
#define LED131 74
#define LED136 75
#define LED154 76
#define LED156 77
#define LED161 78
#define LED168 79

#define LED87R  80
#define LED89  81
#define LED94  82
#define LED99  83
#define LED103  84
#define LED104  85
//#define LED  86	//空接
//#define LED  87   //空接
//#define LED  88   //空接
//#define LED  89   //空接
#define LED138  90
#define LED144  91
#define LED149  92
//#define LED  93	//空接
#define LED163  94
#define LED170  95

#define LED87G		96
#define LED90		97
#define LED95		98
#define LED100		99
#define LED105		100
#define LED109		101
#define LED114		102
#define LED119		103
#define LED125		104
#define LED132		105
#define LED139		106
//#define LED		107
//#define LED		108
#define LED157		109
#define LED164		110
#define LED171		111

#define LED87B	112
#define LED96	113
#define LED101	114
#define LED106	115
#define LED108	116
#define LED110	117
#define LED113	118
#define LED115	119
#define LED120	120
#define LED126	121
#define LED133	122
#define LED140	123
#define LED146	124
#define LED151	125
#define LED158	126
#define LED165	127

#define LED85	128
#define LED91	129
#define LED97	130
#define LED102	131
#define LED338	132
#define LED111	133
#define LED116	134
#define LED121	135
#define LED127	136
#define LED134	137
#define LED141	138
#define LED147	139
#define LED107	140
//#define LED	141
//#define LED	142
//#define LED	143

//************************************************
// I2C 寫入多個位元組資料的函數，使用陣列作為輸入
// 程式設計：李進衛
// 日期： 2024-11-04
// 功能： 將每一個LED的狀態傳入SNLED27351 中
//************************************************
void I2C_W_LED_UPDATE(byte chip_addr, byte start_reg_addr, byte end_reg_addr, byte data_array[]) {
  Wire.beginTransmission(chip_addr);
  Wire.write(start_reg_addr);
  // 假設 start_reg_addr 到 end_reg_addr 的範圍內共 (end_reg_addr - start_reg_addr + 1) 個位置
  for (byte i = 0; i <= (end_reg_addr - start_reg_addr); i++) {
    Wire.write(data_array[i]); // 從 data_array 中寫入資料
  }
  byte Err3 = Wire.endTransmission();
  if (Err3 != 0) {
    Serial.print("Device at address 0x");
    Serial.print(chip_addr, HEX);
    Serial.println(" is not responding.");
  }
}


// I2C 寫入兩個位元組資料的函數
void I2C_W_2BYTE(byte chip_addr, byte reg_addr, byte data) {
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
void I2C_W_NBYTE(byte chip_addr, byte start_reg_addr, byte end_reg_addr, byte data) {
  Wire.beginTransmission(chip_addr);
  Wire.write(start_reg_addr);
  for (byte i = start_reg_addr; i <= end_reg_addr; i++) {
    Wire.write(data);
  }
 byte Err2 =  Wire.endTransmission();
 if (Err2 != 0)
 {
  Serial.print("Device at address 0x");
Serial.print(chip_addr, HEX);
Serial.println(" is not responding.");
 }
}

// 讀取 LED 開路暫存器的函數
void readOpenRegister() {
  I2C_W_2BYTE(SNLED2735_ADDRESS, 0xFD, 0x00); // 選擇 LED 控制頁面 (Page 0)

  Wire.beginTransmission(SNLED2735_ADDRESS);
  Wire.write(0x18); // LED 開路暫存器的起始地址
  Wire.endTransmission(false); // 發送重複起始條件

  Wire.requestFrom(SNLED2735_ADDRESS, 24); // 讀取 24 個位元組的資料
  for (int i = 0; i < 24; i++) {
    byte data = Wire.read();
    // 處理讀取到的資料，例如：列印每個 LED 的開路狀態
    Serial.print("LED Open Register ");
    Serial.print(i, HEX);
    Serial.print(": ");
    Serial.println(data, BIN);
  }
}

// 讀取 LED 短路暫存器的函數
void readShortRegister() {
  I2C_W_2BYTE(SNLED2735_ADDRESS, 0xFD, 0x00); // 選擇 LED 控制頁面 (Page 0)

  Wire.beginTransmission(SNLED2735_ADDRESS);
  Wire.write(0x30); // LED 短路暫存器的起始地址
  Wire.endTransmission(false); // 發送重複起始條件// 維持通訊以便接下來讀取

  Wire.requestFrom(SNLED2735_ADDRESS, 24); // 讀取 24 個位元組的資料
  for (int i = 0; i < 24; i++) {
    byte data = Wire.read();
    // 處理讀取到的資料，例如：列印每個 LED 的短路狀態
    Serial.print("LED Short Register ");
    Serial.print(i, HEX);
    Serial.print(": ");
    Serial.println(data, BIN);
  }
}


// 處理 LED 以 2Hz 閃爍
void handleLEDBlink() {
  unsigned long currentMillis = millis();
  if (currentMillis - ledLastToggleTime >= ledInterval) {
    ledLastToggleTime = currentMillis;
    ledState = !ledState; // 切換 LED 狀態
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
  }
}

void All_ledon(void)
{
  I2C_W_2BYTE(SNLED2735_ADDRESS, 0xFD, 0x03);  // 選擇功能頁面 (Page 3)
  I2C_W_2BYTE(SNLED2735_ADDRESS, 0x00, 0x01);  // 設定 LED 驅動器為一般模式//關閉模式  
  I2C_W_2BYTE(SNLED2735_ADDRESS, 0xFD, 0x00);  // 選擇 LED 控制頁面 (Page 0)
  I2C_W_NBYTE(SNLED2735_ADDRESS, 0x00, 0x17, 0xFF);  // 清除 LED 控制暫存器 0x00~0x47 的資料
}

void All_ledoff(void)
{
  I2C_W_2BYTE(SNLED2735_ADDRESS, 0xFD, 0x03);  // 選擇功能頁面 (Page 3)
  I2C_W_2BYTE(SNLED2735_ADDRESS, 0x00, 0x01);  // 設定 LED 驅動器為一般模式//關閉模式}
  I2C_W_2BYTE(SNLED2735_ADDRESS, 0xFD, 0x00);  // 選擇 LED 控制頁面 (Page 0)
  I2C_W_NBYTE(SNLED2735_ADDRESS, 0x00, 0x17, 0x00);  // 清除 LED 控制暫存器 0x00~0x47 的資料
}

// 定義36個任務函數
void task0() {
	
Serial.println("Task 0 executed LED OPEN TEST!!");
	if (keyonoff[0] ==0)
	{
		SET_LED(LED146, ledon);
		SET_LED(LED151, ledon);
		SET_LED(LED134, ledon);
		SET_LED(LED127, ledon);

//		All_ledon();
//		readOpenRegister();	//讀取LED OPEN的狀態 
		keyonoff[0] = 1;
	} else {
		SET_LED(LED146, ledoff);
		SET_LED(LED151, ledoff);
		SET_LED(LED134, ledoff);
		SET_LED(LED127, ledoff);
//		All_ledoff();
		keyonoff[0] = 0;
		
	}
}
void task1() { 
Serial.println("Task 1 executed LED SHORT TEST !!");


	if (keyonoff[1] ==0)
	{
		SET_LED(LED141, ledon);
		SET_LED(LED147, ledon);
		SET_LED(LED158, ledon);
		SET_LED(LED165, ledon);
//		All_ledon();
//  readShortRegister();	//讀取LED SHORT 的狀態 		
		keyonoff[1] = 1;
	} else {
		SET_LED(LED141, ledoff);
		SET_LED(LED147, ledoff);
		SET_LED(LED158, ledoff);
		SET_LED(LED165, ledoff);
//		All_ledoff();
		keyonoff[1] = 0;
		
	}  
}
void task2() { 
Serial.println("Task 2 executed All LED ON !!");
	if (keyonoff[2] ==0)
	{
		SET_LED(LED116, ledon);
		SET_LED(LED126, ledon);
		SET_LED(LED133, ledon);

//		All_ledon();
		keyonoff[2] = 1;
	} else {
		SET_LED(LED116, ledoff);
		SET_LED(LED126, ledoff);
		SET_LED(LED133, ledoff);
//		All_ledoff();
		keyonoff[2] = 0;
		
	}   
//  All_ledon();
}
void task3() { 
Serial.println("Task 3 executed All LED OFF !!"); 
	if (keyonoff[3] ==0)
	{
		SET_LED(LED115, ledon);
		SET_LED(LED338, ledon);
//		All_ledon();
		keyonoff[3] = 1;
	} else {
		SET_LED(LED115, ledoff);
		SET_LED(LED338, ledoff);
//		All_ledoff();
		keyonoff[3] = 0;
		
	}  
}
void task4() { 
Serial.println("Task 4 executed I2C test !! ");
	if (keyonoff[4] ==0)
	{
		SET_LED(LED97, ledon);
		SET_LED(LED108, ledon);
		SET_LED(LED110, ledon);
//		All_ledon();
		keyonoff[4] = 1;
	} else {
		SET_LED(LED97, ledoff);
		SET_LED(LED108, ledoff);
		SET_LED(LED110, ledoff);
//		All_ledoff();
		keyonoff[4] = 0;
		
	}  

 
}
void task5() { 
Serial.println("Task 5 executed"); 
	if (keyonoff[5] ==0)
	{
		SET_LED(LED85, ledon);
		SET_LED(LED101, ledon);
//		All_ledon();
		keyonoff[5] = 1;
	} else {
		SET_LED(LED85, ledoff);
		SET_LED(LED101, ledoff);
//		All_ledoff();
		keyonoff[5] = 0;
		
	}  


}
//空接
void task6() { 
Serial.println("Task 6 executed"); 
	if (keyonoff[6] ==0)
	{
		All_ledon();
		keyonoff[6] = 1;
	} else {
		All_ledoff();
		keyonoff[6] = 0;
		
	}  
}
void task7() { 
Serial.println("Task 7 executed"); 
	if (keyonoff[7] ==0)
	{
		SET_LED(LED107, ledon);
		SET_LED(LED140, ledon);
//		All_ledon();
		keyonoff[7] = 1;
	} else {
		SET_LED(LED107, ledoff);
		SET_LED(LED140, ledoff);
//		All_ledoff();
		keyonoff[7] = 0;
		
	}}
void task8() { 
Serial.println("Task 8 executed");
	if (keyonoff[8] ==0)
	{
		SET_LED(LED120, ledon);
		SET_LED(LED121, ledon);
//		All_ledon();
		keyonoff[8] = 1;
	} else {
		SET_LED(LED120, ledoff);
		SET_LED(LED121, ledoff);
		All_ledoff();
		keyonoff[8] = 0;
		
	} 
}
void task9() { 
Serial.println("Task 9 executed");
	if (keyonoff[9] ==0)
	{
		SET_LED(LED111, ledon);
		SET_LED(LED113, ledon);
//		All_ledon();
		keyonoff[9] = 1;
	} else {
		SET_LED(LED111, ledoff);
		SET_LED(LED113, ledoff);
		
//		All_ledoff();
		keyonoff[9] = 0;
		
	} 
}
void task10() { 
Serial.println("Task 10 executed");
	if (keyonoff[10] ==0)
	{
		SET_LED(LED102, ledon);
		SET_LED(LED106, ledon);
//		All_ledon();
		keyonoff[10] = 1;
	} else {
		SET_LED(LED102, ledoff);
		SET_LED(LED106, ledoff);
//		All_ledoff();
		keyonoff[10] = 0;
		
	} 
}
void task11() { 
Serial.println("Task 11 executed");
	if (keyonoff[11] ==0)
	{
		SET_LED(LED91, ledon);
		SET_LED(LED96, ledon);
//		All_ledon();
		keyonoff[11] = 1;
	} else {
		SET_LED(LED91, ledoff);
		SET_LED(LED96, ledoff);
//		All_ledoff();
		keyonoff[11] = 0;
		
	} 
}
void task12() {  // 空接
Serial.println("Task 12 executed");
	if (keyonoff[12] ==0)
	{

//		All_ledon();
		keyonoff[12] = 1;
	} else {

//		All_ledoff();
		keyonoff[12] = 0;
		
	} 
}
void task13() { 
Serial.println("Task 13 executed"); 
	if (keyonoff[13] ==0)
	{
		SET_LED(LED164, ledon);
		SET_LED(LED170, ledon);
		SET_LED(LED171, ledon);		
//		All_ledon();
		keyonoff[13] = 1;
	} else {
		SET_LED(LED164, ledoff);
		SET_LED(LED170, ledoff);
		SET_LED(LED171, ledoff);//		All_ledoff();
		keyonoff[13] = 0;
		
	}
}
void task14() { 
Serial.println("Task 14 executed");
	if (keyonoff[14] ==0)
	{
		SET_LED(LED139, ledon);
		SET_LED(LED144, ledon);
		SET_LED(LED149, ledon);

//		All_ledon();
		keyonoff[14] = 1;
	} else {
		SET_LED(LED139, ledoff);
		SET_LED(LED144, ledoff);
		SET_LED(LED149, ledoff);
//		All_ledoff();
		keyonoff[14] = 0;
		
	} 
}
void task15() { 
Serial.println("Task 15 executed");
	if (keyonoff[15] ==0)
	{
		SET_LED(LED103, ledon);
		SET_LED(LED104, ledon);
		SET_LED(LED125, ledon);
//		All_ledon();
		keyonoff[15] = 1;
	} else {
		SET_LED(LED103, ledoff);
		SET_LED(LED104, ledoff);
		SET_LED(LED125, ledoff);
//		All_ledoff();
		keyonoff[15] = 0;
		
	} 
}
void task16() { 
Serial.println("Task 16 executed");
	if (keyonoff[16] ==0)
	{
		SET_LED(LED109, ledon);
		SET_LED(LED114, ledon);
//		All_ledon();
		keyonoff[16] = 1;
	} else {
		SET_LED(LED109, ledoff);
		SET_LED(LED114, ledoff);
//		All_ledoff();
		keyonoff[16] = 0;
		
	} 
}
void task17() { 
Serial.println("Task 17 executed"); 
	if (keyonoff[17] ==0)
	{
		SET_LED(LED87R, ledon);
		SET_LED(LED87G, ledon);
		SET_LED(LED87B, ledon);
		
//		All_ledon();
		keyonoff[17] = 1;
	} else {
		SET_LED(LED87R, ledoff);
		SET_LED(LED87G, ledoff);
		SET_LED(LED87B, ledoff);
//		All_ledoff();
		keyonoff[17] = 0;
		
	}
}
void task18() { //空接
Serial.println("Task 18 executed");
	if (keyonoff[18] ==0)
	{
		All_ledon();
		keyonoff[18] = 1;
	} else {
		All_ledoff();
		keyonoff[18] = 0;
		
	} 
}
void task19() { 
Serial.println("Task 19 executed");
	if (keyonoff[19] ==0)
	{
		SET_LED(LED163, ledon);
//		All_ledon();
		keyonoff[19] = 1;
	} else {
		SET_LED(LED163, ledoff);
//		All_ledoff();
		keyonoff[19] = 0;
		
	} 
}
void task20() { 
Serial.println("Task 20 executed"); 
	if (keyonoff[20] ==0)
	{
		SET_LED(LED132, ledon);
		SET_LED(LED138, ledon);
//		All_ledon();
		keyonoff[20] = 1;
	} else {
		SET_LED(LED132, ledoff);
		SET_LED(LED138, ledoff);
//		All_ledoff();
		keyonoff[20] = 0;
		
	}
}
void task21() { 
Serial.println("Task 21 executed");
	if (keyonoff[21] ==0)
	{
		SET_LED(LED99, ledon);
		SET_LED(LED119, ledon);
//		All_ledon();
		keyonoff[21] = 1;
	} else {
		SET_LED(LED99, ledoff);
		SET_LED(LED119, ledoff);
//		All_ledoff();
		keyonoff[21] = 0;
		
	} 
}
void task22() { 
Serial.println("Task 22 executed"); 
	if (keyonoff[22] ==0)
	{
		SET_LED(LED94, ledon);
		SET_LED(LED100, ledon);
		SET_LED(LED105, ledon);
//		All_ledon();
		keyonoff[22] = 1;
	} else {
		SET_LED(LED94, ledoff);
		SET_LED(LED100, ledoff);
		SET_LED(LED105, ledoff);
//		All_ledoff();
		keyonoff[22] = 0;
		
	}
}
void task23() { 
Serial.println("Task 23 executed");
	if (keyonoff[23] ==0)
	{
		SET_LED(LED89, ledon);
		SET_LED(LED90, ledon);
		SET_LED(LED95, ledon);
//		All_ledon();
		keyonoff[23] = 1;
	} else {
		SET_LED(LED89, ledoff);
		SET_LED(LED90, ledoff);
		SET_LED(LED95, ledoff);
//		All_ledoff();
		keyonoff[23] = 0;
		
	} 
}
void task24() { //空接
Serial.println("Task 24 executed"); 
	if (keyonoff[24] ==0)
	{
		All_ledon();
		keyonoff[24] = 1;
	} else {
		All_ledoff();
		keyonoff[24] = 0;
		
	}
}
void task25() { 
Serial.println("Task 25 executed");
	if (keyonoff[25] ==0)
	{
		SET_LED(LED157, ledon);
		SET_LED(LED161, ledon);
		SET_LED(LED168, ledon);
//		All_ledon();
		keyonoff[25] = 1;
	} else {
		SET_LED(LED157, ledoff);
		SET_LED(LED161, ledoff);
		SET_LED(LED168, ledoff);
//		All_ledoff();
		keyonoff[25] = 0;
		
	} 
}
void task26() { 
Serial.println("Task 26 executed");
	if (keyonoff[26] ==0)
	{
		SET_LED(LED154, ledon);
		SET_LED(LED155, ledon);
		SET_LED(LED347, ledon);
//		All_ledon();
		keyonoff[26] = 1;
	} else {
		SET_LED(LED154, ledoff);
		SET_LED(LED155, ledoff);
		SET_LED(LED347, ledoff);
//		All_ledoff();
		keyonoff[26] = 0;
		
	} 
}
void task27() { 
Serial.println("Task 27 executed");
	if (keyonoff[27] ==0)
	{
		SET_LED(LED124, ledon);
		SET_LED(LED129, ledon);
		SET_LED(LED137, ledon);
		SET_LED(LED143, ledon);
//		All_ledon();
		keyonoff[27] = 1;
	} else {
		SET_LED(LED124, ledoff);
		SET_LED(LED129, ledoff);
		SET_LED(LED137, ledoff);
		SET_LED(LED143, ledoff);
//		All_ledoff();
		keyonoff[27] = 0;
		
	} 
}
void task28() { 
Serial.println("Task 28 executed");
	if (keyonoff[28] ==0)
	{
		SET_LED(LED351, ledon);
		SET_LED(LED340, ledon);
		SET_LED(LED123, ledon);
		SET_LED(LED341, ledon);		
//		All_ledon();
		keyonoff[28] = 1;
	} else {
		SET_LED(LED351, ledoff);
		SET_LED(LED340, ledoff);
		SET_LED(LED123, ledoff);
		SET_LED(LED341, ledoff);		
//		All_ledoff();
		keyonoff[28] = 0;
		
	} 
}
void task29() { 
Serial.println("Task 29 executed"); 
	if (keyonoff[29] ==0)
	{
		SET_LED(LED349, ledon);
		SET_LED(LED350, ledon);
		SET_LED(LED88, ledon);
		SET_LED(LED93, ledon);		
//		All_ledon();
		keyonoff[29] = 1;
	} else {
		SET_LED(LED349, ledoff);
		SET_LED(LED350, ledoff);
		SET_LED(LED88, ledoff);
		SET_LED(LED93, ledoff);		
//		All_ledoff();
		keyonoff[29] = 0;
		
	}
}
void task30() { //空接
Serial.println("Task 30 executed");
	if (keyonoff[30] ==0)
	{
		All_ledon();
		keyonoff[30] = 1;
	} else {
		All_ledoff();
		keyonoff[30] = 0;
		
	} 
}
void task31() { 
Serial.println("Task 31 executed");
	if (keyonoff[31] ==0)
	{
		SET_LED(LED156, ledon);
		SET_LED(LED162, ledon);
		SET_LED(LED169, ledon);
//		All_ledon();
		keyonoff[31] = 1;
	} else {
		SET_LED(LED156, ledoff);
		SET_LED(LED162, ledoff);
		SET_LED(LED169, ledoff);
//		All_ledoff();
		keyonoff[31] = 0;
		
	} 
}
void task32() { 
Serial.println("Task 32 executed");
	if (keyonoff[32] ==0)
	{
		SET_LED(LED145, ledon);
		SET_LED(LED150, ledon);
		SET_LED(LED131, ledon);
		SET_LED(LED136, ledon);		
//		All_ledon();
		keyonoff[32] = 1;
	} else {
		SET_LED(LED145, ledoff);
		SET_LED(LED150, ledoff);
		SET_LED(LED131, ledoff);
		SET_LED(LED136, ledoff);		
//		All_ledoff();
		keyonoff[32] = 0;
		
	} 
}
void task33() { 
Serial.println("Task 33 executed"); 
	if (keyonoff[33] ==0)
	{
		SET_LED(LED130, ledon);
		SET_LED(LED118, ledon);
		SET_LED(LED342, ledon);
//		All_ledon();
		keyonoff[33] = 1;
	} else {
		SET_LED(LED130, ledoff);
		SET_LED(LED118, ledoff);
		SET_LED(LED342, ledoff);
//		All_ledoff();
		keyonoff[33] = 0;
		
	}
}
void task34() { 
Serial.println("Task 34 executed"); 
	if (keyonoff[34] ==0)
	{
		SET_LED(LED339, ledon);
		SET_LED(LED348, ledon);
//		All_ledon();
		keyonoff[34] = 1;
	} else {
		SET_LED(LED339, ledoff);
		SET_LED(LED348, ledoff);
//		All_ledoff();
		keyonoff[34] = 0;
		
	}
}
void task35() { 
Serial.println("Task 35 executed");
	if (keyonoff[35] ==0)
	{
		SET_LED(LED84, ledon);
		SET_LED(LED345, ledon);
		SET_LED(LED346, ledon);
//		All_ledon();
		keyonoff[35] = 1;
	} else {
		SET_LED(LED84, ledoff);
		SET_LED(LED345, ledoff);
		SET_LED(LED346, ledoff);
//		All_ledoff();
		keyonoff[35] = 0;
		
	} 
}

// 創建函數指針數組
void (*tasks[])() = {
  task0,  task1,  task2,  task3,  task4,  task5,  task6,  task7,  task8,
  task9,  task10,  task11,  task12,  task13,  task14,  task15,  task16,
  task17,  task18,  task19,  task20,  task21,  task22,  task23,  task24,
  task25,  task26,  task27,  task28,  task29,  task30,  task31,  task32,
  task33,  task34,  task35
};

void setup() {
    Wire.begin(); //(D2,D1);  //(0,2); //sda=0 | D3, scl=2 | D4
  Wire.setClock(100000); // 設定 i2c 的速率為 100 KHz RP2040 很重要不然會導致通信錯誤！！ 導線太長速度太快會導致資料錯誤！！！2024/11/03 by leecw


  Serial.begin(115200);

	
  pinMode(LED_PIN, OUTPUT); // 工作LED閃爍 初始化
  pinMode(debugPin, OUTPUT); // 工作LED閃爍 初始化
	digitalWrite(debugPin, HIGH);
	
  for (int i = 0; i < cols; i++) {
    pinMode(colPins[i], OUTPUT);
    digitalWrite(rowPins[i], HIGH);
  }
  for (int i = 0; i < rows; i++) {
    pinMode(rowPins[i], INPUT_PULLUP);
  }
  // 對SNLES27351 reset
	pinMode(RST_LEDn_L,OUTPUT);
	digitalWrite(RST_LEDn_L,LOW);
	delay(10);
	digitalWrite(RST_LEDn_L,HIGH);
	delay(25); // SNLED27351 datasheet 規定reset 之後必須間隔20ms以上才能動作
	
    // 設定 LED 驅動器的初始值，包括功能頁面、LED 控制頁面、PWM 頁面和電流頁面
  I2C_W_2BYTE(SNLED2735_ADDRESS, 0xFD, 0x03);  // 選擇功能頁面 (Page 3)
  I2C_W_2BYTE(SNLED2735_ADDRESS, 0x00, 0x01);  // 設定 LED 驅動器為一般模式//關閉模式
  I2C_W_2BYTE(SNLED2735_ADDRESS, 0x13, 0xAA);  // 設定內部通道下拉/上拉電阻
  I2C_W_2BYTE(SNLED2735_ADDRESS, 0x14, 0x00);  // 選擇掃描相位為 CB1~CB12
  I2C_W_2BYTE(SNLED2735_ADDRESS, 0x15, 0x04);  // 設定 PWM 延遲相位啟用
  I2C_W_2BYTE(SNLED2735_ADDRESS, 0x16, 0xC0);  // 設定 CA/CB 通道擺率控制啟用
//  I2C_W_2BYTE(SNLED2735_ADDRESS, 0x17, 0xC0);  // 設定 OPEN/SHORT 偵測啟用
  
  I2C_W_2BYTE(SNLED2735_ADDRESS, 0x1A, 0x00);  // 設定 Iref 模式禁用

  I2C_W_2BYTE(SNLED2735_ADDRESS, 0xFD, 0x00);  // 選擇 LED 控制頁面 (Page 0)
  I2C_W_NBYTE(SNLED2735_ADDRESS, 0x00, 0x47, 0x00);  // 清除 LED 控制暫存器 0x00~0x47 的資料

  I2C_W_2BYTE(SNLED2735_ADDRESS, 0xFD, 0x01);  // 選擇 PWM 頁面 (Page 1)
  I2C_W_NBYTE(SNLED2735_ADDRESS, 0x00, 0xBF, 0x80);  // 清除 PWM 暫存器 0x00~0xBF 的資料  這裡要設0x80才會亮這個很重要！！！！2024/11/03 by leecw 

  I2C_W_2BYTE(SNLED2735_ADDRESS, 0xFD, 0x04);  // 選擇電流頁面 (Page 4)
//  I2C_W_NBYTE(SNLED2735_ADDRESS, 0x00, 0x0B, 0x80);  // 設定 CCS 暫存器 Addr. 0x00~0x0B = 20mA
//  I2C_W_NBYTE(SNLED2735_ADDRESS, 0x00, 0x0B, 0x40);  // 設定 CCS 暫存器 Addr. 0x00~0x0B = 10mA (64*0.157)
  I2C_W_NBYTE(SNLED2735_ADDRESS, 0x00, 0x0B, 0x40);  // 設定 CCS 暫存器 Addr. 0x00~0x0B = 5mA (31*0.157)
  All_ledon();
  delay(2000);
  All_ledoff();
}

void loop() {
		handleLEDBlink(); //工作燈
  I2C_W_2BYTE(SNLED2735_ADDRESS, 0xFD, 0x03);  // 選擇功能頁面 (Page 3)
  I2C_W_2BYTE(SNLED2735_ADDRESS, 0x00, 0x01);  // 設定 LED 驅動器為一般模式//關閉模式  
  I2C_W_2BYTE(SNLED2735_ADDRESS, 0xFD, 0x00);  // 選擇 LED 控制頁面 (Page 0)
  I2C_W_LED_UPDATE(SNLED2735_ADDRESS, 0x00, 0x17, MAIN_LEFT_LED_MAP); // 更新LED狀態		
  for (int i = 0; i < rows; i++) {
    digitalWrite(colPins[i], LOW);
    for (int j = 0; j < rows; j++) {
      bool reading = !digitalRead(rowPins[j]);
      if (reading != lastKeyState[i][j]) {
        lastDebounceTime[i][j] = millis();
      }
      if ((millis() - lastDebounceTime[i][j]) > debounceDelay) {  //彈跳時間OK
        if (reading != keyState[i][j]) {   //假如讀進來的值與按鍵的狀態不同就更新按鍵的狀態
          keyState[i][j] = reading;       // 更新按鍵狀態
          if (keyState[i][j]) {           // 假如按鍵的狀態是ON
            executeTask(i, j);            // 執行對應的程式
          }
        }
      }
      lastKeyState[i][j] = reading;
    }
    digitalWrite(colPins[i], HIGH);
	delayMicroseconds(300); // PORT 切換時要稍微delay 300uS 一下否則會誤判key code 2024/11/01 李進衛增加
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
讓我們看一個具體的例子來理解這個公式：
假設我們有一個按鍵在第2行第3列（row = 2, col = 3），我們可以計算它的任務索引如下：
taskIndex=2×6+3=12+3=15
這意味著這個按鍵對應的任務函數在 tasks 數組中的第15個位置。
為什麼這樣計算有效
這樣計算的好處是：
簡單明瞭：只需要一個公式就可以將二維索引轉換為一維索引。
高效：計算過程中只涉及簡單的乘法和加法運算。
易於管理：可以方便地管理和調用36個不同的任務函數。
****************************************************************************************************************
*/

void executeTask(int col, int row) {
  int taskIndex = col * rows + row; // 計算任務索引
  Serial.print("taskIndex: ");
  Serial.println(taskIndex);
  if (taskIndex >= 0 && taskIndex < 36) {
    tasks[taskIndex](); // 呼叫對應的任務函數
  }
}
