/*
開發板 : Arduino UNO
日期 : 2024/10/07
程式設計 : 李進衛

功能:

1. 當LOCK 開關 ON 時 2pin送出一個0.5sec HI 信號之後維持LO
2. 當LOCK 開關 OFF 時 2pin送出一個0.5sec HI 信號之後維持LO
3. 當LOCK 開關 ON 時 2pin送出一個0.5sec HI 信號時第3pin偵測IO 狀態是HI 或者LO 的狀態。
4. 2pin的信號和3pin的信號一致時不發送RF 信號

2024/10/07 初版
2024/10/08 修正當開關在off的狀態RF Relay不同步問題


*/
#define mainloop_time 10	//主迴圈時間
#define TBASE 10 // main loop time base for 5mS
#define TBASE100 100	//

#define T20MS 20 / TBASE // 4
#define T30MS 30 / TBASE // 6
#define T100MS 100 / TBASE
#define T150MS 150 / TBASE
#define T200MS 200 / TBASE
#define T250MS 250 / TBASE
#define T300MS 300 / TBASE // 60
#define T500MS 500 / TBASE
#define T1000MS 1000 / TBASE
#define T2000MS 2000 / TBASE
#define T1SEC 1000 / TBASE100
#define T2SEC 2000 / TBASE100

unsigned long previousMillis = 0;  // 

// constants won't change:
const long interval = 10;  // (milliseconds)

//**************************************************
//定義 IO PORT

#define P_RFout 2
#define P_RELAYin 3
#define P_LOCKSW 4
#define P_WORK_LED 13
//***************************************************

boolean Blink1HZ; // 1HZ閃爍旗標
boolean Blink2HZ; // 2HZ閃爍旗標
boolean RF_ONOFF =0;
boolean F_LOCKSW =0;
boolean F_RFIN =0;
boolean F_LOCKON = 0;
boolean F_LOCKOFF = 0;
boolean F_LOCKstatus;
boolean F_RFINstatus;
int T_CNT1;
int MODE = 0;
int SQN = 0;
int TB_100MS;
int TB_250MS;
int TB_500MS;
int TB_1SEC;
int TB_1MINS;
int TB_1HOURS;
int T_Blink_Count;
int T_LED_delay = 0; // 100ms 為一個單位


boolean F_CMDON;
boolean F_KEYREP;

byte KEY_NEW = 0xff; // = 0xFF;
byte KEY_OLD = 0xff; // = 0xFF;
byte KEY_CMD;		 // = 0xFF;
byte KEY_CHT;		 // = T20MS;
byte KEY_SQN = 0;	 // = 1;
byte KEY_CHEN;
byte KEY_REP;

byte Pulse_SQN = 0;

void setup() 
{                
	Serial.begin(115200);
	Serial.println("RF_RELAY20241008.ino"); // 版本
	pinMode(P_RFout, OUTPUT);//set the pin to be OUTPUT pin.
	pinMode(P_WORK_LED, OUTPUT);//set the pin to be OUTPUT pin.
	pinMode(P_LOCKSW, INPUT_PULLUP);//設定為輸入腳
	pinMode(P_RELAYin, INPUT_PULLUP);//設定為輸入腳
  
	digitalWrite(P_RFout,HIGH);// relay off 
//**************************************************************
//***開機檢查狀態如果開關狀態和繼電器輸出狀態不符合要發射一次RF
//**************************************************************
/*
	F_LOCKstatus = digitalRead(P_LOCKSW);
	F_RFINstatus = digitalRead(P_RELAYin);
  if (F_LOCKstatus != F_RFINstatus)
  {
	  Serial.println("Power ON LOCK and RFin <>");
	  Pulse_SQN = 0;
	  RF_ONOFF = 1;
  } */
}
/*
***************************
**** RF LOW 500ms 
***************************
*/
void Pulse_000(void)
{
	
		digitalWrite(P_RFout,LOW);
		Pulse_SQN = 1;
		T_CNT1 = T500MS;
		Serial.println("RF pin is Low!!");
}
/*
***************************
**** 等待 500ms 
***************************
*/
void Pulse_100(void)
{
		if (T_CNT1 !=0)
			return;
		Serial.println("RF pin is HIGH!!");
		digitalWrite(P_RFout,HIGH);
	F_LOCKstatus = digitalRead(P_LOCKSW);
  Serial.println(F_LOCKstatus);
	F_RFINstatus = digitalRead(P_RELAYin);
  Serial.println(F_RFINstatus);
  if (F_LOCKstatus != F_RFINstatus)
  {
		
		Pulse_SQN = 2;
    T_CNT1 = T500MS;
  } else {
  
		RF_ONOFF =0; //停止發射RF
		
  }
}

