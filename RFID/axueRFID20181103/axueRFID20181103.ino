/*
Date 2017-09-05 V0.00
	2018-11-03 v0.01
	
Arduino UNO
MRC522 RFID chip
程式設計：李進衛

2018-11-03 新增EEPROM 的處理
2018-11-03 新增加EEPROM 清除程式
2018-11-03 新增加MFRC522 天線增益設定



*/
  /*
  *****************************************************************************************************************************
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
  *****************************************************************************************************************************
  mfrc522.PCD_SetAntennaGain
  
  
  */
  
  
  //mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
#include <EEPROM.h>     // 我們將從/向EEPROM讀取和寫入PICC的UID We are going to read and write PICC's UIDs from/to EEPROM 
#include <SPI.h>
#include <MFRC522.h>     // 引用程式庫

#define redLed 7		// Set Led Pins
#define greenLed 6
#define blueLed 5
#define RST_PIN      9  // 讀卡機的重置腳位
#define SS_PIN       10  // 晶片選擇腳位
#define P_LED			2
#define	P_wipeB		4
#define	P_relay		5
#define	LED_ON	1
#define	LED_OFF	0
bool lockerSwitch = 0;  //false;  // 伺服馬達的狀態
//*********************************************************************************************************************************
boolean match = false;          // initialize card match to false
boolean programMode = false;	// initialize programming mode to false
boolean replaceMaster = false;

int successRead;		// 如果我們從Reader讀取成功，則保留變數整數 Variable integer to keep if we have Successful Read from Reader

byte storedCard[4];		// 存儲從EEPROM讀取的ID Stores an ID read from EEPROM 
byte readCard[4];		// 存儲從RFID模塊讀取的掃描ID Stores scanned ID read from RFID Module 
byte masterCard[4];		// 存儲從EEPROM讀取的主卡ID Stores master card's ID read from EEPROM 
byte oldCard[4];		//舊的卡號
//**********************************************************************************************************************************


//Servo servo;    // 宣告伺服馬達物件

struct RFIDTag {   // 定義結構
   byte uid[4];
   char *name;
};

struct RFIDTag tags[] = {  // 初始化結構資料，請自行修改RFID識別碼。
  {{0xA6,0xA1,0xA0,0xBB}, "Huang 1"},
  {{0xF0,0x8D,0x0F,0x7C}, "Huang 2"},
  {{21,8,10,83}, "Espruino"}
};

byte totalTags = sizeof(tags) / sizeof(RFIDTag);  // 計算結構資料筆數，結果為3。

MFRC522 mfrc522(SS_PIN, RST_PIN);  // 建立MFRC522物件

// 開鎖或關鎖
void locker(bool toggle) {
  if (toggle) {
	  digitalWrite(P_LED,HIGH);
   Serial.println("open the door !!");
 //     servo.write(90);  // 開鎖
  } else {
 //     servo.write(0);   // 關鎖
   Serial.println("Close the door !!");
	  digitalWrite(P_LED,LOW);
  }
//  delay(15);    // 等伺服馬達轉到定位
}

void setup() {
	//******************************
	//*** 設定IO PORT
	//******************************
	pinMode(P_LED,OUTPUT);
	pinMode(P_wipeB,INPUT_PULLUP);	//This is a  IO PORT setting for PULL HIGH
	pinMode(P_relay,OUTPUT);
	//*******************************
  Serial.begin(115200);		//設定串列鮑率
  Serial.println();
  Serial.print("size of RFIDTag:");
  Serial.println(sizeof(RFIDTag));
  Serial.print("size of tag:");
  Serial.println(sizeof(tags));
  Serial.println("RFID reader is ready!");

  SPI.begin();
  mfrc522.PCD_Init();       // 初始化MFRC522讀卡機模組
  mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_38dB);	//設定MFRC522天線增益
  EEPROM_init(); //EEPROM 是否需要全部清除檢查
    if (digitalRead(P_wipeB) == LOW) 
    {  // when button pressed pin should get low, button connected to ground
        Check_EEPROM();    
    }  
	Set_Mast_Card();	// 2018-11-04  李進衛建立
}


