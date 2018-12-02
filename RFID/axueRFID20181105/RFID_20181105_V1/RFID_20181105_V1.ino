/*
  Arduino RFID Access Control

  Security !

  To keep it simple we are going to use Tag's Unique IDs
  as only method of Authenticity. It's simple and not hacker proof.
  If you need security, don't use it unless you modify the code

  Copyright (C) 2015 Omer Siar Baysal

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
Arduino RFID訪問控制

   安全！

   為了簡單起見，我們將使用Tag的唯一ID
   僅作為真實性的方法。 這很簡單，而不是黑客證明。
   如果您需要安全性，請不要使用它，除非您修改代碼

   版權所有（C）2015 Omer Siar Baysal

   這個程序是免費軟件; 您可以重新分發和/或修改
   它是根據GNU通用公共許可證的條款發布的
   自由軟件基金會; 許可證的第2版，或
   （根據您的選擇）任何更高版本。

   這個程序是分發的，希望它有用，
   但沒有任何擔保; 甚至沒有暗示的保證
   適銷性或適用於特定用途。 見
   GNU通用公共許可證了解更多詳情。

   您應該已經收到了GNU通用公共許可證的副本
   有這個程序; 如果沒有，請寫信給Free Software Foundation，Inc。，
   51 Franklin Street，Fifth Floor，Boston，MA 02110-1301 USA。
*/

#include <EEPROM.h>     // We are going to read and write PICC's UIDs from/to EEPROM 我們將從/向EEPROM讀取和寫入PICC的UID
#include <SPI.h>        // RC522 Module uses SPI protocol
#include <MFRC522.h>	// Library for Mifare RC522 Devices

/*
	Instead of a Relay maybe you want to use a servo
	Servos can lock and unlock door locks too
	There are examples out there.
	而不是繼電器可能你想使用伺服
伺服系統也可以鎖定和解鎖門鎖
有一些例子。
*/

// #include <Servo.h>

/*
	For visualizing whats going on hardware
	we need some leds and
	to control door lock a relay and a wipe button
	(or some other hardware)
	Used common anode led,digitalWriting HIGH turns OFF led
	Mind that if you are going to use common cathode led or
	just seperate leds, simply comment out #define COMMON_ANODE,
	用於可視化硬件上的最新情況
我們需要一些LED和
控制門鎖一個繼電器和一個擦拭按鈕
（或其他一些硬件）
使用共陽極led，digitalWriting HIGH關閉led
請注意，如果你打算使用共陰極led或
只是單獨的LED，只需註釋#define COMMON_ANODE，
*/

#define COMMON_ANODE

#ifdef COMMON_ANODE
#define LED_ON LOW
#define LED_OFF HIGH
#else
#define LED_ON HIGH
#define LED_OFF LOW
#endif

#define redLed 7		// Set Led Pins
#define greenLed 6
#define blueLed 5

#define relay 4			// Set Relay Pin
#define wipeB 3			// Button pin for WipeMode

boolean match = false;          // initialize card match to false
boolean programMode = false;	// initialize programming mode to false
boolean replaceMaster = false;

int successRead;		// 如果我們從Reader讀取成功，則保留變數整數 Variable integer to keep if we have Successful Read from Reader

byte storedCard[4];		// 存儲從EEPROM讀取的ID Stores an ID read from EEPROM 
byte readCard[4];		// 存儲從RFID模塊讀取的掃描ID Stores scanned ID read from RFID Module 
byte masterCard[4];		// 存儲從EEPROM讀取的主卡ID Stores master card's ID read from EEPROM 

/*
	We need to define MFRC522's pins and create instance
	Pin layout should be as follows (on Arduino Uno):
	MOSI: Pin 11 / ICSP-4
	MISO: Pin 12 / ICSP-1
	SCK : Pin 13 / ICSP-3
	SS : Pin 10 (Configurable)
	RST : Pin 9 (Configurable)
	look MFRC522 Library for
	other Arduinos' pin configuration
*/

