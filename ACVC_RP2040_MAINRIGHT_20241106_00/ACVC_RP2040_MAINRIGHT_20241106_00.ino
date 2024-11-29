/*
******************************
程式設計：李進衛
開發板： RP-2040
日期：2024/11/05
機種： ACVC
基板： MAIN RIGHT 右主機板
57歲生日特別版本2024/11/06

******************************
在矩陣形式的鍵盤掃描電路中，ROW（行）和COL（列）分別代表鍵盤矩陣的行和列。

•  ROW（行）：這些是鍵盤矩陣中的橫向線路。當進行掃描時，行線通常設置為輸出，並依次被拉低以檢測是否有按鍵被按下。

•  COL（列）：這些是鍵盤矩陣中的縱向線路。列線通常設置為輸入，並監測行線拉低時的電平變化，以確定具體哪一個按鍵被按下
   這種行列掃描方式可以有效地檢測到矩陣中每個按鍵的狀態，並且能夠支持多按鍵同時按下的情況。
*/
#include <Wire.h>
#include <Adafruit_TinyUSB.h> //RP2040使用下載程式的USB 當作UART 傳輸資料

const int rows = 8;
const int cols = 6;
const int rowPins[rows] = {2, 3, 6, 7, 8, 9, 10, 11}; // 定義行引腳
const int colPins[cols] = {12, 13, 14, 15, 16, 17}; // 定義列引腳

bool keyState[cols][rows] = {false}; // 按鍵狀態
bool lastKeyState[cols][rows] = {false}; // 上一次按鍵狀態
bool reading = 0;
unsigned long lastDebounceTime[cols][rows] = {0}; // 上一次去抖動時間
const unsigned long debounceDelay = 20; // 去抖動延遲時間
// 定義LED引腳
#define LED_PIN 25 //Raspberry  pi pico
// LED 閃爍變數
unsigned long ledLastToggleTime = 0;
bool ledState = false;
const unsigned long ledInterval = 500; // 2Hz 閃爍，每次切換狀態間隔 500ms
// **設定 SNLED2735
#define SNLED2735_ADDRESS 0x75	//0x74 // **設定 SNLED2735 的 I2C 地址** 左板位址0x74 右板位址：0x75
#define RST_LEDn_L 18  //reset pin
#define ledon 1
#define ledoff 0
#define debugPin 0
bool keyonoff[48] = {0}; //按鍵副程式旗標
byte MAIN_LEFT_LED_MAP[24] = {0}; // SNLED27351 LED 矩陣映射記憶體陣列開啟共24 byte
int keychecksum = 0;

volatile int encoderValue = 0; // 旋轉編碼器計數值
int lastEncoded = 0;

#define CLK 19
#define DT 20
#define SW 4

int counter = 0;
int currentStateCLK;
int lastStateCLK;
//int currentStateSW;
//bool SWPressed = false;


//#define	EncoderA 19
//#define EncoderB 20
#define FS1 21
#define FS2 22
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
#define LED175 0
#define LED181 1
#define LED187 2
#define LED193 3
//#define LED 4
#define LED205 5
#define LED211 6
//#define LED 
//#define LED 
//#define LED 
#define LED232 10
#define LED238 11
#define LED244 12

#define LED176	16
#define LED182 	17
#define LED188  18
#define LED194  19
#define LED200	20
#define LED206	21
#define LED212	22
#define LED217	23 
#define LED221	24 
#define LED227	25 
#define LED233	26 
#define LED239	27 
#define LED245	28 
#define LED250	29  
#define LED255	30 
#define LED260	31
 
#define LED177	32 
#define LED183	33 
#define LED189	34 
#define LED195	35 
#define LED201	36
#define LED207  37
#define LED213  38
//#define LED     39
#define LED222  40
#define LED228  41
#define LED234  42
#define LED240  43
#define LED246  44
#define LED251  45
#define LED256  46
#define LED261  47

#define LED178	48
#define LED184	49
#define LED190	50
#define LED196	51
#define LED202	52
#define LED208	53
#define LED214	54
#define LED218	55
#define LED223	56
#define LED229	57
#define LED235	58
#define LED241	59
#define LED313		60
#define LED318		61
#define LED323		62
#define LED328		63

#define LED265		64
#define LED269		65
#define LED273		66
#define LED277		67
#define LED281		68
#define LED285		69
#define LED289		70
#define LED293		71
#define LED297		72
#define LED301		73
#define LED305		74
#define LED309      75
#define LED314      76
#define LED319      77
#define LED324      78
#define LED329      79

#define LED266	80
#define LED270  81
#define LED274  82
#define LED278  83
#define LED282	84
#define LED286	85
#define LED290	86
#define LED294	87
#define LED298	88
#define LED302	89
#define LED306	90
#define LED310	91
#define LED315	92
#define LED320	93
#define LED325	94
#define LED330	95

#define LED267	96
#define LED271	97
#define LED275	98
#define LED279	99

#define LED283	100
#define LED287	101
#define LED291	102
#define LED295	103
#define LED299	104
#define LED303	105
#define LED307	106
#define LED311	107
#define LED316	108
#define LED321	109
#define LED326	110
#define LED331	111

#define LED268	112
#define LED272	113
#define LED276	114	
#define LED280	115

#define LED284	116
#define LED288	117
#define LED292	118
#define LED296	119
#define LED300	120
#define LED304	121
#define LED308	122
#define LED312	123
#define LED317	124
#define LED322	125
#define LED327	126
#define LED332	127
#define LED224  136
#define LED230	137
//#define LED
//#define LED

