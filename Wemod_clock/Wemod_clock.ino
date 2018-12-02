//#include <LiquidCrystal_I2C.h>

 /* 網路時鐘，配合4位顯示LED範例
 * 請注意：本範例必須配合已焊控制板的4位LED哦！
 * 
 * 網拍賣場：
 * http://goods.ruten.com.tw/item/show?21649712601470
 *
 * WeMos D1 R2：
 * http://goods.ruten.com.tw/item/show?21639776266588
 * 
 * 程式整理：Jason--> 2018-03-05 by lee chin Wei append  I2C LCD  
 * http://blog.geeks.tw
 *
 * 本範例要配合Frankie Chu寫的Digital Tube。
 *
 * 下載網址：
 * https://brainy-bits.com/wp-content/uploads/2015/01/DigitalTube_Library.zip
 *
 * 本範例要配合 Paul Stoffregen寫的Time函式庫：
 * https://github.com/PaulStoffregen/Time
 */

#include <TimeLib.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#include <TM1637.h> // "
#include <Wire.h>					// IIC communication I2C使用的函數
//#include <avr/io.h>
#include <LiquidCrystal_I2C.h>

#define CLK D6 //CLK pin，可自行決定接到哪個PIN
#define DIO D7 //DIO pin，可自行決定接到哪個PIN
TM1637 tm1637(CLK,DIO);
LiquidCrystal_I2C lcd(0x3f,16,2); // Check I2C address of LCD, normally 0x27 or 0x3F

int point_flag =0;

const char ssid[] = "your SSID";  //  your network SSID (name)
const char pass[] = "YOUR PASSWORD";       // your network password
char daysOfTheWeek[7][12] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
// NTP Server:
static const char ntpServerName[] = "us.pool.ntp.org";

const int timeZone = 8;     //台灣時區+8

int hourH = 0;     //
int hourL = 0;     //
int minH = 0;     //
int minL = 0;     //
int secH = 0;     //
int secL = 0;     //
int monH = 0;
int monL = 0;
int dayH = 0;
int dayL = 0;
int dayOfWeek = 0;
//建立與NTP的連線
WiFiUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets

time_t getNtpTime();

void setup()
{
  tm1637.init();
  tm1637.set(BRIGHT_TYPICAL); //BRIGHT_TYPICAL = 2,BRIGHT_DARKEST = 0,BRIGHTEST = 7;
  
  tm1637.display(0,0);  //設定每一位燈號顯示的內容，參數1：燈號，參數2：顯示的數字
  tm1637.display(1,0); 
  tm1637.display(2,0);
  tm1637.display(3,0);
  Wire.begin(D14,D15);  //一定要加這一行還可以正常顯示出來 (0,2); //sda=0 | D3, scl=2 | D4
  lcd.backlight();
  lcd.clear();
//  lcd.noBlink();
  lcd.home();                // At column=0, row=0
 
  Serial.begin(115200);
  while (!Serial) ; // Needed for Leonardo only
  delay(250);

  //建立WiFi連線
  Serial.println("NTP 網路時鐘");
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.print("IP number assigned by DHCP is ");
  Serial.println(WiFi.localIP());
  Serial.println("Starting UDP");
  Udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());
  Serial.println("waiting for sync");
  setSyncProvider(getNtpTime);
  setSyncInterval(300);
}

time_t prevDisplay = 0; // 記錄上一次的時間，如果相同，LED就不用更新

void loop()
{
  if (timeStatus() != timeNotSet) {
    if (now() != prevDisplay) { //時間變化就顯示
      prevDisplay = now();   //把目前時間丟給prevDisplay
//      Serial.println(now());
      digitalClockDisplay(); //呼叫顯示時間的副程式
    }
  }
}

void digitalClockDisplay()
{
//    lcd.home();                // At column=0, row=0
  //設定每一位燈號顯示的內容，參數1：燈號，參數2：顯示的數字
  tm1637.display(0,hour() / 10);  
  tm1637.display(1,hour() % 10); 
  tm1637.display(2,minute() / 10);
  tm1637.display(3,minute() % 10);
//==============================================
  monH = (month()/10);
  monL = (month()%10);
  dayH = (day()/10);
  dayL = (day()%10);
  
  hourH = (hour()/10) ; 
//  hourH = hourH/10;
//  Serial.print(hourH); 
  hourL = (hour()%10);
//  Serial.print(hourL); 
  minH = (minute() / 10);
//  Serial.print(minH); 
  
  minL = (minute() % 10);
//  Serial.print(hourL); 
  
  secH = (second() /10);
//  Serial.print(secH); 
  
  secL = (second() %10);
//  Serial.println(secL); 
  
//================================================
    lcd.setCursor(0, 0);
    lcd.print(year());
    lcd.setCursor(4,0);
    lcd.print("/");
    lcd.setCursor(5,0);
    lcd.print(monH,DEC);
    lcd.setCursor(6,0);
    lcd.print(monL,DEC);
    lcd.setCursor(7,0);
    lcd.print("/");
    lcd.setCursor(8,0);
    lcd.print(dayH,DEC);
    lcd.setCursor(9,0);
    lcd.print(dayL,DEC);
    
//	dayOfWeek = weekday();
    lcd.setCursor(11,0);
	lcd.print(daysOfTheWeek[weekday()-1]);
/*
switch(dayOfWeek){
  case 1:
    lcd.setCursor(11,0);
    lcd.print("Sun");
  
//    Serial.println("Sunday");
    break;
  case 2:
    lcd.setCursor(11,0);
    lcd.print("Mon");
//    Serial.println("Monday");
    break;
  case 3:
    lcd.setCursor(11,0);
    lcd.print("Tue");
//    Serial.println("Tuesday");
    break;
  case 4:
    lcd.setCursor(11,0);
    lcd.print("Wed");
  
//    Serial.println("Wednesday");
    break;
  case 5:
    lcd.setCursor(11,0);
    lcd.print("Thu");
//    Serial.println("Thursday");
    break;
  case 6:
    lcd.setCursor(11,0);
    lcd.print("Fri");
  
 //   Serial.println("Friday");
    break;
  case 7:
    lcd.setCursor(11,0);
    lcd.print("Sat");
  
//    Serial.println("Saturday");
    break;
  }   
  
  */  
    
    

//  Serial.print(year());
//  Serial.print("/");
//  Serial.print(month());
//  Serial.print("/");
//  Serial.print(day());
 // Serial.print("===>");
 // Serial.println(weekday());
    lcd.setCursor(0, 1);
    lcd.print(hourH,DEC);
    lcd.setCursor(1, 1);
    lcd.print(hourL,DEC);
    lcd.setCursor(2, 1);
    lcd.print(":");
    lcd.setCursor(3, 1);
    lcd.print(minH,DEC);
    lcd.setCursor(4, 1);
    lcd.print(minL,DEC);
    lcd.setCursor(5, 1);
    lcd.print(":");
     lcd.setCursor(6, 1);
    lcd.print(secH,DEC);
     lcd.setCursor(7, 1);
    lcd.print(secL,DEC);
     lcd.setCursor(16,1);
    
//    lcd.home();                // At column=0, row=0
 // lcd.noBlink();
 
 // Serial.print(hour());
 // Serial.print(":");
 // Serial.print(minute());
 // Serial.print(":");
 // Serial.println(second());
    
  //顯示時和分之間的：，如果目前是顯示，就設定下次不顯示
  if(point_flag){
    tm1637.point(POINT_OFF);
    point_flag = 0;
  }else{
    tm1637.point(POINT_ON);
    point_flag = 1;
  }
}


/*-------- NTP 程式碼 ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