#define SS_PIN 10
#define RST_PIN 9
MFRC522 mfrc522(SS_PIN, RST_PIN);	// Create MFRC522 instance.

///////////////////////////////////////// Setup ///////////////////////////////////
void setup() {
  //Arduino Pin Configuration
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(blueLed, OUTPUT);
  pinMode(wipeB, INPUT_PULLUP);		// Enable pin's pull up resistor
  pinMode(relay, OUTPUT);
  //Be careful how relay circuit behave on while resetting or power-cycling your Arduino
  digitalWrite(relay, HIGH);		// Make sure door is locked
  digitalWrite(redLed, LED_OFF);	// Make sure led is off
  digitalWrite(greenLed, LED_OFF);	// Make sure led is off
  digitalWrite(blueLed, LED_OFF);	// Make sure led is off

  //Protocol Configuration
  Serial.begin(115200);	 // Initialize serial communications with PC
  SPI.begin();           // MFRC522 Hardware uses SPI protocol
  mfrc522.PCD_Init();    // Initialize MFRC522 Hardware

  //If you set Antenna Gain to Max it will increase reading distance
  //如果將“天線增益”設置為“最大”，則會增加讀取距離
  /*
  設定讀卡機的天線增益
  	// MFRC522 RxGain[2:0] masks, defines the receiver's signal voltage gain factor (on the PCD).
	// Described in 9.3.3.6 / table 98 of the datasheet at http://www.nxp.com/documents/data_sheet/MFRC522.pdf
	enum PCD_RxGain : byte {
		RxGain_18dB				= 0x00 << 4,	// 000b - 18 dB, minimum
		RxGain_23dB				= 0x01 << 4,	// 001b - 23 dB
		RxGain_18dB_2			= 0x02 << 4,	// 010b - 18 dB, it seems 010b is a duplicate for 000b
		RxGain_23dB_2			= 0x03 << 4,	// 011b - 23 dB, it seems 011b is a duplicate for 001b
		RxGain_33dB				= 0x04 << 4,	// 100b - 33 dB, average, and typical default
		RxGain_38dB				= 0x05 << 4,	// 101b - 38 dB
		RxGain_43dB				= 0x06 << 4,	// 110b - 43 dB
		RxGain_48dB				= 0x07 << 4,	// 111b - 48 dB, maximum
		RxGain_min				= 0x00 << 4,	// 000b - 18 dB, minimum, convenience for RxGain_18dB
		RxGain_avg				= 0x04 << 4,	// 100b - 33 dB, average, convenience for RxGain_33dB
		RxGain_max				= 0x07 << 4		// 111b - 48 dB, maximum, convenience for RxGain_48dB
  mfrc522.PCD_SetAntennaGain
  
  
  */
  
  
  mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_38dB);
  

  Serial.println(F("Access Control v3.4"));   // 用於除錯目的 For debugging purposes
  ShowReaderDetails();	// 顯示PCD - MFRC522讀卡器詳細信息 Show details of PCD - MFRC522 Card Reader details

  //Wipe Code if Button Pressed while setup run (powered on) it wipes EEPROM
  //如果在設置運行（打開電源）時按下按鈕，則擦除代碼擦除EEPROM
  if (digitalRead(wipeB) == LOW) {	// when button pressed pin should get low, button connected to ground
    digitalWrite(redLed, LED_ON);	// Red Led stays on to inform user we are going to wipe
    Serial.println(F("Wipe Button Pressed"));
    Serial.println(F("You have 15 seconds to Cancel"));
    Serial.println(F("This will be remove all records and cannot be undone"));
    delay(15000);                        // Give user enough time to cancel operation
    if (digitalRead(wipeB) == LOW) {    // If button still be pressed, wipe EEPROM
      Serial.println(F("Starting Wiping EEPROM"));
      for (int x = 0; x < EEPROM.length(); x = x + 1) {    //Loop end of EEPROM address
        if (EEPROM.read(x) == 0) {              //If EEPROM address 0
          // do nothing, already clear, go to the next address in order to save time and reduce writes to EEPROM
        }
        else {
          EEPROM.write(x, 0); 			// if not write 0 to clear, it takes 3.3mS
        }
      }
      Serial.println(F("EEPROM Successfully Wiped"));
      digitalWrite(redLed, LED_OFF); 	// visualize successful wipe
      delay(200);
      digitalWrite(redLed, LED_ON);
      delay(200);
      digitalWrite(redLed, LED_OFF);
      delay(200);
      digitalWrite(redLed, LED_ON);
      delay(200);
      digitalWrite(redLed, LED_OFF);
    }
    else {
      Serial.println(F("Wiping Cancelled"));
      digitalWrite(redLed, LED_OFF);
    }
  }
  // Check if master card defined, if not let user choose a master card
  // This also useful to just redefine Master Card
  // You can keep other EEPROM records just write other than 143 to EEPROM address 1
  // EEPROM address 1 should hold magical number which is '143'