void I2C_test(void)
{
  byte error, address;
  int nDevices;
 
//  SerialUSB.println("Scanning...");
  Serial.println("Scanning...");
 
  nDevices = 0;
  for(address = 1; address < 127; address++ ) 
  {
 
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
 
    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address<16) 
        Serial.print("0");
      Serial.print(address,HEX);
      Serial.println("  !");
 
      nDevices++;
    }
    else if (error==4) 
    {
      Serial.print("Unknow error at address 0x");
      if (address<16) 
        Serial.print("0");
      Serial.println(address,HEX);
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
		Serial.println("Task 0 ON!!");
//		SET_LED(LED146, ledon);
//		SET_LED(LED151, ledon);
//		SET_LED(LED134, ledon);
//		SET_LED(LED127, ledon);
		keyonoff[0] = 1;
	} else {
		Serial.println("Task 0 OFF!!");
//		SET_LED(LED146, ledoff);
//		SET_LED(LED151, ledoff);
//		SET_LED(LED134, ledoff);
//		SET_LED(LED127, ledoff);
		keyonoff[0] = 0;
		
	}
}
void task1() { 
Serial.println("Task 1 executed LED SHORT TEST !!");

	if (keyonoff[1] ==0)
	{
		Serial.println("Task 1 ON!!");
		SET_LED(LED260, ledon);
//		SET_LED(LED147, ledon);
//		SET_LED(LED158, ledon);
//		SET_LED(LED165, ledon);
		keyonoff[1] = 1;
	} else {
		Serial.println("Task 1 OFF!!");
		SET_LED(LED260, ledoff);
//		SET_LED(LED147, ledoff);
//		SET_LED(LED158, ledoff);
//		SET_LED(LED165, ledoff);
//		All_ledoff();
		keyonoff[1] = 0;
		
	}  
}
void task2() { 
Serial.println("Task 2 executed All LED ON !!");
	if (keyonoff[2] ==0)
	{
		Serial.println("Task 2 ON!!");
		SET_LED(LED255, ledon);
//		SET_LED(LED126, ledon);
//		SET_LED(LED133, ledon);
		keyonoff[2] = 1;
	} else {
		Serial.println("Task 2 OFF!!");
		SET_LED(LED255, ledoff);
//		SET_LED(LED126, ledoff);
//		SET_LED(LED133, ledoff);
		keyonoff[2] = 0;
		
	}   
}
void task3() { 
Serial.println("Task 3 executed All LED OFF !!"); 
	if (keyonoff[3] ==0)
	{
		SET_LED(LED229, ledon);
		SET_LED(LED256, ledon);
		keyonoff[3] = 1;
	} else {
		SET_LED(LED229, ledoff);
		SET_LED(LED256, ledoff);
		keyonoff[3] = 0;
		
	}  
}
void task4() { 
Serial.println("Task 4 executed I2C test !! ");
	if (keyonoff[4] ==0)
	{
		SET_LED(LED241, ledon);
		SET_LED(LED235, ledon);
		SET_LED(LED230, ledon);
		SET_LED(LED224, ledon);
		SET_LED(LED246, ledon);		
		keyonoff[4] = 1;
	} else {
		SET_LED(LED241, ledoff);
		SET_LED(LED235, ledoff);
		SET_LED(LED230, ledoff);
		SET_LED(LED224, ledoff);
		SET_LED(LED246, ledoff);		
//		SET_LED(LED110, ledoff);
		keyonoff[4] = 0;
		
	}  

 
}
void task5() { 
Serial.println("Task 5 executed"); 
	if (keyonoff[5] ==0)
	{
		SET_LED(LED190, ledon);
		SET_LED(LED234, ledon);
//		All_ledon();
		keyonoff[5] = 1;
	} else {
		SET_LED(LED190, ledoff);
		SET_LED(LED234, ledoff);
//		All_ledoff();
		keyonoff[5] = 0;
		
	}  


}

void task6() { 
Serial.println("Task 6 executed"); 
	if (keyonoff[6] ==0)
	{
		SET_LED(LED188, ledon);
		SET_LED(LED187, ledon);
		SET_LED(LED189, ledon);
//		All_ledon();
		keyonoff[6] = 1;
	} else {
		SET_LED(LED188, ledoff);
		SET_LED(LED187, ledoff);
		SET_LED(LED189, ledoff);
		//		All_ledoff();
		keyonoff[6] = 0;
		
	}  
}
void task7() { 
Serial.println("Task 7 executed"); 
	if (keyonoff[7] ==0)
	{
//		SET_LED(LED107, ledon);
//		SET_LED(LED140, ledon);
//		All_ledon();
		keyonoff[7] = 1;
	} else {
//		SET_LED(LED107, ledoff);
//		SET_LED(LED140, ledoff);
//		All_ledoff();
		keyonoff[7] = 0;
		
	}}
