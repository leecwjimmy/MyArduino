#include <Timer1.h>
#include <Wire.h>			// IIC communication 
#include <LiquidCrystal_I2C.h>		// LCD 
#include <SPI.h>
//#include <avr/io.h>

//  程式設計：李進衛
// 2014-06-28
// 2014-08-09 增加FSR PAD 偵測模式
//  延續以前的程式架構採用main loop 架構
// main loop 5ms 
// 2014-06-29將microchip程式keyboard process base移植到arduino 系統
// 2014-06-29 移植check time process  to Arduino 
// 2014-06-29 compiler OK 需要再做hardware test 
// Pin 13 has a LED connected on most Arduino boards
// 2014-08-12 再以20pin pad 改為16pin適用於AD49
//
#define T100MS 20  // 
#define T20MS 4
#define T30MS 6
#define T300MS 60
//*******************2014-08-10 由PAD TEST V3 移植過來
#define P_playpause 2
#define  P_3345 3
#define P_version 4
#define P_revers 5
#define P_ss 10

//#define swd 10
#define adcwait 20
#define ADC_margin 250  //ADC  margin
const byte startport = 2;	//設定起始pin為第2腳
const byte stopport = 13;	//設定掃描線結束位置為13pin
int adcval = 0;
int padval = 0;
byte keycount = 0;
boolean f_keyon;
boolean f_play = 0;
boolean f_3345 = 0;
boolean f_rf = 0;
boolean key4ok = 0;
boolean key5ok = 0;
boolean key6ok = 0;
boolean key7ok = 0;
boolean key8ok = 0;
boolean key9ok = 0;
boolean key10ok = 0;
boolean key11ok = 0;
boolean key12ok = 0;

boolean allok = 0;
byte keycode;
//*****************************************************
byte ledState;
boolean  Blink1HZ;
boolean Blink2HZ;
boolean TMAIN;
byte T_CNT1;
byte SQN = 0;
byte TB_100MS;
byte TB_500MS;
byte TB_1SEC;
byte TB_1MINS;
byte TB_1HOURS;
byte T_Blink_Count;
byte KEY_NEW = 0xff;   		// = 0xFF;
byte KEY_OLD;   		// = 0xFF;
byte KEY_CMD;   		// = 0xFF;
byte KEY_CHT;        	// = T20MS;
byte KEY_SQN  = 0;        	// = 1;
boolean F_CMDON;
boolean F_KEYREP;
byte KEY_CHEN;
byte KEY_REP;
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address

void setup()
{

	DDRD = 0x00;  //設定埠D為輸入模式
	pinMode(P_ss,OUTPUT);
 //       PORTB7 = LOW;
  ledState = 0;
  T_CNT1 = 0;  // CLEAR T_CNT1
  TMAIN = 0;
  //pinMode(LED, OUTPUT);
	//*************************
    //****** LCD setting ********
	//*************************
  lcd.begin(16,2);         // initialize the lcd for 20 chars 4 lines, turn on backlight
  lcd.backlight();		//點亮背光
  // Print a message to the LCD.
  lcd.setCursor(2, 0);	//設定游標在第2列第0行
  lcd.print("YA Horng PE ");
  lcd.setCursor(2, 1);
  lcd.print("MD992 150723");
//  Serial.println("Ya horng");
//*******************************************
//設定通信模式
//*******************************************
  Serial.begin(115200);  // 開啟UART 鮑率為 115200
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);			// MSB先送
  SPI.setDataMode(SPI_MODE1);	// setting SPI mode clock 由0--> 1 資料由負緣取樣 
  SPI.setClockDivider(SPI_CLOCK_DIV64); // SPI clock
  
	delay(1000);
	lcd.clear();
  
  // Disable Arduino's default millisecond counter (from now on, millis(), micros(),
  // delay() and delayMicroseconds() will not work)
	disableMillis();
  // Prepare Timer1 to send notifications every 1000000us (1s)
  // On 16 MHz Arduino boards, this function has a resolution of 4us for intervals <= 260000,
  // and a resolution of 16us for other intervals
  // On 8 MHz Arduino boards, this function has a resolution of 8us for intervals <= 520000,
  // and a resolution of 32us for other intervals