/*
檢查是否定義了主卡，如果沒有，則讓用戶選擇主卡
這對重新定義主卡也很有用
您可以將其他EEPROM記錄保留在143以外的EEPROM地址1中
EEPROM地址1應該保存魔法數字'143'

*/
  
  if (EEPROM.read(1) != 143) {
    Serial.println(F("No Master Card Defined"));
    Serial.println(F("Scan A PICC to Define as Master Card"));
    do {
      successRead = getID();            // sets successRead to 1 when we get read from reader otherwise 0當讀取讀取器時，將successRead設置為1，否則為0
      digitalWrite(blueLed, LED_ON);    // Visualize Master Card need to be defined需要定義可視化主卡
      delay(200);
      digitalWrite(blueLed, LED_OFF);
      delay(200);
    }
    while (!successRead);                  // Program will not go further while you not get a successful read如果您沒有成功讀取，程序將不會更進一步
    for ( int j = 0; j < 4; j++ ) {        // Loop 4 times
      EEPROM.write( 2 + j, readCard[j] );  // Write scanned PICC's UID to EEPROM, start from address 3 將掃描的PICC UID寫入EEPROM，從地址3開始
    }
    EEPROM.write(1, 143);                  // Write to EEPROM we defined Master Card.寫入EEPROM我們定義了Master Card
    Serial.println(F("Master Card Defined")); // 主卡定義
  }
  Serial.println(F("-------------------"));
  Serial.println(F("Master Card's UID"));
  for ( int i = 0; i < 4; i++ ) {          // 從EEPROM讀取Master Card的UID  Read Master Card's UID from EEPROM
    masterCard[i] = EEPROM.read(2 + i);    // 寫到masterCard   Write it to masterCard
    Serial.print(masterCard[i], HEX);
  }
  Serial.println("");
  Serial.println(F("-------------------"));
  Serial.println(F("Everything Ready"));
  Serial.println(F("Waiting PICCs to be scanned"));
  cycleLeds();    // 一切準備好讓我們通過循環的LED給用戶一些回饋 Everything ready lets give user some feedback by cycling leds
}


