/*
程式設計：李進衛
日期：2018-12-02
MCU：ESP32S www.doit.aw
功能：OTA測試
我使用的板子為DOIT  相容的板子


*/




#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

const char* ssid = "your ssid";
const char* password = "your password";
unsigned long previousMillis = 0;        // will store last time LED was updated

// constants won't change:
const long interval = 10;           // interval at which to blink (milliseconds)
#define TBASE  10  //main loop 
#define T100MS 20
//#define T500MS 500/5
//#define T1SEC 1000/5
#define T20MS 20/TBASE
#define T30MS 30/TBASE
#define T300MS 300/TBASE
boolean  Blink1HZ;
boolean Blink2HZ;
boolean TMAIN;
byte ledState;

byte T_CNT1;
byte SQN = 0;
byte TB_100MS;
byte TB_500MS;
byte TB_1SEC;
byte TB_1MINS;
byte TB_1HOURS;
byte T_Blink_Count;
byte KEY_NEW = 0xff;      // = 0xFF;
byte KEY_OLD;       // = 0xFF;
byte KEY_CMD;       // = 0xFF;
byte KEY_CHT;         // = T20MS;
byte KEY_SQN  = 0;          // = 1;
boolean F_CMDON;
boolean F_KEYREP;
byte KEY_CHEN;
byte KEY_REP;
byte counter = 0;

void setup() {
    T_CNT1 = 0;  // CLEAR T_CNT1
  ledState = 0;
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
   ArduinoOTA.setHostname("Myesp32");

  // No authentication by default
   ArduinoOTA.setPassword("Ab1234");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

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

void  LED_000(void)
{
  if (Blink2HZ ==0)
  return;   
  digitalWrite(LED_BUILTIN,Blink2HZ);
  SQN = 1;
  counter++;
  if (counter >=60)
  {
    SQN = 2;
    counter = 0;
  }
}

void  LED_100(void)
{
  if (Blink2HZ ==1)
  return;   
  digitalWrite(LED_BUILTIN,Blink2HZ);
  counter++;
  
  SQN = 0;
  if (counter >=60)
  {
    SQN = 2;
    counter = 0;
  }
    
  
  
}

void  LED_200(void)
{
  if (Blink1HZ ==0)
  return;   
  digitalWrite(LED_BUILTIN,Blink1HZ);
  SQN = 3;
  counter++;
  if (counter >=60)
  {
    SQN = 0;
    counter = 0;
  }
}

void  LED_300(void)
{
  if (Blink1HZ ==1)
  return;   
  digitalWrite(LED_BUILTIN,Blink1HZ);
  counter++;
  
  SQN = 2;
  if (counter >=30)
  {
    SQN = 0;
    counter = 0;
    
  }
  
  
}

void  LED_proce(void)
{
  switch(SQN)
  {
    case 0: LED_000();break;
    case 1: LED_100();break;
    case 2: LED_200();break;
    case 3: LED_300();break;
    
    
    
  }
}
void loop() {
    unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    //
    previousMillis = currentMillis;
    
  ArduinoOTA.handle();
  check_time();  
  LED_proce();
  
  }   //Loop
}
