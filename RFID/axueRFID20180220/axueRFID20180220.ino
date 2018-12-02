/*
Date 2017-09-05
Arduino UNO
MRC522 RFID chip





*/


#include <SPI.h>
#include <MFRC522.h>     // 引用程式庫
//#include <Servo.h>         // 引用伺服馬達程式庫

#define RST_PIN      9  // 讀卡機的重置腳位
#define SS_PIN       10  // 晶片選擇腳位
//#define SERVO_PIN    2   // 伺服馬達的控制訊號接腳
#define P_LED			2
bool lockerSwitch = 0;  //false;  // 伺服馬達的狀態

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
	pinMode(P_LED,OUTPUT);
  Serial.begin(115200);
  Serial.println();
  Serial.print("size of RFIDTag:");
  Serial.println(sizeof(RFIDTag));
  Serial.print("size of tag:");
  Serial.println(sizeof(tags));
  Serial.println("RFID reader is ready!");

  SPI.begin();
  mfrc522.PCD_Init();       // 初始化MFRC522讀卡機模組
 // servo.attach(SERVO_PIN);  // 將伺服馬達物件附加在數位2腳
//  locker(lockerSwitch);
}

void loop() {
  
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

      if (!foundTag) {  									// 若掃描到紀錄之外的標籤，則顯示"Wrong card!"。
							Serial.println("Wrong card!");
            digitalWrite(P_LED,LOW);
					}
       
//					    mfrc522.PICC_HaltA();  // 讓卡片進入停止模式      
    } else {
      mfrc522.PICC_HaltA();  // 讓卡片進入停止模式
          Serial.println("no date");
			      digitalWrite(P_LED,LOW);
//		        lockerSwitch = 0;
	}
//            delay(1000);
//          Serial.println("READY WAIT!!");
// 	while ( (mfrc522.PICC_IsNewCardPresent() ==1) && (mfrc522.PICC_ReadCardSerial() ==1));
//          Serial.println("no date");
//              mfrc522.PICC_HaltA();  // 讓卡片進入停止模式      
//			      digitalWrite(P_LED,LOW);
	
     
}
