/*
開發板 : Arduino UNO
日期 : 2024/10/07
程式設計 : 李進衛
功能:

1. 當LOCK 開關 ON 時 2pin送出一個0.5sec HI 信號之後維持LO
2. 當LOCK 開關 OFF 時 2pin送出一個0.5sec HI 信號之後維持LO
3. 當LOCK 開關 ON 時 2pin送出一個0.5sec HI 信號時第3pin偵測IO 狀態是HI 或者LO 的狀態。
4. 2pin的信號和3pin的信號一致時不發送RF 信號



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
byte locksw_sqn = 0;
byte locksw_cht = 0;
byte tactsw_sqn = 0;
byte tactsw_cht = 0;
byte Pulse_SQN = 0;

void setup() 
{                
	Serial.begin(115200);
	Serial.println("RF_RELAY20241007.ino"); // 版本
	pinMode(P_RFout, OUTPUT);//set the pin to be OUTPUT pin.
	pinMode(P_WORK_LED, OUTPUT);//set the pin to be OUTPUT pin.
	pinMode(P_LOCKSW, INPUT_PULLUP);//設定為輸入腳
	pinMode(P_RELAYin, INPUT_PULLUP);//設定為輸入腳
  
	digitalWrite(P_RFout,HIGH);// relay off 
//**************************************************************
//***開機檢查狀態如果開關狀態和繼電器輸出狀態不符合要發射一次RF
//**************************************************************
	F_LOCKstatus = digitalRead(P_LOCKSW);
	F_RFINstatus = digitalRead(P_RELAYin);
  if (F_LOCKstatus != F_RFINstatus)
  {
	  Serial.println("Power ON LOCK and RFin <>");
	  Pulse_SQN = 0;
	  RF_ONOFF = 1;
  } 
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
	F_RFINstatus = digitalRead(P_RELAYin);
  if (F_LOCKstatus != F_RFINstatus)
  {
		
		Pulse_SQN = 2;
  } else {
  
		RF_ONOFF =0; //停止發射RF
		
  }
}

void Pulse_200(void)
{

 
//	  Serial.println("Power ON LOCK and RFin <>");
	  Pulse_SQN = 0;
	  RF_ONOFF = 1;


	
}

void Pulse(void)
{
		if (RF_ONOFF==0)	//RF ON/OFF
			return;
	switch(Pulse_SQN)
	{
	case 0: Pulse_000(); break; // RF 拉LOW
	case 1: Pulse_100(); break; //wait 500ms HI 
	case 2: Pulse_200(); break;
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

/*
*****************************

*****************************

*/


void key0_pro(void)
{
	if (F_KEYREP == 1) // 已經執行過了就返回不再重複執行
  		return;
//		F_LOCKSW = 1;
		Pulse_SQN = 0;
		RF_ONOFF = 1;
		Serial.println("LOCK SW ON!!");

}

void key1_pro(void)
{
  Serial.print("key1_pro!!");
	if (F_KEYREP == 1) // 已經執行過了就返回不再重複執行
		return;
		Pulse_SQN = 0;
		RF_ONOFF = 1;
		Serial.println("LOCK SW OFF!!");
}


//**************************************************************************************
//******************************************************************************
//;****************************
//;*****  KEYON_PROCESS   *****
//;****************************
//;FOR CHECK KEY FUNCTION WHEN FIRST KEY PRESSED
//;當KEY按住不放時，仍然只執行一次
void KEYON_PROCESS()
{
		switch (KEY_CMD)
		{

		case 0:
			key0_pro();
			break;
//		case 1:
//			key1_pro();
//			break;
			
		}

}

//;****************************
//;*****  KEYOFF_PROCESS  *****
//;****************************
//;FOR CHECK KEY FUNCTION WHEN KEY RELEASED
void KEYOFF_PROCESS()
{
	switch (KEY_CMD)
	{

							case 0: key1_pro(); break;
		//					case 2: key2_off_pro(); break;
		//					case 3: key3_off_pro(); break;
		//					case 4: key4_off_pro(); break;
		//					case 5: key5_off_pro(); break;
		//					case 6: key6_off_pro(); break;
		//					case 7: key7_off_pro(); break;
		//					case 8: key8_off_pro(); break;
		//					case 9: key9_off_pro(); break;
		//					case 10: key10_off_pro(); break;
		//					case 11: key11_off_pro(); break;
		//					case 12: key12_off_pro(); break;
		//					case 13: key13_off_pro(); break;
		//					case 14: key14_off_pro(); break;
		//					case 15: key15_off_pro(); break;
		//					case 16: key16_off_pro(); break;
	}
}

void KEYCHT_440()
{
	KEY_SQN = 1;
	KEYOFF_PROCESS(); // 按鍵放開的第一時間處理
}

void initial_key() //.............. Step 0
{
	KEY_NEW = 0xFF;
	KEY_OLD = 0xFF;
	KEY_CMD = 0xFF;
	KEY_CHT = T20MS;
	KEY_SQN = 1;
}

void new_old_check() //.............. Step 1
{
	if ((KEY_NEW != KEY_OLD) && (KEY_NEW != 0XFF))
	{
		KEY_OLD = KEY_NEW;
		KEY_CHT = T100MS;
		KEY_SQN = 2;
	}
	if ((KEY_NEW != KEY_OLD) && (KEY_NEW == 0XFF))
	{
		KEY_SQN = 0;
	}
}