void Pulse_200(void)
{

 if (T_CNT1 !=0)
 return;
//	  Serial.println("Power ON LOCK and RFin <>");
	  Pulse_SQN = 0;
	  RF_ONOFF = 1;
  Serial.println("Pulse SQN 2 !!");

	
}
/*
**********************************************
***** 發射 RF duty寬度500ms
**********************************************
*/
void Pulse(void)
{
		if (RF_ONOFF==0)	//如果RF 發射除能就停止發射RF信號
			return;
	switch(Pulse_SQN)
	{
	case 0: Pulse_000(); break; // RF 拉LOW
	case 1: Pulse_100(); break; //wait 500ms HI 
	case 2: Pulse_200(); break; //檢查開關的狀態和繼電器的狀態是否一致如果沒有再發射一次RF信號
	}
	
}


//***************************
//***** 系統時間
//***************************
void check_time()
{
	if (TB_100MS == 0)
	{
		TB_100MS = T100MS;
		//t100ms_process();
	}
	else
	{

		TB_100MS--;
	}

	if (TB_250MS == 0)
	{
		TB_250MS = T250MS;
		Blink2HZ ^= 1; // Blink2HZ與1 互斥或  【反向的意思】
	}
	else
	{
		TB_250MS--;
	}
	if (T_Blink_Count == 0)
	{
		T_Blink_Count = 50; // 100;
		//      Blink1HZ ^=1;	// Blink1HZ與1 互斥或  【反向的意思】
		//      Blink2HZ ^=1;	// Blink2HZ與1 互斥或  【反向的意思】
	}
	else
	{
		T_Blink_Count--;
		if (T_Blink_Count == 25) // 50)
		{
			//                      Blink2HZ ^=1;
		}
	}

	if (T_CNT1 != 0)
		T_CNT1--;

	if (KEY_CHT != 0)
		KEY_CHT--;

	if (KEY_REP != 0)
		KEY_REP--;


	
}

//*****************************
//*** 系統工作LED
//*****************************
void WORK_TEST(void)
{
	if (Blink2HZ == 1)
	{
		digitalWrite(P_WORK_LED, HIGH);
	}
	else
	{

		digitalWrite(P_WORK_LED, LOW);
	}
}




void ReadKey(void)
{

	
	if (digitalRead(P_LOCKSW) ==0)
	{
		if (F_LOCKON ==1)
			return;
		Pulse_SQN = 0;
		RF_ONOFF = 1;
		F_LOCKON = 1;
		F_LOCKOFF =0;
		Serial.println("LOCK SW ON !!");
	} else{
		if (F_LOCKOFF ==1)
			return;
		Pulse_SQN = 0;
		RF_ONOFF = 1;
		F_LOCKON = 0;
		F_LOCKOFF =1;
	Serial.println("key off!!");	
		
	}   
 		
	

}



/*
**********************************************
***** 主迴圈
**********************************************
*/

void loop() 
{
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;
    check_time();
	ReadKey();
//	KEY_CHT_PROCESS();
    Pulse();
    WORK_TEST();
  
  
  }  

}