///////////////////////////////////////// Main Loop ///////////////////////////////////
void loop () {
  do {
    successRead = getID(); 	// sets successRead to 1 when we get read from reader otherwise 0當讀取讀取器時，將successRead設置為1，否則為0
    if (digitalRead(wipeB) == LOW) {
      digitalWrite(redLed, LED_ON);  // Make sure led is off
      digitalWrite(greenLed, LED_OFF);  // Make sure led is off
      digitalWrite(blueLed, LED_OFF); // Make sure led is off
      Serial.println(F("Wipe Button Pressed"));
      Serial.println(F("Master Card will be Erased! in 10 seconds")); //主卡將被刪除！ 在10秒鐘內
      delay(10000);
      if (digitalRead(wipeB) == LOW) {
        EEPROM.write(1, 0);                  // Reset Magic Number.重置幻數。
        Serial.println(F("Restart device to re-program Master Card"));
        while (1);
      }
    }
    if (programMode) {
      cycleLeds();              // Program Mode cycles through RGB waiting to read a new card 程序模式循環通過RGB等待讀取新卡
    }
    else {
      normalModeOn(); 		// Normal mode, blue Power LED is on, all others are off正常模式下，藍色電源LED亮起，其他所有關閉
    }
  }
  while (!successRead); 	//the program will not go further while you not get a successful read如果您沒有成功讀取，該程式將不會更進一步
  if (programMode) {
    if ( isMaster(readCard) ) { //If master card scanned again exit program mode如果再次掃描主卡退出程序模式
      Serial.println(F("Master Card Scanned"));
      Serial.println(F("Exiting Program Mode"));
      Serial.println(F("-----------------------------"));
      programMode = false;
      return;
    }
    else {
      if ( findID(readCard) ) { // If scanned card is known delete it
        Serial.println(F("I know this PICC, removing..."));
        deleteID(readCard);
        Serial.println("-----------------------------");
        Serial.println(F("Scan a PICC to ADD or REMOVE to EEPROM"));
      }
      else {                    // If scanned card is not known add it
        Serial.println(F("I do not know this PICC, adding..."));
        writeID(readCard);
        Serial.println(F("-----------------------------"));
        Serial.println(F("Scan a PICC to ADD or REMOVE to EEPROM"));
      }
    }
  }
  else {
    if ( isMaster(readCard)) {  	//如果掃描卡的ID與主卡的ID匹配，請進入程序模式  If scanned card's ID matches Master Card's ID enter program mode
      programMode = true;
      Serial.println(F("Hello Master - Entered Program Mode"));
      int count = EEPROM.read(0); 	// Read the first Byte of EEPROM that
      Serial.print(F("I have "));    	// stores the number of ID's in EEPROM
      Serial.print(count);
      Serial.print(F(" record(s) on EEPROM"));
      Serial.println("");
      Serial.println(F("Scan a PICC to ADD or REMOVE to EEPROM"));
      Serial.println(F("Scan Master Card again to Exit Program Mode"));
      Serial.println(F("-----------------------------"));
    }
    else {
      if ( findID(readCard) ) {	// If not, see if the card is in the EEPROM 如果沒有，請查看該卡是否在EEPROM中
        Serial.println(F("Welcome, You shall pass"));
        granted(300);        	// Open the door lock for 300 ms 打開門鎖300毫秒
      }
      else {			// If not, show that the ID was not valid 如果沒有，請顯示該ID無效
        Serial.println(F("You shall not pass"));
        denied();
      }
    }
  }
}

/////////////////////////////////////////  Access Granted    ///////////////////////////////////
void granted (int setDelay) {
  digitalWrite(blueLed, LED_OFF); 	// Turn off blue LED
  digitalWrite(redLed, LED_OFF); 	// Turn off red LED
  digitalWrite(greenLed, LED_ON); 	// Turn on green LED
  digitalWrite(relay, LOW); 		// Unlock door!
  delay(setDelay); 					// Hold door lock open for given seconds
  digitalWrite(relay, HIGH); 		// Relock door
  delay(1000); 						// Hold green LED on for a second
}

///////////////////////////////////////// Access Denied  ///////////////////////////////////
void denied() {
  digitalWrite(greenLed, LED_OFF); 	// Make sure green LED is off
  digitalWrite(blueLed, LED_OFF); 	// Make sure blue LED is off
  digitalWrite(redLed, LED_ON); 	// Turn on red LED
  delay(1000);
}