//****************************************
//***** 設定主卡
//***** 2018-11-04
//***** 李進衛建立
//****************************************

void	Set_Mast_Card(void)
{
  if (EEPROM.read(1) != 143) {
    Serial.println(F("No Master Card Defined"));
    Serial.println(F("Scan A PICC to Define as Master Card"));
	// 等待卡片設定
    do {
      successRead = getID();            // sets successRead to 1 when we get read from reader otherwise 0當讀取讀取器時，將successRead設置為1，否則為0
      digitalWrite(blueLed, LED_ON);    // Visualize Master Card need to be defined需要定義可視化主卡
      delay(200);
      digitalWrite(blueLed, LED_OFF);
      delay(200);
    } while (!successRead);                  // Program will not go further while you not get a successful read如果您沒有成功讀取，程序將不會更進一步
    
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
	
	
}

//**********************************************
//***** 檢查EEPROM 記憶體內容
//***** 2018-11-04
//***** 李進衛 
//**********************************************

void  Check_EEPROM(void)
{
          for (int x = 0; x < EEPROM.length(); x = x + 1) {    //Loop end of EEPROM address
            int xy;
           xy = EEPROM.read(x);
          Serial.print(xy,HEX);  

          }  
  
  
  }

//**********************************************
//***** 初始化EEPROM 記憶體內容
//***** 按住wipe按鍵-->開機-->按住15sec-->清除所有EEPROM爲‘0’
//***** 如果15sec內放開此按鍵，就取消清除所有的EEPROM記憶體功能
//***** 2018-11-04
//***** 李進衛 
//**********************************************

  void	EEPROM_init(void)
{
  //Wipe Code if Button Pressed while setup run (powered on) it wipes EEPROM
  //如果在設置運行（打開電源）時按下按鈕，則擦除代碼擦除EEPROM
  if (digitalRead(P_wipeB) == LOW) {	// when button pressed pin should get low, button connected to ground
    digitalWrite(P_LED, LED_ON);	// Red Led stays on to inform user we are going to wipe
    Serial.println(F("Wipe Button Pressed"));
    Serial.println(F("You have 15 seconds to Cancel"));
    Serial.println(F("This will be remove all records and cannot be undone"));
	for(int tim = 0; tim <100; tim++){
		Serial.print(".");
		delay(150);  
		if (digitalRead(P_wipeB) == HIGH)	//按鍵放開？
    {
      Serial.println(" ");
      Serial.println("Break timer counter...");
        break;
      }
	}                      // Give user enough time to cancel operation
    delay(20);
    if (digitalRead(P_wipeB) == LOW) {    // If button still be pressed, wipe EEPROM
      Serial.println(F("Starting Wiping EEPROM"));
      for (int x = 0; x < EEPROM.length(); x = x + 1) {    //Loop end of EEPROM address
				Clear_All_EEPROM();

		}
		digitalWrite(P_LED, LED_OFF);	// EEPROM all erase OK
		
	}		
	
}
}

//*****************************************************
//***** 清除所有的EEPROM 記憶體爲0
//*****************************************************
void	Clear_All_EEPROM(void)
{
	      for (int x = 0; x < EEPROM.length(); x = x + 1) 
		  {    //Loop end of EEPROM address
				EEPROM.update(x, 0);  

          }

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


//////////////////////////////////////// Read an ID from EEPROM //////////////////////////////
void readID( int number ) {
  int start = (number * 4 ) + 2; 		//弄清楚起始位置 Figure out starting position
  for ( int i = 0; i < 4; i++ ) { 		// 循環4次以獲得4個位元組 Loop 4 times to get the 4 Bytes
    storedCard[i] = EEPROM.read(start + i); 	// 將從EEPROM讀取的值分配給陣列 Assign values read from EEPROM to array
  }
}

///////////////////////////////////////// Add ID to EEPROM   ///////////////////////////////////
void writeID( byte a[] ) {
  if ( !findID( a ) ) { 		// 在我們寫入EEPROM之前，請檢查我們之前是否看過這張卡！ Before we write to the EEPROM, check to see if we have seen this card before!
    int num = EEPROM.read(0); 		// 獲取已用空間的數量，位置0存儲ID卡的數量 Get the numer of used spaces, position 0 stores the number of ID cards
    int start = ( num * 4 ) + 6; 	// 找出下一程式所佔的位置開始的地方 Figure out where the next slot starts
    num++; 								// 將計數器增加一個 Increment the counter by one 
    EEPROM.write( 0, num ); 		// 將新計數寫入計數器 Write the new count to the counter 
    for ( int j = 0; j < 4; j++ ) { 	// 循環4次 Loop 4 times
      EEPROM.write( start + j, a[j] ); 	// 將陣列值寫入正確位置的EEPROM Write the array values to EEPROM in the right position
    }
//    successWrite();		//LED燈號表示
    Serial.println(F("Succesfully added ID record to EEPROM"));
  }
  else {
//    failedWrite();
    Serial.println(F("Failed! There is something wrong with ID or bad EEPROM"));
  }
}

///////////////////////////////////////// Remove ID from EEPROM 從EEPROM中刪除ID   ///////////////////////////////////
void deleteID( byte a[] ) {
  if ( !findID( a ) ) { 		// 在我們從EEPROM中刪除之前，檢查一下我們是否有這張卡！ Before we delete from the EEPROM, check to see if we have this card! 
//    failedWrite(); 			// If not 如果沒有
    Serial.println(F("Failed! There is something wrong with ID or bad EEPROM"));
  }
  else {
    int num = EEPROM.read(0); 	//獲取已用空間的數量，位置0存儲ID卡的數量  Get the numer of used spaces, position 0 stores the number of ID cards 
    int slot; 			//找出卡在記憶體中的位置號碼 Figure out the slot number of the card 
    int start;			// = ( num * 4 ) + 6; //找出下一個開始的地方  Figure out where the next slot starts 
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
//    successDelete();
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

////////////////////// Check readCard IF is masterCard   ///////////////////////////////////
// Check to see if the ID passed is the master programing card
// 查看是否通過ID是主編程卡
boolean isMaster( byte test[] ) {
  if ( checkTwo( test, masterCard ) )
    return true;
  else
    return false;
}


void loop() {
	
	
	getID();
//	delay(500);
}
/*
  
//    if (mfrc522.PICC_IsNewCardPresent())
//    {
//      Serial.println("card!!");
//    }
    if (mfrc522.PICC_ReadCardSerial())
    {
      Serial.println("Read card");
    }
//    while(1);
    // 確認是否有新卡片
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial() ) {
      byte *id = mfrc522.uid.uidByte;   // 取得卡片的UID
      byte idSize = mfrc522.uid.size;   // 取得UID的長度
      bool foundTag = false;    // 是否找到紀錄中的標籤，預設為「否」。
      
      for (byte i=0; i<totalTags; i++) {
        if (memcmp(tags[i].uid, id, idSize) == 0) {  // 比對陣列資料值
          Serial.println(tags[i].name);  // 顯示標籤的名稱
          foundTag = true;  // 設定成「找到標籤了！」
        digitalWrite(P_LED,HIGH);
          
//          lockerSwitch = 1;  //!lockerSwitch;  // 切換鎖的狀態
//          locker(lockerSwitch);          // 開鎖或關鎖
          break;  // 退出for迴圈
        }
      }
      //************************************
      // 沒有找到的卡片 處理
      //************************************
      if (!foundTag) {  									// 若掃描到紀錄之外的標籤，則顯示"Wrong card!"。
							Serial.println("Wrong card!");
//            digitalWrite(P_LED,LOW);
			      digitalWrite(P_LED,HIGH);
				  delay(100);
			      digitalWrite(P_LED,LOW);
				  delay(100);			
					}
       
//					    mfrc522.PICC_HaltA();  // 讓卡片進入停止模式      
    } else {
      //********************************
      //卡片已經離開或者沒有卡片的處理
      //********************************
      mfrc522.PICC_HaltA();  // 讓卡片進入停止模式
          Serial.println("no date");
			      digitalWrite(P_LED,HIGH);
				  delay(50);
			      digitalWrite(P_LED,LOW);
				  delay(50);
				  
//		        lockerSwitch = 0;
	}

	
     
} */