//  startTimer1(1000000L);
	startTimer1(5000);
  
	motor_33_speed();
	motor_forward();
	motor_stop();
  
}


void motor_start(void)
{
	lcd.clear();
  lcd.setCursor(2, 0);	//設定游標在第2列第0行
  lcd.print("Start...");

		digitalWrite(P_ss,LOW);	// = 0;
		SPI.transfer(0x01);
		digitalWrite(P_ss,HIGH);	// = 1;
}
//*******************************
// Instant start
//
//*******************************

void motor_stop(void)
{
	lcd.clear();
lcd.setCursor(2, 0);	//設定游標在第2列第0行
  lcd.print("Stop... ");
		digitalWrite(P_ss,LOW);	// = 0;
		SPI.transfer(0x02);
		digitalWrite(P_ss,HIGH);	// = 1;


	
}
//*******************************
// normall start
//
//*******************************

void motor_norm_start(void)
{
	lcd.clear();
		lcd.setCursor(2, 0);	//設定游標在第2列第0行
		lcd.print("NOR Start... ");
		digitalWrite(P_ss,LOW);	// = 0;
		SPI.transfer(0x03);
		digitalWrite(P_ss,HIGH);	// = 1;

}
//*******************************
// norm stop 
//
//*******************************

void motor_norm_stop(void)
{

  	lcd.clear();
		lcd.setCursor(2, 0);	//設定游標在第2列第0行
		lcd.print("NOR Stop... ");
		digitalWrite(P_ss,LOW);	// = 0;
		SPI.transfer(0x04);
		digitalWrite(P_ss,HIGH);	// = 1;


	
}

//******************************
// speed 33
//******************************
void motor_33_speed(void)
{
	lcd.clear();
		lcd.setCursor(2, 0);	//設定游標在第2列第0行
		lcd.print("33 Speed... ");
		digitalWrite(P_ss,LOW);	// = 0;
		SPI.transfer(0x05);
    delayMicroseconds(22);
//		micros(22);
		SPI.transfer(0x00);
		
		digitalWrite(P_ss,HIGH);	// = 1;



	
}
//******************************
// speed 45
//******************************

void motor_45_speed(void)
{
	lcd.clear();	
			lcd.setCursor(2, 0);	//設定游標在第2列第0行
		lcd.print("45 Speed... ");
		digitalWrite(P_ss,LOW);	// = 0;
		SPI.transfer(0x05);
    delayMicroseconds(22);
//		micros(22);
		SPI.transfer(0x01);
		
		digitalWrite(P_ss,HIGH);	// = 1;



}


void motor_forward(void)
{
	lcd.clear();
			lcd.setCursor(2, 0);	//設定游標在第2列第0行
		lcd.print("FORWARD >> ");
		digitalWrite(P_ss,LOW);	// = 0;
		SPI.transfer(0x06);
    delayMicroseconds(22);
//		micros(22);
		SPI.transfer(0x00);
		
		digitalWrite(P_ss,HIGH);	// = 1;


	
}


void motor_reverse(void)
{
	lcd.clear();
  			lcd.setCursor(2, 0);	//設定游標在第2列第0行
		lcd.print("REVERSE << ");
		digitalWrite(P_ss,LOW);	// = 0;
		SPI.transfer(0x06);
    delayMicroseconds(22);
//		micros(22);
		SPI.transfer(0x01);
		
		digitalWrite(P_ss,HIGH);	// = 1;
	
	
	

	
}