void task8() { 
Serial.println("Task 8 executed");
	if (keyonoff[8] ==0)
	{
//		SET_LED(LED120, ledon);
//		SET_LED(LED121, ledon);
//		All_ledon();
		keyonoff[8] = 1;
	} else {
//		SET_LED(LED120, ledoff);
//		SET_LED(LED121, ledoff);
//		All_ledoff();
		keyonoff[8] = 0;
		
	} 
}
void task9() { 
Serial.println("Task 9 executed");
	if (keyonoff[9] ==0)
	{
		SET_LED(LED223, ledon);
//		SET_LED(LED113, ledon);
//		All_ledon();
		keyonoff[9] = 1;
	} else {
		SET_LED(LED223, ledoff);
//		SET_LED(LED113, ledoff);
		
//		All_ledoff();
		keyonoff[9] = 0;
		
	} 
}
void task10() { 
Serial.println("Task 10 executed");
	if (keyonoff[10] ==0)
	{
		SET_LED(LED218, ledon);
//		SET_LED(LED106, ledon);
//		All_ledon();
		keyonoff[10] = 1;
	} else {
		SET_LED(LED218, ledoff);
//		SET_LED(LED106, ledoff);
//		All_ledoff();
		keyonoff[10] = 0;
		
	} 
}
void task11() { 
Serial.println("Task 11 executed");
	if (keyonoff[11] ==0)
	{
		SET_LED(LED214, ledon);
//		SET_LED(LED96, ledon);
//		All_ledon();
		keyonoff[11] = 1;
	} else {
		SET_LED(LED214, ledoff);
//		SET_LED(LED96, ledoff);
//		All_ledoff();
		keyonoff[11] = 0;
		
	} 
}
void task12() {  // 空接
Serial.println("Task 12 executed");
	if (keyonoff[12] ==0)
	{

		SET_LED(LED208, ledon);
//		All_ledon();
		keyonoff[12] = 1;
	} else {

		SET_LED(LED208, ledoff);
//		All_ledoff();
		keyonoff[12] = 0;
		
	} 
}
void task13() { 
Serial.println("Task 13 executed"); 
	if (keyonoff[13] ==0)
	{
		SET_LED(LED202, ledon);
//		SET_LED(LED170, ledon);
//		SET_LED(LED171, ledon);		
//		All_ledon();
		keyonoff[13] = 1;
	} else {
		SET_LED(LED202, ledoff);
//		SET_LED(LED170, ledoff);
//		SET_LED(LED171, ledoff);//		All_ledoff();
		keyonoff[13] = 0;
		
	}
}
void task14() { 
Serial.println("Task 14 executed");
	if (keyonoff[14] ==0)
	{
		SET_LED(LED196, ledon);
//		SET_LED(LED144, ledon);
//		SET_LED(LED149, ledon);

//		All_ledon();
		keyonoff[14] = 1;
	} else {
		SET_LED(LED196, ledoff);
//		SET_LED(LED144, ledoff);
//		SET_LED(LED149, ledoff);
//		All_ledoff();
		keyonoff[14] = 0;
		
	} 
}
void task15() { 
Serial.println("Task 15 executed");
	if (keyonoff[15] ==0)
	{
		SET_LED(LED178, ledon);
		SET_LED(LED213, ledon);
//		SET_LED(LED125, ledon);
//		All_ledon();
		keyonoff[15] = 1;
	} else {
		SET_LED(LED178, ledoff);
		SET_LED(LED213, ledoff);
//		SET_LED(LED125, ledoff);
//		All_ledoff();
		keyonoff[15] = 0;
		
	} 
}
void task16() { 
Serial.println("Task 16 executed");
	if (keyonoff[16] ==0)
	{
		SET_LED(LED323, ledon);
		SET_LED(LED328, ledon);
//		All_ledon();
		keyonoff[16] = 1;
	} else {
		SET_LED(LED323, ledoff);
		SET_LED(LED328, ledoff);
//		All_ledoff();
		keyonoff[16] = 0;
		
	} 
}
void task17() { 
Serial.println("Task 17 executed"); 
	if (keyonoff[17] ==0)
	{
		SET_LED(LED221, ledon);
//		SET_LED(LED87G, ledon);
//		SET_LED(LED87B, ledon);
		
//		All_ledon();
		keyonoff[17] = 1;
	} else {
		SET_LED(LED221, ledoff);
//		SET_LED(LED87G, ledoff);
//		SET_LED(LED87B, ledoff);
//		All_ledoff();
		keyonoff[17] = 0;
		
	}
}
void task18() { 
Serial.println("Task 18 executed");
	if (keyonoff[18] ==0)
	{
		SET_LED(LED227, ledon);
//		All_ledon();
		keyonoff[18] = 1;
	} else {
		SET_LED(LED227, ledoff);
//		All_ledoff();
		keyonoff[18] = 0;
		
	} 
}
void task19() { 
Serial.println("Task 19 executed");
	if (keyonoff[19] ==0)
	{
		SET_LED(LED212, ledon);
//		All_ledon();
		keyonoff[19] = 1;
	} else {
		SET_LED(LED212, ledoff);
//		All_ledoff();
		keyonoff[19] = 0;
		
	} 
}
void task20() { 
Serial.println("Task 20 executed"); 
	if (keyonoff[20] ==0)
	{
		SET_LED(LED217, ledon);
//		SET_LED(LED138, ledon);
//		All_ledon();
		keyonoff[20] = 1;
	} else {
		SET_LED(LED217, ledoff);
//		SET_LED(LED138, ledoff);
//		All_ledoff();
		keyonoff[20] = 0;
		
	}
}
void task21() { 
Serial.println("Task 21 executed");
	if (keyonoff[21] ==0)
	{
		SET_LED(LED200, ledon);
//		SET_LED(LED119, ledon);
//		All_ledon();
		keyonoff[21] = 1;
	} else {
		SET_LED(LED200, ledoff);
//		SET_LED(LED119, ledoff);
//		All_ledoff();
		keyonoff[21] = 0;
		
	} 
}
void task22() { 
Serial.println("Task 22 executed"); 
	if (keyonoff[22] ==0)
	{
		SET_LED(LED206, ledon);
//		SET_LED(LED100, ledon);
//		SET_LED(LED105, ledon);
//		All_ledon();
		keyonoff[22] = 1;
	} else {
		SET_LED(LED206, ledoff);
//		SET_LED(LED100, ledoff);
//		SET_LED(LED105, ledoff);
//		All_ledoff();
		keyonoff[22] = 0;
		
	}
}
void task23() { 
Serial.println("Task 23 executed");
	if (keyonoff[23] ==0)
	{
		SET_LED(LED184, ledon);
		SET_LED(LED201, ledon);
		SET_LED(LED207, ledon);
//		All_ledon();
		keyonoff[23] = 1;
	} else {
		SET_LED(LED184, ledoff);
		SET_LED(LED201, ledoff);
		SET_LED(LED207, ledoff);
//		All_ledoff();
		keyonoff[23] = 0;
		
	} 
}
void task24() { //空接
Serial.println("Task 24 executed"); 
	if (keyonoff[24] ==0)
	{
		SET_LED(LED313, ledon);
		SET_LED(LED318, ledon);
//		SET_LED(LED207, ledon);
//		All_ledon();
		keyonoff[24] = 1;
	} else {
		SET_LED(LED313, ledoff);
		SET_LED(LED318, ledoff);
//		SET_LED(LED207, ledoff);
//		All_ledoff();
		keyonoff[24] = 0;
		
	}
}
void task25() { 
Serial.println("Task 25 executed");
	if (keyonoff[25] ==0)
	{
		SET_LED(LED250, ledon);
//		SET_LED(LED161, ledon);
//		SET_LED(LED168, ledon);
//		All_ledon();
		keyonoff[25] = 1;
	} else {
		SET_LED(LED250, ledoff);
//		SET_LED(LED161, ledoff);
//		SET_LED(LED168, ledoff);
//		All_ledoff();
		keyonoff[25] = 0;
		
	} 
}
void task26() { 
Serial.println("Task 26 executed");
	if (keyonoff[26] ==0)
	{
		SET_LED(LED233, ledon);
//		SET_LED(LED155, ledon);
//		SET_LED(LED347, ledon);
//		All_ledon();
		keyonoff[26] = 1;
	} else {
		SET_LED(LED233, ledoff);
//		SET_LED(LED155, ledoff);
//		SET_LED(LED347, ledoff);
//		All_ledoff();
		keyonoff[26] = 0;
		
	} 
}
void task27() { 
Serial.println("Task 27 executed");
	if (keyonoff[27] ==0)
	{
		SET_LED(LED266, ledon);
//		SET_LED(LED129, ledon);
//		SET_LED(LED137, ledon);
//		SET_LED(LED143, ledon);
//		All_ledon();
		keyonoff[27] = 1;
	} else {
		SET_LED(LED266, ledoff);
//		SET_LED(LED129, ledoff);
//		SET_LED(LED137, ledoff);
//		SET_LED(LED143, ledoff);
//		All_ledoff();
		keyonoff[27] = 0;
		
	} 
}
void task28() { 
Serial.println("Task 28 executed");
	if (keyonoff[28] ==0)
	{
		SET_LED(LED239, ledon);
//		SET_LED(LED340, ledon);
//		SET_LED(LED123, ledon);
//		SET_LED(LED341, ledon);		
//		All_ledon();
		keyonoff[28] = 1;
	} else {
		SET_LED(LED239, ledoff);
//		SET_LED(LED340, ledoff);
//		SET_LED(LED123, ledoff);
//		SET_LED(LED341, ledoff);		
//		All_ledoff();
		keyonoff[28] = 0;
		
	} 
}
void task29() { 
Serial.println("Task 29 executed"); 
	if (keyonoff[29] ==0)
	{
		SET_LED(LED245, ledon);
//		SET_LED(LED350, ledon);
//		SET_LED(LED88, ledon);
//		SET_LED(LED93, ledon);		
//		All_ledon();
		keyonoff[29] = 1;
	} else {
		SET_LED(LED245, ledoff);
//		SET_LED(LED350, ledoff);
//		SET_LED(LED88, ledoff);
//		SET_LED(LED93, ledoff);		
//		All_ledoff();
		keyonoff[29] = 0;
		
	}
}
void task30() { 
Serial.println("Task 30 executed");
	if (keyonoff[30] ==0)
	{
		SET_LED(LED261, ledon);
		SET_LED(LED238, ledon);
		SET_LED(LED232, ledon);		
//		All_ledon();
		keyonoff[30] = 1;
	} else {
		SET_LED(LED261, ledoff);
		SET_LED(LED238, ledoff);
		SET_LED(LED232, ledoff);		
//		All_ledoff();
		keyonoff[30] = 0;
		
	} 
}
void task31() { 
Serial.println("Task 31 executed");
	if (keyonoff[31] ==0)
	{
		SET_LED(LED211, ledon);
		SET_LED(LED205, ledon);
		SET_LED(LED251, ledon);
//		All_ledon();
		keyonoff[31] = 1;
	} else {
		SET_LED(LED211, ledoff);
		SET_LED(LED205, ledoff);
		SET_LED(LED251, ledoff);
//		All_ledoff();
		keyonoff[31] = 0;
		
	} 
}
void task32() { 
Serial.println("Task 32 executed");
	if (keyonoff[32] ==0)
	{
		SET_LED(LED327, ledon);
//		SET_LED(LED150, ledon);
//		SET_LED(LED131, ledon);
//		SET_LED(LED136, ledon);		
//		All_ledon();
		keyonoff[32] = 1;
	} else {
		SET_LED(LED327, ledoff);
//		SET_LED(LED150, ledoff);
//		SET_LED(LED131, ledoff);
//		SET_LED(LED136, ledoff);		
//		All_ledoff();
		keyonoff[32] = 0;
		
	} 
}
void task33() { 
Serial.println("Task 33 executed"); 
	if (keyonoff[33] ==0)
	{
		SET_LED(LED270, ledon);
//		SET_LED(LED118, ledon);
//		SET_LED(LED342, ledon);
//		All_ledon();
		keyonoff[33] = 1;
	} else {
		SET_LED(LED270, ledoff);
//		SET_LED(LED118, ledoff);
//		SET_LED(LED342, ledoff);
//		All_ledoff();
		keyonoff[33] = 0;
		
	}
}
void task34() { 
Serial.println("Task 34 executed"); 
	if (keyonoff[34] ==0)
	{
		SET_LED(LED274, ledon);
//		SET_LED(LED348, ledon);
//		All_ledon();
		keyonoff[34] = 1;
	} else {
		SET_LED(LED274, ledoff);
//		SET_LED(LED348, ledoff);
//		All_ledoff();
		keyonoff[34] = 0;
		
	}
}
void task35() { 
Serial.println("Task 35 executed");
	if (keyonoff[35] ==0)
	{
		SET_LED(LED278, ledon);
//		SET_LED(LED345, ledon);
//		SET_LED(LED346, ledon);
//		All_ledon();
		keyonoff[35] = 1;
	} else {
		SET_LED(LED278, ledoff);
//		SET_LED(LED345, ledoff);
//		SET_LED(LED346, ledoff);
//		All_ledoff();
		keyonoff[35] = 0;
		
	} 
}


