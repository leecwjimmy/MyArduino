/*
這個程式是用來縮短PIN網站的時間
李進衛



*/



#include <ESP8266WiFi.h>

const char* ssid = "******";
const char* password = "*******";

const char* host = "www.google.com";
unsigned long previousMillis = 0;        // will store last time LED was updated

// constants won't change :
const long interval = 5;           // interval at which to blink (milliseconds)****************設定main loop time for 50ms ****************
boolean  Blink1HZ;  //1HZ閃爍旗標
boolean Blink2HZ; //2HZ閃爍旗標
int T_CNT1;
int SQN = 0;
int MODE = 0;
int TB_100MS = 0;
int TB_250MS = 0;
int TB_500MS = 0;
int TB_1SEC = 0;
int TB_1MINS = 0;
int TB_1HOURS = 0;
int T_Blink_Count;
int T_CNT1SEC = 0;   // 1sec counter delay ?sec  由這一個變數控制 
int T_CNT1MIN = 0;   // 1mins counter delay ? mins由這一個變數控制 
int ping_cnt = 0;		// ping count
int retry_times = 0;
byte KEY_NEW = 0xff;      // = 0xFF;
byte KEY_OLD = 0xff;      // = 0xFF;
byte KEY_CMD;       // = 0xFF;
byte KEY_CHT;         // = T20MS;
byte KEY_SQN  = 0;          // = 1;
byte READ_SQN = 0;
float temp;
boolean F_CMDON;
boolean F_KEYREP;

byte KEY_CHEN;
byte KEY_REP;
#define TBASE 5   // main loop time base for 5mS
// #define T100MS 20  // 
#define T20MS 20/TBASE  //4
#define T30MS 30/TBASE  //6
#define T100MS 100/TBASE
#define T150MS 150/TBASE
#define T200MS 200/TBASE
#define T250MS 250/TBASE
#define T300MS 300/TBASE  //60
#define T500MS 500/TBASE
#define T1000MS 1000/TBASE
#define T2000MS 2000/TBASE
#define T1SEC   1000/100

#define P_WORK_LED D4  // for D1 min // for wemos d1 led pin -->D9


void setup()
{
	pinMode(P_WORK_LED,OUTPUT);		// 74HC595 PIN 11 CLOCK PIN FOR OUTPUT
  Serial.begin(115200);
  Serial.println();

  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected");
}


void loop()
{
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
  
     //******************* 程式區**********************
    check_time();
	WORK_TEST();
	switch(MODE)
	{
		case 0: ESPping();	break;
		
	}
  
  
  
  
  
  }	

 // delay(1000);
}

void	ESP_000(void)
{
	if (T_CNT1SEC != 0)
		return;
	ESP_ping();
	T_CNT1SEC = 1;
//	SQN = 1;
	
}

void	ESP_100(void)
{
	if (T_CNT1SEC != 0)
		return;
		SQN = 0;
		T_CNT1SEC = 1;
	
}

void ESPping(void)
{
		switch(SQN)
		{
			case 0 :  ESP_000(); break;
			case 1 :  ESP_100(); break;
//			case 2: ESP_200(); break;
			
		}
	
}


void	ESP_ping(void)
{
  WiFiClient client;

  Serial.printf("\n[Connecting to %s ... ", host);
  if (client.connect(host, 80))
  {
    Serial.println("connected]");

    Serial.println("[Sending a request]");
    client.print(String("GET /") + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Connection: close\r\n" +
                 "\r\n"
                );

    Serial.println("[Response:]");
    while (client.connected())
    {
      if (client.available())
      {
        String line = client.readStringUntil('\n');
        Serial.println(line);
        Serial.println("OK OK OK ..........OK!!");
		
      }
    }
    client.stop();
    Serial.println("\n[Disconnected]");
  }
  else
  {
    Serial.println("connection failed!]");
        Serial.println("NG NG NG ..........NG!!");
    client.stop();
  }	
	
	
}

void sec1proc(void)
{
    if (TB_1MINS ==0)
    {
      TB_1MINS = 60;
      if (T_CNT1MIN ==0)
      return;
      T_CNT1MIN--;
      }else{
            TB_1MINS--;
//            if (F_pingNG ==0)
//            {
//              check_wifi();
//            }
        }
  
  }
void t100ms_process()
{
  if (TB_500MS ==0)
  {
    TB_500MS = 5;
    //0.5sec要處理的程式放這裡
 //     ledState ^= 1;
      Blink1HZ ^=1;  // Blink1HZ與1 互斥或  【反向的意思】
//    bittest();
  } else {
           
           TB_500MS--;
            
  }
//  Print_VR_VAL();
  // counter 1sec
  if ( TB_1SEC ==0)
  {
    // 1sec 時間到處理
    TB_1SEC = 10;
    sec1proc();       //1sec 處理
  //  temp  = dht.readTemperature();
    
    if (T_CNT1SEC ==0)
    return;
    T_CNT1SEC--;
    
    } else{
            TB_1SEC--;
      } 

      
}

//****************************************************
//***** 系統時間
//****************************************************
void check_time()
{
  if (TB_100MS ==0)
  {
    TB_100MS = T100MS;
    t100ms_process();    
  } else {
    
            TB_100MS--;
          }
    
  if (TB_250MS ==0)
  {
      TB_250MS = T250MS;
      Blink2HZ ^=1;  // Blink2HZ與1 互斥或  【反向的意思】
      
    } else{
        TB_250MS--;
      }
  if (T_Blink_Count ==0)
  {
      T_Blink_Count = 50; //100;    
//      Blink1HZ ^=1; // Blink1HZ與1 互斥或  【反向的意思】
//      Blink2HZ ^=1; // Blink2HZ與1 互斥或  【反向的意思】
      
    } else{
        T_Blink_Count--;
        if (T_Blink_Count == 25)  //50) 
        {
//                      Blink2HZ ^=1;
       
        }
    
    }
      
  if (T_CNT1 !=0)
   T_CNT1--;
   
   if (KEY_CHT !=0)
   KEY_CHT--;
   
   if (KEY_REP !=0)
   KEY_REP--;
 
}

//***********************************************
//***** 控制開機時LED閃爍表示治具的程式有在動作
//***********************************************
void WORK_TEST(void)
{
	if (Blink1HZ ==1)
	{
		digitalWrite(P_WORK_LED,HIGH);
	
	}else{
	
		digitalWrite(P_WORK_LED,LOW);
	
	
	}


}