void confirm_key() //.............. Step 2
{
	if (KEY_NEW == KEY_OLD)
	{
		//               Serial.println("key confirm !!");
		KEY_CMD = KEY_NEW;
		F_CMDON = 1;	  // 按鍵生效
		KEY_REP = T200MS; // 原來的值 T700MS,T500MS
		KEY_CHT = T30MS;
		KEYON_PROCESS(); // 按鍵按下的第一時間處理
		KEY_SQN = 3;
	}
	else
	{
		if ((KEY_NEW != 0xFF) || ((KEY_NEW == 0xFF) && (KEY_CHT == 0)))
		{
			KEY_OLD = KEY_NEW;
			KEY_SQN = 1;
		}
	}
}

void hold_key() //.............. Step 3
{
	if (KEY_NEW == KEY_OLD) // === KEY REPEAT
	{
		if (KEY_NEW == 0)
		{
			KEY_CHT = T20MS;
			F_KEYREP = 0; //
			KEY_SQN = 4;  // 2004.12.07 APPEND BY JEREMY(對應按鍵失效)
		}
		KEY_CHT = T30MS;
		if (KEY_REP == 0)
		{
			KEY_REP = T200MS;  // 200 ms time out
			F_CMDON = 1;	   // 按鍵生效
			F_KEYREP = 1;	   // KEY REPEAT
      Serial.println("key rep = 1");
			KEY_COD_PROCESS(); // 2016-03-21 by leecw Append for key on process
		}
	}
	else

		if ((KEY_NEW == 0xFF) && (KEY_CHT == 0))
	{
		KEY_OLD = KEY_NEW;
		KEY_CHT = T20MS;
		F_KEYREP = 0; // CLEAR KEY REPEAT FLAG
		KEY_SQN = 4;  // 執行release_key的判斷
	}
}

void release_key(void) //.............. Step 4
{
	if (KEY_NEW == KEY_OLD)
	{
		if (KEY_CHT == 0)
			KEYCHT_440();
	}
	else if ((KEY_NEW != KEY_OLD) && (KEY_NEW != KEY_CMD))		   // 對應在處理 KEY_CHT 的 timming 再按到 BEND +/-
	{															   //
		if ((KEY_CHEN == 0) && ((KEY_NEW == 1) || (KEY_NEW == 2))) //
		{														   //
			KEYCHT_440();										   //
		}														   //
		else													   //
		{														   //
			KEY_OLD = KEY_NEW;									   //
			KEY_CHT = T30MS;									   //
		}														   //
	}
	else if ((KEY_NEW != KEY_OLD) && (KEY_NEW == KEY_CMD))
	{
		KEY_OLD = KEY_NEW;
		KEY_SQN = 3;
	}
}

//;***********************************
//;*****  KEY CHATTER  &  CHECK  *****
//;*****  HAVE  ANY  KEY  INPUT  *****
//;***********************************
//;IN     :KEY_NEW,KEY_SQN,KEY_CHT
//;OUT    :KEY_OLD,KEY_CMD,F_KEYREP
//;KEY_SQN =  0 --> INITIAL ALL REGISTER
//;KEY_SQN =  1 --> CHECK KEY_NEW & KEY_OLD
//;KEY_SQN =  2 --> KEY ON CHATTER CHECK
//;KEY_SQN =  3 --> KEY REPEAT CHECK
//;KEY_SQN =  4 --> KEY OFF CHATTER
//;KEY_SQN = 32 --> 按住 X 按鍵再按 Y 鍵

void KEY_CHT_PROCESS()
{
	//	Serial.println(KEY_SQN);
	switch (KEY_SQN)
	{
	case 0:
		initial_key(); // *** 初始所使用的暫存器
		break;
	case 1:
		new_old_check(); // *** 檢查新碼和舊碼
		break;
	case 2:
		confirm_key(); // *** 承認按下的按鍵
		break;
	case 3:
		hold_key(); // *** 保持按鍵按下的處理
		break;
	case 4:
		release_key(); // *** 放開按鍵
		break;
	default:
		KEY_SQN = 0;
		break;
	}
}

//;*****************************************
//;*****  KEY CODE PROCESS SUBROUTINE  *****
//;*****************************************
//;IN     :KEY_CMD
//;OUT    :當KEY按住不放時，利用KEY_REP的設定，可重複執行KEY CODE動作
void KEY_COD_PROCESS()
{
	if (F_CMDON == 1)
	{

		F_CMDON = 0;
		switch (KEY_CMD)
		{
			//                        case 0 : K_PITCH();     break;
			//                        case 1 :   K_counter_add(); break;  //K_PIT_M();     break;
			//                        case 2 : K_PIT_P();     break;
			//                        case 3 : K_STOP();      break;
			//                        case 4 : K_SINGLE();    break;
			//                        case 5 : K_TIME();      break;
			//                        case 6 : K_PLYPAS();    break;
			//                        case 7 : K_PTEN();      break;
			//                        case 8 : K_CUE1();      break;
			//                        case 9 : K_BSKIP();     break;
			//                        case 10: K_FSKIP();     break;
			//                        case 11: K_FBS();       break;
			//                        case 12: K_FFS();       break;
			//                        case 13: K_OPCL();      break;
			//                        case 14: K_BPM();       break;
		}
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