void task36() { 
Serial.println("Task 36 executed");
	if (keyonoff[36] ==0)
	{
		SET_LED(LED282, ledon);
//		SET_LED(LED345, ledon);
//		SET_LED(LED346, ledon);
//		All_ledon();
		keyonoff[36] = 1;
	} else {
		SET_LED(LED282, ledoff);
//		SET_LED(LED345, ledoff);
//		SET_LED(LED346, ledoff);
//		All_ledoff();
		keyonoff[36] = 0;
		
	} 
}

void task37() { 
Serial.println("Task 37 executed");
	if (keyonoff[37] ==0)
	{
		SET_LED(LED286, ledon);
//		SET_LED(LED345, ledon);
//		SET_LED(LED346, ledon);
//		All_ledon();
		keyonoff[37] = 1;
	} else {
		SET_LED(LED286, ledoff);
//		SET_LED(LED345, ledoff);
//		SET_LED(LED346, ledoff);
//		All_ledoff();
		keyonoff[37] = 0;
		
	} 
}

void task38() { 
Serial.println("Task 38 executed");
	if (keyonoff[38] ==0)
	{
		SET_LED(LED228, ledon);
		SET_LED(LED194, ledon);
		SET_LED(LED193, ledon);
		keyonoff[38] = 1;
	} else {
		SET_LED(LED228, ledoff);
		SET_LED(LED194, ledoff);
		SET_LED(LED193, ledoff);
		keyonoff[38] = 0;
		
	} 
}