///////////////////////////////////////// Get PICC's UID ///////////////////////////////////
int getID() {
  // Getting ready for Reading PICCs
  if ( ! mfrc522.PICC_IsNewCardPresent()) { //If a new PICC placed to RFID reader continue
    return 0;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {   //Since a PICC placed get Serial and continue
    return 0;
  }
  // There are Mifare PICCs which have 4 byte or 7 byte UID care if you use 7 byte PICC
  // I think we should assume every PICC as they have 4 byte UID
  // Until we support 7 byte PICCs
  Serial.println(F("Scanned PICC's UID:"));
  for (int i = 0; i < 4; i++) {  //
    readCard[i] = mfrc522.uid.uidByte[i];
    Serial.print(readCard[i], HEX);
  }
  Serial.println("");
  mfrc522.PICC_HaltA(); // Stop reading
  return 1;
}

void ShowReaderDetails() {
  // Get the MFRC522 software version
  byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  Serial.print(F("MFRC522 Software Version: 0x"));
  Serial.print(v, HEX);
  if (v == 0x91)
    Serial.print(F(" = v1.0"));
  else if (v == 0x92)
    Serial.print(F(" = v2.0"));
  else
    Serial.print(F(" (unknown),probably a chinese clone?"));
  Serial.println("");
  // When 0x00 or 0xFF is returned, communication probably failed
  if ((v == 0x00) || (v == 0xFF)) {
    Serial.println(F("WARNING: Communication failure, is the MFRC522 properly connected?"));
    Serial.println(F("SYSTEM HALTED: Check connections."));
    while (true); // do not go further
  }
}

///////////////////////////////////////// Cycle Leds (Program Mode) ///////////////////////////////////
void cycleLeds() {
  digitalWrite(redLed, LED_OFF); 	// Make sure red LED is off
  digitalWrite(greenLed, LED_ON); 	// Make sure green LED is on
  digitalWrite(blueLed, LED_OFF); 	// Make sure blue LED is off
  delay(200);
  digitalWrite(redLed, LED_OFF); 	// Make sure red LED is off
  digitalWrite(greenLed, LED_OFF); 	// Make sure green LED is off
  digitalWrite(blueLed, LED_ON); 	// Make sure blue LED is on
  delay(200);
  digitalWrite(redLed, LED_ON); 	// Make sure red LED is on
  digitalWrite(greenLed, LED_OFF); 	// Make sure green LED is off
  digitalWrite(blueLed, LED_OFF); 	// Make sure blue LED is off
  delay(200);
}

//////////////////////////////////////// Normal Mode Led  ///////////////////////////////////
void normalModeOn () {
  digitalWrite(blueLed, LED_ON); 	// Blue LED ON and ready to read card
  digitalWrite(redLed, LED_OFF); 	// Make sure Red LED is off
  digitalWrite(greenLed, LED_OFF); 	// Make sure Green LED is off
  digitalWrite(relay, HIGH); 		// Make sure Door is Locked
}

//////////////////////////////////////// Read an ID from EEPROM //////////////////////////////
void readID( int number ) {
  int start = (number * 4 ) + 2; 		// Figure out starting position弄清楚起始位置
  for ( int i = 0; i < 4; i++ ) { 		// Loop 4 times to get the 4 Bytes循環4次以獲得4個字節
    storedCard[i] = EEPROM.read(start + i); 	// Assign values read from EEPROM to array將從EEPROM讀取的值分配給陣列
  }
}

///////////////////////////////////////// Add ID to EEPROM   ///////////////////////////////////
void writeID( byte a[] ) {
  if ( !findID( a ) ) { 		// Before we write to the EEPROM, check to see if we have seen this card before!在我們寫入EEPROM之前，請檢查我們之前是否看過這張卡！
    int num = EEPROM.read(0); 		// Get the numer of used spaces, position 0 stores the number of ID cards獲取已用空間的數量，位置0存儲ID卡的數量
    int start = ( num * 4 ) + 6; 	// Figure out where the next slot starts找出下一程式所佔的位置開始的地方
    num++; 								// Increment the counter by one 將計數器增加一個
    EEPROM.write( 0, num ); 		// Write the new count to the counter 將新計數寫入計數器
    for ( int j = 0; j < 4; j++ ) { 	// Loop 4 times循環4次
      EEPROM.write( start + j, a[j] ); 	// Write the array values to EEPROM in the right position將陣列值寫入正確位置的EEPROM
    }
    successWrite();
    Serial.println(F("Succesfully added ID record to EEPROM"));
  }
  else {
    failedWrite();
    Serial.println(F("Failed! There is something wrong with ID or bad EEPROM"));
  }
}

///////////////////////////////////////// Remove ID from EEPROM 從EEPROM中刪除ID   ///////////////////////////////////
void deleteID( byte a[] ) {
  if ( !findID( a ) ) { 		// Before we delete from the EEPROM, check to see if we have this card! 在我們從EEPROM中刪除之前，檢查一下我們是否有這張卡！
    failedWrite(); 			// If not 如果沒有
    Serial.println(F("Failed! There is something wrong with ID or bad EEPROM"));
  }
  else {
    int num = EEPROM.read(0); 	// Get the numer of used spaces, position 0 stores the number of ID cards 獲取已用空間的數量，位置0存儲ID卡的數量
    int slot; 			// Figure out the slot number of the card 找出卡在記憶體中的位置號碼
    int start;			// = ( num * 4 ) + 6; // Figure out where the next slot starts 找出下一個開始的地方
    int looping; 		// The number of times the loop repeats
    int j;
    int count = EEPROM.read(0); // Read the first Byte of EEPROM that stores number of cards 讀取存儲卡的數量在EEPROM第一個位元
    slot = findIDSLOT( a ); 	// Figure out the slot number of the card to delete 找出要刪除的卡的位置號碼
    start = (slot * 4) + 2;
    looping = ((num - slot) * 4);
    num--; 			// Decrement the counter by one
    EEPROM.write( 0, num ); 	// Write the new count to the counter
    for ( j = 0; j < looping; j++ ) { 				// Loop the card shift times
      EEPROM.write( start + j, EEPROM.read(start + 4 + j)); 	// Shift the array values to 4 places earlier in the EEPROM
    }
    for ( int k = 0; k < 4; k++ ) { 				// Shifting loop
      EEPROM.write( start + j + k, 0);
    }
    successDelete();
    Serial.println(F("Succesfully removed ID record from EEPROM"));
  }
}

///////////////////////////////////////// Check Bytes   ///////////////////////////////////
boolean checkTwo ( byte a[], byte b[] ) {
  if ( a[0] != NULL ) 			// Make sure there is something in the array first確保首先在數組中有一些東西
    match = true; 			// Assume they match at first假設它們首先匹配
  for ( int k = 0; k < 4; k++ ) { 	// Loop 4 times循環4次
    if ( a[k] != b[k] ) 		// IF a != b then set match = false, one fails, all fail如果a！= b然後設置match = false，一個失敗，全部失敗
      match = false;
  }
  if ( match ) { 			// Check to see if if match is still true檢查是否匹配仍然是真的
    return true; 			// Return true
  }
  else  {
    return false; 			// Return false
  }
}

///////////////////////////////////////// Find Slot   ///////////////////////////////////
int findIDSLOT( byte find[] ) {
  int count = EEPROM.read(0); 			// Read the first Byte of EEPROM that
  for ( int i = 1; i <= count; i++ ) { 		// Loop once for each EEPROM entry
    readID(i); 								// Read an ID from EEPROM, it is stored in storedCard[4]
    if ( checkTwo( find, storedCard ) ) { 	// Check to see if the storedCard read from EEPROM
      // is the same as the find[] ID card passed
      return i; 				// The slot number of the card
      break; 					// Stop looking we found it
    }
  }
}

///////////////////////////////////////// Find ID From EEPROM   ///////////////////////////////////
boolean findID( byte find[] ) {
  int count = EEPROM.read(0);			// Read the first Byte of EEPROM that
  for ( int i = 1; i <= count; i++ ) {  	// Loop once for each EEPROM entry
    readID(i); 					// Read an ID from EEPROM, it is stored in storedCard[4]
    if ( checkTwo( find, storedCard ) ) {  	// Check to see if the storedCard read from EEPROM
      return true;
      break; 	// Stop looking we found it
    }
    else {  	// If not, return false
    }
  }
  return false;
}

///////////////////////////////////////// Write Success to EEPROM   ///////////////////////////////////
// Flashes the green LED 3 times to indicate a successful write to EEPROM
void successWrite() {
  digitalWrite(blueLed, LED_OFF); 	// Make sure blue LED is off
  digitalWrite(redLed, LED_OFF); 	// Make sure red LED is off
  digitalWrite(greenLed, LED_OFF); 	// Make sure green LED is on
  delay(200);
  digitalWrite(greenLed, LED_ON); 	// Make sure green LED is on
  delay(200);
  digitalWrite(greenLed, LED_OFF); 	// Make sure green LED is off
  delay(200);
  digitalWrite(greenLed, LED_ON); 	// Make sure green LED is on
  delay(200);
  digitalWrite(greenLed, LED_OFF); 	// Make sure green LED is off
  delay(200);
  digitalWrite(greenLed, LED_ON); 	// Make sure green LED is on
  delay(200);
}

///////////////////////////////////////// Write Failed to EEPROM 寫入EEPROM失敗   ///////////////////////////////////
// Flashes the red LED 3 times to indicate a failed write to EEPROM
//閃爍紅色LED 3次，表示寫入EEPROM失敗
void failedWrite() {
  digitalWrite(blueLed, LED_OFF); 	// Make sure blue LED is off 確保藍色LED熄滅
  digitalWrite(redLed, LED_OFF); 	// Make sure red LED is off 確保紅色LED熄滅
  digitalWrite(greenLed, LED_OFF); 	// Make sure green LED is off 確保綠色LED熄滅
  delay(200);
  digitalWrite(redLed, LED_ON); 	// Make sure red LED is on
  delay(200);
  digitalWrite(redLed, LED_OFF); 	// Make sure red LED is off
  delay(200);
  digitalWrite(redLed, LED_ON); 	// Make sure red LED is on
  delay(200);
  digitalWrite(redLed, LED_OFF); 	// Make sure red LED is off
  delay(200);
  digitalWrite(redLed, LED_ON); 	// Make sure red LED is on
  delay(200);
}

///////////////////////////////////////// Success Remove UID From EEPROM  成功從EEPROM中刪除UID ///////////////////////////////////
// Flashes the blue LED 3 times to indicate a success delete to EEPROM
//閃爍藍色LED 3次，表示成功刪除EEPROM
void successDelete() {
  digitalWrite(blueLed, LED_OFF); 	// Make sure blue LED is off
  digitalWrite(redLed, LED_OFF); 	// Make sure red LED is off
  digitalWrite(greenLed, LED_OFF); 	// Make sure green LED is off
  delay(200);
  digitalWrite(blueLed, LED_ON); 	// Make sure blue LED is on
  delay(200);
  digitalWrite(blueLed, LED_OFF); 	// Make sure blue LED is off
  delay(200);
  digitalWrite(blueLed, LED_ON); 	// Make sure blue LED is on
  delay(200);
  digitalWrite(blueLed, LED_OFF); 	// Make sure blue LED is off
  delay(200);
  digitalWrite(blueLed, LED_ON); 	// Make sure blue LED is on
  delay(200);
}

////////////////////// Check readCard IF is masterCard   ///////////////////////////////////
// Check to see if the ID passed is the master programing card
// 查看是否通過ID是主編程卡
boolean isMaster( byte test[] ) {
  if ( checkTwo( test, masterCard ) )
    return true;
  else
    return false;
}