void check_version(void)
{
        if (F_KEYREP==1)
        return;
	lcd.clear();        		
//                Massge2 = 9;		// display show "See version"
//        LCD_Set_Cursor(0,0);     // LCD cursor Home /
//	putrsLCD(LCD_MsgB);
//        LCD_Set_Cursor(1,0);     // LCD cursor Home 
//	putrsLCD(LCD_MsgC);      // CLEAR 第二行

		digitalWrite(P_ss,LOW);	// = 0;
		SPI.transfer(0x80);
    delayMicroseconds(22);
//		micros(22);
		byte L_VER = SPI.transfer(0);
    delayMicroseconds(22);
//		micros(22);
		byte M_VER = SPI.transfer(0);
		digitalWrite(P_ss,HIGH);	// = 1;
			lcd.setCursor(2, 0);	//設定游標在第2列第0行
		lcd.print(M_VER);
			lcd.setCursor(4, 0);	//設定游標在第2列第0行
		lcd.print(L_VER);
//		lcd.setCursor(6, 0);	//
//		lcd.print("        ");
          

//                LCD_Set_Cursor(1,0);     // LCD cursor Home 
//                puthexLCD(D1SEC);
//                LCD_Set_Cursor(1,2);     // LCD cursor Home 
//                puthexLCD(D1FRM);
	
	
}	

void read_key(void)
{
		byte PORTD_DATA = PIND; // read portd data to port buffer 
		PORTD_DATA &= 0b00111100; //mark bit0，1，6，7
		
		switch (PORTD_DATA)
		{
			case 0b00111000 : KEY_NEW = 0x01; break;	//play/pause key is press
			case 0b00110100 : KEY_NEW = 0x02; break;	//33/45 key is press
			case 0b00101100 : KEY_NEW = 0x03; break;
			case 0b00011100 : KEY_NEW = 0x04; break;
			default : KEY_NEW = 0xff;
		
		}

}

//************************************
//***** Play/Pause key 
//************************************
void key1_pro(void)
{
  	if (F_KEYREP ==1)
	return;
	if (f_play == 0)
	{
		f_play = 1;
		motor_start();
	}else{
		f_play = 0;
		motor_stop();
	}
}
void key2_pro(void)
{
	if (F_KEYREP ==1)
	return;
	if ( f_3345==0)
	{
		f_3345 = 1;
		motor_33_speed();
	}else{
		f_3345 = 0;
		motor_45_speed();
	}
}
void key3_pro(void)
{
	if (F_KEYREP ==1)
	return;
  check_version();

}
void key4_pro(void)
{
	if (F_KEYREP ==1)
	return;
  if (f_rf == 0)
  {
    f_rf = 1;
  motor_reverse();
  }else{
    f_rf = 0;
    motor_forward();
  }
}
void key5_pro(void)
{
	if (F_KEYREP ==1)
	return;

}




//******************************************************************************
        //;****************************
        //;*****  KEYON_PROCESS   *****
        //;****************************
        //;FOR CHECK KEY FUNCTION WHEN FIRST KEY PRESSED
        //;當KEY按住不放時，仍然只執行一次
void    KEYON_PROCESS()
{
        switch ( KEY_CMD )
        {
		
					case 1 : key1_pro(); break;
					case 2 : key2_pro(); break;
					case 3 : key3_pro(); break;
					case 4 : key4_pro(); break;
//					case 5 : key5_pro(); break;
//					case 6 : key6_pro(); break;
//					case 7 : key7_pro(); break;
//					case 8 : key8_pro(); break;
//					case 9 : key9_pro(); break;
//					case 10 : key10_pro(); break;
//					case 11 : key11_pro(); break;
//					case 12 : key12_pro(); break;
//					case 13 : key13_pro(); break;
//					case 14 : key14_pro(); break;
//					case 15 : key15_pro(); break;
//					case 16 : key16_pro(); break;
					
        }
}


        //;****************************
        //;*****  KEYOFF_PROCESS  *****
        //;****************************
        //;FOR CHECK KEY FUNCTION WHEN KEY RELEASED