void task39() { 
Serial.println("Task 39 executed");
	if (keyonoff[39] ==0)
	{
		SET_LED(LED195, ledon);
//		SET_LED(LED345, ledon);
//		SET_LED(LED346, ledon);
		keyonoff[39] = 1;
	} else {
		SET_LED(LED195, ledoff);
//		SET_LED(LED345, ledoff);
//		SET_LED(LED346, ledoff);
		keyonoff[39] = 0;
		
	} 
}

void task40() { 
Serial.println("Task 40 executed");
	if (keyonoff[40] ==0)
	{
		SET_LED(LED284, ledon);
//		SET_LED(LED345, ledon);
//		SET_LED(LED346, ledon);
		keyonoff[40] = 1;
	} else {
		SET_LED(LED284, ledoff);
//		SET_LED(LED345, ledoff);
//		SET_LED(LED346, ledoff);
		keyonoff[40] = 0;
		
	} 
}

void task41() { 
	Serial.println("Task 41 executed");
	if (keyonoff[41] ==0)
	{
		Serial.println("Task 41 key on !!");		
		SET_LED(LED290, ledon);
//		SET_LED(LED345, ledon);
//		SET_LED(LED346, ledon);
//		All_ledon();
		keyonoff[41] = 1;
	} else {
		Serial.println("Task 41 key off !!");		
		SET_LED(LED290, ledoff);
//		SET_LED(LED345, ledoff);
//		SET_LED(LED346, ledoff);
//	All_ledoff();
		keyonoff[41] = 0;
		
	} 
}

void task42() { 
	Serial.println("Task 42 executed");
	if (keyonoff[42] ==0)
	{
		Serial.println("Task 42 key on !!");		
		SET_LED(LED244, ledon);
//		SET_LED(LED345, ledon);
//		SET_LED(LED346, ledon);
		keyonoff[42] = 1;
	} else {
		Serial.println("Task 42 key off !!");		
		SET_LED(LED244, ledoff);
//		SET_LED(LED345, ledoff);
//		SET_LED(LED346, ledoff);
		keyonoff[42] = 0;
		
	} 
}

void task43() { 
	Serial.println("Task 43 executed");
	if (keyonoff[43] ==0)
	{
		Serial.println("Task 43 key on !!");		
		SET_LED(LED294, ledon);
//		SET_LED(LED345, ledon);
//		SET_LED(LED346, ledon);
//		All_ledon();
		keyonoff[43] = 1;
	} else {
		Serial.println("Task 42 key off !!");		
		SET_LED(LED294, ledoff);
//		SET_LED(LED345, ledoff);
//		SET_LED(LED346, ledoff);
//		All_ledoff();
		keyonoff[43] = 0;
		
	} 
}

void task44() { 
	Serial.println("Task 44 executed");
	if (keyonoff[44] ==0)
	{
		Serial.println("Task 44 key on !!");		
		SET_LED(LED240, ledon);
//		SET_LED(LED345, ledon);
//		SET_LED(LED346, ledon);
		keyonoff[44] = 1;
	} else {
		Serial.println("Task 44 key off !!");		
		SET_LED(LED240, ledoff);
//		SET_LED(LED345, ledoff);
//		SET_LED(LED346, ledoff);
		keyonoff[44] = 0;
		
	} 
}

void task45() { 
	Serial.println("Task 45 executed");
	if (keyonoff[45] ==0)
	{
		Serial.println("Task 45 key on !!");		
		SET_LED(LED222, ledon);
//		SET_LED(LED345, ledon);
//		SET_LED(LED346, ledon);
		keyonoff[45] = 1;
	} else {
		Serial.println("Task 45 key off !!");		
		SET_LED(LED222, ledoff);
//		SET_LED(LED345, ledoff);
//		SET_LED(LED346, ledoff);
		keyonoff[45] = 0;
		
	} 
}

void task46() { 
	Serial.println("Task 46 executed");
	if (keyonoff[46] ==0)
	{
		Serial.println("Task 46 key on !!");		
		SET_LED(LED175, ledon);
		SET_LED(LED176, ledon);
		SET_LED(LED177, ledon);
		keyonoff[46] = 1;
	} else {
		Serial.println("Task 46 key off !!");		
		SET_LED(LED175, ledoff);
		SET_LED(LED176, ledoff);
		SET_LED(LED177, ledoff);
		keyonoff[46] = 0;
		
	} 
}

void task47() { 
	Serial.println("Task 47 executed");
	if (keyonoff[47] ==0)
	{
		Serial.println("Task 47 key on !!");		
		SET_LED(LED181, ledon);
		SET_LED(LED182, ledon);
		SET_LED(LED183, ledon);
		keyonoff[47] = 1;
	} else {
		Serial.println("Task 47 key off !!");		
		SET_LED(LED181, ledoff);
		SET_LED(LED182, ledoff);
		SET_LED(LED183, ledoff);
		keyonoff[47] = 0;
		
	} 
}
/*
*****************************************
Encoder Interrupt Process subroutine

*****************************************
*/
/*
void updateEncoder() {
  int MSB = digitalRead(2); // 讀取A腳
  int LSB = digitalRead(3); // 讀取B腳
  int encoded = (MSB << 1) | LSB; // 合併A和B腳狀態
  int sum = (lastEncoded << 2) | encoded; // 計算變化量

  // 判斷旋轉方向
  if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) {
    encoderValue++;
  } else if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) {
    encoderValue--;
  }

  lastEncoded = encoded; // 更新上一次編碼狀態
	// 限制 encoderValue 在 0 到 36 之間
	
	//這段程式碼使用了 (encoderValue + 37) % 37 來確保 encoderValue 始終在 0 到 36 之間。
	//加上 37 是為了處理 encoderValue 可能為負數的情況。
	
	encoderValue = (encoderValue + 37) % 37;
	Serial.print("Encoder counter : ");
	Serial.println(encoderValue);
	
} */


// 創建函數指針數組
void (*tasks[])() = {
  task0,  task1,  task2,  task3,  task4,  task5,  task6,  task7,  task8,
  task9,  task10,  task11,  task12,  task13,  task14,  task15,  task16,
  task17,  task18,  task19,  task20,  task21,  task22,  task23,  task24,
  task25,  task26,  task27,  task28,  task29,  task30,  task31,  task32,
  task33,  task34,  task35, task36,  task37,  task38,  task39,  task40,
  task41,  task42,  task43, task44,  task45, task46, task47
};