void    KEYOFF_PROCESS()
{
        switch ( KEY_CMD )
        {
		
//					case 1: key1_off_pro(); break;
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


void    KEYCHT_440()
{
        KEY_SQN = 1;
        KEYOFF_PROCESS();       // 按鍵放開的第一時間處理
}


void    initial_key()                   //.............. Step 0
{
        KEY_NEW = 0xFF;
        KEY_OLD = 0xFF;
        KEY_CMD = 0xFF;
        KEY_CHT = T20MS;
        KEY_SQN = 1;
}


void    new_old_check()                 //.............. Step 1
{
        if ( (KEY_NEW!=KEY_OLD)&&(KEY_NEW!=0XFF) )
        {
        KEY_OLD = KEY_NEW;
        KEY_CHT = T100MS;
        KEY_SQN = 2;
        }
        if ( (KEY_NEW!=KEY_OLD)&&(KEY_NEW==0XFF) )
        {
        KEY_SQN = 0;
        }
}


void    confirm_key()                   //.............. Step 2
{
        if ( KEY_NEW == KEY_OLD )
        {
                Serial.println("key confirm !!");
                KEY_CMD = KEY_NEW;
                F_CMDON = 1;            // 按鍵生效
                KEY_REP = T300MS;       // 原來的值 T700MS,T500MS
                KEY_CHT = T30MS;
                KEYON_PROCESS();        // 按鍵按下的第一時間處理
                KEY_SQN = 3;
        }
        else
        {
                if ( (KEY_NEW!=0xFF) || ((KEY_NEW==0xFF)&&(KEY_CHT==0)) )
                {
                KEY_OLD = KEY_NEW;
                KEY_SQN = 1;
                }
        }
}


void    hold_key()                      //.............. Step 3
{
        if ( KEY_NEW == KEY_OLD )       // === KEY REPEAT 
        {
                if (KEY_NEW==0)
                {
                        KEY_CHT = T20MS;
                        F_KEYREP = 0;           //
                        KEY_SQN = 4;            //2004.12.07 APPEND BY JEREMY(對應按鍵失效)
                }
                KEY_CHT = T30MS;
                if ( KEY_REP == 0 )
                {
                       KEY_REP  = T300MS;      // 300 ms time out
                       F_CMDON  = 1;           // 按鍵生效
                       F_KEYREP = 1;           // KEY REPEAT
                }
        } else

                if ( (KEY_NEW==0xFF)&&(KEY_CHT==0) )
                {
                        KEY_OLD = KEY_NEW;
                        KEY_CHT = T20MS;
                        F_KEYREP = 0;           // CLEAR KEY REPEAT FLAG
                        KEY_SQN = 4;            //執行release_key的判斷
                }
		
		//        else if ( KEY_NEW < KEY_OLD )                   // === 按住 X 鍵(KEY CODE較大的按鍵)再按 Y 鍵(KEY CODE較小的按鍵)
//        {
//                if ( (KEY_CHT == 0) && (KEY_NEW == 1) || (KEY_NEW == 2) )//KEY_CODE(BEND-)=1,KEY_CODE(BEND+)=2 
//                {
//                        F_KEYREP = 0;
//                        KEYCHT_440();   //須執行按鍵的動作
//                }
 //       }
//        else if ( KEY_NEW > KEY_OLD )                   // === KEY RELEASE或再按住 X 鍵(KEY CODE較大的按鍵)
//        {       
//                if ( (KEY_NEW==0xFF)&&(KEY_CHT==0) )
//                {
//                        KEY_OLD = KEY_NEW;
//                        KEY_CHT = T20MS;
//                        F_KEYREP = 0;           // CLEAR KEY REPEAT FLAG
//                        KEY_SQN = 4;            //執行release_key的判斷
//                }
//                else if ( KEY_NEW != 0xFF )
//                {
//                        if ( (KEY_CHEN==0)&&( (KEY_NEW==1)||(KEY_NEW==2) ) )    // 對應按住 BEND- 放開後迅速按 BEND+ 的處理
//                        {
//                                F_KEYREP = 0;           // CLEAR KEY REPEAT FLAG
//                                KEYCHT_440();           //
//                        }
//                }
//        }
}


void    release_key(void)                   //.............. Step 4
{        
        if ( KEY_NEW == KEY_OLD )
        {
                if ( KEY_CHT == 0 )
                KEYCHT_440();
        }
        else if ( (KEY_NEW!=KEY_OLD)&&(KEY_NEW!=KEY_CMD) )              // 對應在處理 KEY_CHT 的 timming 再按到 BEND +/-  
        {                                                               //
                if ( (KEY_CHEN==0)&&((KEY_NEW==1)||(KEY_NEW==2)) )      //
                {                                                       //
                KEYCHT_440();                                           //
                }                                                       //
                else                                                    //
                {                                                       //
                KEY_OLD = KEY_NEW;                                      //
                KEY_CHT = T30MS;                                        //
                }                                                       //
        }
        else if ( (KEY_NEW!=KEY_OLD)&&(KEY_NEW==KEY_CMD) )
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
        
void    KEY_CHT_PROCESS()
{
        switch ( KEY_SQN )
        {
        case  0:
                initial_key();          // *** 初始所使用的暫存器
                break;
        case  1:
                new_old_check();        // *** 檢查新碼和舊碼
                break;
        case  2:
                confirm_key();          // *** 承認按下的按鍵
                break;
        case  3:
                hold_key();             // *** 保持按鍵按下的處理
                break;
        case  4:
                release_key();          // *** 放開按鍵
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
void    KEY_COD_PROCESS()
{
        if ( F_CMDON == 1 )
        {

        F_CMDON = 0;
                switch ( KEY_CMD )
                {
//                        case 0 : K_PITCH();     break;
//                        case 1 : K_PIT_M();     break;
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


//******************************************************************************


void t100ms_process()
{
  
  if (TB_500MS ==0)
  {
    TB_500MS = 5;
    //0.5sec要處理的程式放這裡
      ledState ^= 1;

  } else {
           
           TB_500MS--;
            
  }
  
  
}

//***************************
//***** 系統時間
//***************************
void check_time()
{
  if (TB_100MS ==0)
  {
    TB_100MS = T100MS;
    t100ms_process();    
  } else {
    
            TB_100MS--;
          }
    
  if (T_Blink_Count ==0)
  {
      T_Blink_Count = 100;    
      Blink1HZ ^=1;
      Blink2HZ ^=1;
      
    } else{
        T_Blink_Count--;
        if (T_Blink_Count == 50) 
        {
                      Blink2HZ ^=1;
       
        }
    
    }
      
  if (T_CNT1 !=0)
   T_CNT1--;
   
   if (KEY_CHT !=0)
   KEY_CHT--;
   
   if (KEY_REP !=0)
   KEY_REP--;
   


}

void display_process()
{
 //   digitalWrite(LED,Blink2HZ);

}

//********************************************************************
//********************************************************************
//*****  Main Loop 
//*****  Program by Chin Wei Lee
//*****  Date : 2014-06 28
//********************************************************************
//********************************************************************

void loop()
{
  while (TMAIN){
				check_time();  
				read_key();
				KEY_CHT_PROCESS();
				TMAIN = 0;
					}

					
}

// Define the function which will handle the notifications
ISR(timer1Event)
{
  TMAIN = 1;
  // Reset Timer1 (resetTimer1 should be the first operation for better timer precision)
//    Serial.println("time interrupt 5ms");
  resetTimer1();
//  check_time();
  // For a smaller and faster code, the line above could safely be replaced with a call
  // to the function resetTimer1Unsafe() as, despite its name, it IS safe to call
  // that function in here (interrupts are disabled)
  
  // Make sure to do your work as fast as possible, since interrupts are automatically
  // disabled when this event happens (refer to interrupts() and noInterrupts() for
  // more information on that)
}