void setup() {
    Wire.begin(); //(D2,D1);  //(0,2); //sda=0 | D3, scl=2 | D4
  Wire.setClock(100000); // 設定 i2c 的速率為 100 KHz RP2040 很重要不然會導致通信錯誤！！ 導線太長速度太快會導致資料錯誤！！！2024/11/03 by leecw


  Serial.begin(115200);

	
  pinMode(LED_PIN, OUTPUT); // 工作LED閃爍 初始化
  pinMode(debugPin, OUTPUT); // 工作LED閃爍 初始化
	digitalWrite(debugPin, HIGH);
	pinMode(FS1, OUTPUT);
	pinMode(FS2, OUTPUT);
	digitalWrite(FS1, LOW);	
	digitalWrite(FS2, LOW);	
  for (int i = 0; i < cols; i++) {
    pinMode(colPins[i], OUTPUT);
    digitalWrite(colPins[i], HIGH);
  }
  for (int i = 0; i < rows; i++) {
    pinMode(rowPins[i], INPUT_PULLUP);
  }
  
  pinMode(CLK, INPUT_PULLUP); // 設定A腳為輸入並啟用內部上拉電阻
  pinMode(DT, INPUT_PULLUP); // 設定B腳為輸入並啟用內部上拉電阻
//  attachInterrupt(digitalPinToInterrupt(EncoderA), updateEncoder, CHANGE); // 當A腳變化時觸發中斷

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

/*
**********************************
***** led 更新處理副程式
**********************************
*/
//void led_update(void)
//{
	
	
//}




//void keyProcess(void)
//{

	
//}


void Encode_process(void)
{
	currentStateCLK = digitalRead(CLK);
//	currentStateSW = digitalRead(SW);

  if (currentStateCLK != lastStateCLK && currentStateCLK == HIGH) {
    if (digitalRead(DT) != currentStateCLK) {
      counter++;
    } else {
      counter--;
    }
	counter = (counter + 16) % 16;
    Serial.print("Position: ");
    Serial.println(counter);
  }

//  if (currentStateSW == LOW && !SWPressed) {
//    SWPressed = true;
//    Serial.println("Switch pressed");
//  }

//  if (currentStateSW == HIGH && SWPressed) {
//    SWPressed = false;
//    Serial.println("Switch released");
//  }

  lastStateCLK = currentStateCLK;
  
  switch(counter)
  {
	  case 0:
		SET_LED(LED304, ledon);
		SET_LED(LED300, ledoff);
		SET_LED(LED303, ledoff);
		SET_LED(LED299, ledoff);			
		SET_LED(LED302, ledoff);
		SET_LED(LED298, ledoff);
		SET_LED(LED301, ledoff);
		SET_LED(LED297, ledoff);			
		SET_LED(LED312, ledoff);
		SET_LED(LED308, ledoff);
		SET_LED(LED311, ledoff);
		SET_LED(LED307, ledoff);			
		SET_LED(LED310, ledoff);
		SET_LED(LED306, ledoff);
		SET_LED(LED309, ledoff);
		SET_LED(LED305, ledoff);			
		break;
	case 1:	
		SET_LED(LED304, ledoff);
		SET_LED(LED300, ledon);
		SET_LED(LED303, ledoff);
		SET_LED(LED299, ledoff);			
		SET_LED(LED302, ledoff);
		SET_LED(LED298, ledoff);
		SET_LED(LED301, ledoff);
		SET_LED(LED297, ledoff);			
		SET_LED(LED312, ledoff);
		SET_LED(LED308, ledoff);
		SET_LED(LED311, ledoff);
		SET_LED(LED307, ledoff);			
		SET_LED(LED310, ledoff);
		SET_LED(LED306, ledoff);
		SET_LED(LED309, ledoff);
		SET_LED(LED305, ledoff);			
		break;
	case 2:	
		SET_LED(LED304, ledoff);
		SET_LED(LED300, ledoff);
		SET_LED(LED303, ledon);
		SET_LED(LED299, ledoff);			
		SET_LED(LED302, ledoff);
		SET_LED(LED298, ledoff);
		SET_LED(LED301, ledoff);
		SET_LED(LED297, ledoff);			
		SET_LED(LED312, ledoff);
		SET_LED(LED308, ledoff);
		SET_LED(LED311, ledoff);
		SET_LED(LED307, ledoff);			
		SET_LED(LED310, ledoff);
		SET_LED(LED306, ledoff);
		SET_LED(LED309, ledoff);
		SET_LED(LED305, ledoff);			
		break;		
	case 3:	
		SET_LED(LED304, ledoff);	//0
		SET_LED(LED300, ledoff);    //1
		SET_LED(LED303, ledoff);    //2
		SET_LED(LED299, ledon);		//3	
		SET_LED(LED302, ledoff);    //4
		SET_LED(LED298, ledoff);    //5
		SET_LED(LED301, ledoff);    //6
		SET_LED(LED297, ledoff);	//7		
		SET_LED(LED312, ledoff);    //8
		SET_LED(LED308, ledoff);    //9
		SET_LED(LED311, ledoff);    //10
		SET_LED(LED307, ledoff);	//11		
		SET_LED(LED310, ledoff);    //12
		SET_LED(LED306, ledoff);    //13
		SET_LED(LED309, ledoff);    //14
		SET_LED(LED305, ledoff);	//15		
		break;	  
	case 4:	
		SET_LED(LED304, ledoff);	//0
		SET_LED(LED300, ledoff);    //1
		SET_LED(LED303, ledoff);    //2
		SET_LED(LED299, ledoff);		//3	
		SET_LED(LED302, ledon);    //4
		SET_LED(LED298, ledoff);    //5
		SET_LED(LED301, ledoff);    //6
		SET_LED(LED297, ledoff);	//7		
		SET_LED(LED312, ledoff);    //8
		SET_LED(LED308, ledoff);    //9
		SET_LED(LED311, ledoff);    //10
		SET_LED(LED307, ledoff);	//11		
		SET_LED(LED310, ledoff);    //12
		SET_LED(LED306, ledoff);    //13
		SET_LED(LED309, ledoff);    //14
		SET_LED(LED305, ledoff);	//15		
		break;	  
	case 5:	
		SET_LED(LED304, ledoff);	//0
		SET_LED(LED300, ledoff);    //1
		SET_LED(LED303, ledoff);    //2
		SET_LED(LED299, ledoff);		//3	
		SET_LED(LED302, ledoff);    //4
		SET_LED(LED298, ledon);    //5
		SET_LED(LED301, ledoff);    //6
		SET_LED(LED297, ledoff);	//7		
		SET_LED(LED312, ledoff);    //8
		SET_LED(LED308, ledoff);    //9
		SET_LED(LED311, ledoff);    //10
		SET_LED(LED307, ledoff);	//11		
		SET_LED(LED310, ledoff);    //12
		SET_LED(LED306, ledoff);    //13
		SET_LED(LED309, ledoff);    //14
		SET_LED(LED305, ledoff);	//15		
		break;	  
	case 6:	
		SET_LED(LED304, ledoff);	//0
		SET_LED(LED300, ledoff);    //1
		SET_LED(LED303, ledoff);    //2
		SET_LED(LED299, ledoff);		//3	
		SET_LED(LED302, ledoff);    //4
		SET_LED(LED298, ledoff);    //5
		SET_LED(LED301, ledon);    //6
		SET_LED(LED297, ledoff);	//7		
		SET_LED(LED312, ledoff);    //8
		SET_LED(LED308, ledoff);    //9
		SET_LED(LED311, ledoff);    //10
		SET_LED(LED307, ledoff);	//11		
		SET_LED(LED310, ledoff);    //12
		SET_LED(LED306, ledoff);    //13
		SET_LED(LED309, ledoff);    //14
		SET_LED(LED305, ledoff);	//15		
		break;	  
	case 7:	
		SET_LED(LED304, ledoff);	//0
		SET_LED(LED300, ledoff);    //1
		SET_LED(LED303, ledoff);    //2
		SET_LED(LED299, ledoff);		//3	
		SET_LED(LED302, ledoff);    //4
		SET_LED(LED298, ledoff);    //5
		SET_LED(LED301, ledoff);    //6
		SET_LED(LED297, ledon);	//7		
		SET_LED(LED312, ledoff);    //8
		SET_LED(LED308, ledoff);    //9
		SET_LED(LED311, ledoff);    //10
		SET_LED(LED307, ledoff);	//11		
		SET_LED(LED310, ledoff);    //12
		SET_LED(LED306, ledoff);    //13
		SET_LED(LED309, ledoff);    //14
		SET_LED(LED305, ledoff);	//15		
		break;	  
	case 8:	
		SET_LED(LED304, ledoff);	//0
		SET_LED(LED300, ledoff);    //1
		SET_LED(LED303, ledoff);    //2
		SET_LED(LED299, ledoff);		//3	
		SET_LED(LED302, ledoff);    //4
		SET_LED(LED298, ledoff);    //5
		SET_LED(LED301, ledoff);    //6
		SET_LED(LED297, ledoff);	//7		
		SET_LED(LED312, ledon);    //8
		SET_LED(LED308, ledoff);    //9
		SET_LED(LED311, ledoff);    //10
		SET_LED(LED307, ledoff);	//11		
		SET_LED(LED310, ledoff);    //12
		SET_LED(LED306, ledoff);    //13
		SET_LED(LED309, ledoff);    //14
		SET_LED(LED305, ledoff);	//15		
		break;	  
	case 9:	
		SET_LED(LED304, ledoff);	//0
		SET_LED(LED300, ledoff);    //1
		SET_LED(LED303, ledoff);    //2
		SET_LED(LED299, ledoff);		//3	
		SET_LED(LED302, ledoff);    //4
		SET_LED(LED298, ledoff);    //5
		SET_LED(LED301, ledoff);    //6
		SET_LED(LED297, ledoff);	//7		
		SET_LED(LED312, ledoff);    //8
		SET_LED(LED308, ledon);    //9
		SET_LED(LED311, ledoff);    //10
		SET_LED(LED307, ledoff);	//11		
		SET_LED(LED310, ledoff);    //12
		SET_LED(LED306, ledoff);    //13
		SET_LED(LED309, ledoff);    //14
		SET_LED(LED305, ledoff);	//15		
		break;	  
	case 10:	
		SET_LED(LED304, ledoff);	//0
		SET_LED(LED300, ledoff);    //1
		SET_LED(LED303, ledoff);    //2
		SET_LED(LED299, ledoff);		//3	
		SET_LED(LED302, ledoff);    //4
		SET_LED(LED298, ledoff);    //5
		SET_LED(LED301, ledoff);    //6
		SET_LED(LED297, ledoff);	//7		
		SET_LED(LED312, ledoff);    //8
		SET_LED(LED308, ledoff);    //9
		SET_LED(LED311, ledon);    //10
		SET_LED(LED307, ledoff);	//11		
		SET_LED(LED310, ledoff);    //12
		SET_LED(LED306, ledoff);    //13
		SET_LED(LED309, ledoff);    //14
		SET_LED(LED305, ledoff);	//15		
		break;	
	case 11:	
		SET_LED(LED304, ledoff);	//0
		SET_LED(LED300, ledoff);    //1
		SET_LED(LED303, ledoff);    //2
		SET_LED(LED299, ledoff);		//3	
		SET_LED(LED302, ledoff);    //4
		SET_LED(LED298, ledoff);    //5
		SET_LED(LED301, ledoff);    //6
		SET_LED(LED297, ledoff);	//7		
		SET_LED(LED312, ledoff);    //8
		SET_LED(LED308, ledoff);    //9
		SET_LED(LED311, ledoff);    //10
		SET_LED(LED307, ledon);	//11		
		SET_LED(LED310, ledoff);    //12
		SET_LED(LED306, ledoff);    //13
		SET_LED(LED309, ledoff);    //14
		SET_LED(LED305, ledoff);	//15		
		break;	  
	case 12:	
		SET_LED(LED304, ledoff);	//0
		SET_LED(LED300, ledoff);    //1
		SET_LED(LED303, ledoff);    //2
		SET_LED(LED299, ledoff);		//3	
		SET_LED(LED302, ledoff);    //4
		SET_LED(LED298, ledoff);    //5
		SET_LED(LED301, ledoff);    //6
		SET_LED(LED297, ledoff);	//7		
		SET_LED(LED312, ledoff);    //8
		SET_LED(LED308, ledoff);    //9
		SET_LED(LED311, ledoff);    //10
		SET_LED(LED307, ledoff);	//11		
		SET_LED(LED310, ledon);    //12
		SET_LED(LED306, ledoff);    //13
		SET_LED(LED309, ledoff);    //14
		SET_LED(LED305, ledoff);	//15		
		break;	  
	case 13:	
		SET_LED(LED304, ledoff);	//0
		SET_LED(LED300, ledoff);    //1
		SET_LED(LED303, ledoff);    //2
		SET_LED(LED299, ledoff);		//3	
		SET_LED(LED302, ledoff);    //4
		SET_LED(LED298, ledoff);    //5
		SET_LED(LED301, ledoff);    //6
		SET_LED(LED297, ledoff);	//7		
		SET_LED(LED312, ledoff);    //8
		SET_LED(LED308, ledoff);    //9
		SET_LED(LED311, ledoff);    //10
		SET_LED(LED307, ledoff);	//11		
		SET_LED(LED310, ledoff);    //12
		SET_LED(LED306, ledon);    //13
		SET_LED(LED309, ledoff);    //14
		SET_LED(LED305, ledoff);	//15		
		break;	  
	case 14:	
		SET_LED(LED304, ledoff);	//0
		SET_LED(LED300, ledoff);    //1
		SET_LED(LED303, ledoff);    //2
		SET_LED(LED299, ledoff);		//3	
		SET_LED(LED302, ledoff);    //4
		SET_LED(LED298, ledoff);    //5
		SET_LED(LED301, ledoff);    //6
		SET_LED(LED297, ledoff);	//7		
		SET_LED(LED312, ledoff);    //8
		SET_LED(LED308, ledoff);    //9
		SET_LED(LED311, ledoff);    //10
		SET_LED(LED307, ledoff);	//11		
		SET_LED(LED310, ledoff);    //12
		SET_LED(LED306, ledoff);    //13
		SET_LED(LED309, ledon);    //14
		SET_LED(LED305, ledoff);	//15		
		break;	  
	case 15:	
		SET_LED(LED304, ledoff);	//0
		SET_LED(LED300, ledoff);    //1
		SET_LED(LED303, ledoff);    //2
		SET_LED(LED299, ledoff);		//3	
		SET_LED(LED302, ledoff);    //4
		SET_LED(LED298, ledoff);    //5
		SET_LED(LED301, ledoff);    //6
		SET_LED(LED297, ledoff);	//7		
		SET_LED(LED312, ledoff);    //8
		SET_LED(LED308, ledoff);    //9
		SET_LED(LED311, ledoff);    //10
		SET_LED(LED307, ledoff);	//11		
		SET_LED(LED310, ledoff);    //12
		SET_LED(LED306, ledoff);    //13
		SET_LED(LED309, ledoff);    //14
		SET_LED(LED305, ledon);	//15		
		break;	  
		

  }
}
	
	
//}

/*
***************************
***** Main Loop
***************************
*/
void loop() {
	handleLEDBlink(); //工作燈
	Encode_process();
	I2C_W_2BYTE(SNLED2735_ADDRESS, 0xFD, 0x03);  // 選擇功能頁面 (Page 3)
	I2C_W_2BYTE(SNLED2735_ADDRESS, 0x00, 0x01);  // 設定 LED 驅動器為一般模式//關閉模式  
	I2C_W_2BYTE(SNLED2735_ADDRESS, 0xFD, 0x00);  // 選擇 LED 控制頁面 (Page 0)
	I2C_W_LED_UPDATE(SNLED2735_ADDRESS, 0x00, 0x17, MAIN_LEFT_LED_MAP); // 更新LED狀態		
//	led_update();	// led 的狀態更新
//	keyProcess();	// 按鍵掃描彈跳處理副程式處理
  for (int i = 0; i < cols; i++) {
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
//  int taskIndex = col * cols + row; // 計算任務索引
  Serial.print("taskIndex: ");
  Serial.println(taskIndex);
  if (taskIndex >= 0 && taskIndex < 48) {
    tasks[taskIndex](); // 呼叫對應的任務函數
  }
}