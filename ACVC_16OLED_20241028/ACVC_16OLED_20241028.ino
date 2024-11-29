/*
開發板： Arduino UNO
日期： 2024-10-28
程式設計： 李進衛
研發部門： 生技課
程式架構： 狀態機
硬體設定
使用2個74LVC138A連接16個128*32的SSD1306 OLED
pin 2 連接74LVC138A 的S0 在程式中命名為OSEL0
pin 3 連接74LVC138A 的S1 在程式中命名為OSEL1
pin 4 連接74LVC138A 的S2 在程式中命名為OSEL2
pin 5 連接74LVC138A 1 的第4 pin 在程式中命名為OEN_1 這一支pin是display 1 的 chip select
pin 6 連接74LVC138A 2 的第4 pin 在程式中命名為OEN_2 這一支pin是display 2 的 chip select
pin 7 連接OLED的RESET pin  在程式中命名為nORST 
pin 8 連接OLED的data/command pin  在程式中命名為OD_C
pin 9 連接一個Tact switch  在程式中命名為key1
SPI ：
MOSI： pin 11
CLOCK ： pin 13
函數庫：Adafruit_SSD1306.h
當按下key16個OLED 由0~15間隔250ms顯示填滿畫素一直到全部顯示完成停止等待key再次按下，當第二次按下開關OLED清除，再依序0~15 在畫素的最外圍顯示
方框如此反覆動作

狀態機架構的步驟：

狀態劃分
1. 等待按鍵狀態（WAIT_FOR_KEY_PRESS）：等待按鍵被按下，然後進入顯示更新的狀態。
2. 顯示填滿畫面狀態（DISPLAY_FILL）：依序顯示 OLED 填滿畫面。
3. 顯示邊框狀態（DISPLAY_BORDER）：依序顯示 OLED 邊框。
4. 等待間隔狀態（WAIT_INTERVAL）：用於處理每個 OLED 之間的顯示間隔。
狀態機架構程式
主要修改部分：
1. State 狀態機：程式的不同階段對應到不同的狀態，這樣能清楚管理行為邏輯。
2. switch-case 控制流程：根據當前的狀態決定要執行哪種操作，例如等待按鍵、顯示畫面等。
3. 等待間隔（WAIT_INTERVAL）：在顯示 OLED 間隔期間等待，這樣可以避免一次性顯示所有的畫面。
4. 按鍵處理邏輯：當按鍵被按下時進入顯示狀態，依照狀態來顯示不同的內容（填滿或邊框）。
這種狀態機的架構使得邏輯更容易擴展和管理。可以進一步添加更多狀態或切換顯示效果。

*/

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
//#include <Adafruit_NeoPixel.h>
#include <FastLED.h>

// 定義 OLED 尺寸和數量
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
//#define OLED_RESET -1 // 未使用硬體重置引腳
#define nORST 7
#define OD_C 8
// 定義74LVC138A的選擇引腳
#define OSEL0 2
#define OSEL1 3
#define OSEL2 4
#define OEN_1 5
#define OEN_2 6
// 定義LED引腳
#define LED_PIN 13 //25 Raspberry  pi pico

// 定義按鍵引腳
#define key1 9
/*
Adafruit_SSD1306 display[NUM_OLEDS] = {
  Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OD_C, nORST,  -1),
  Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OD_C, nORST,  -1),
  Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OD_C, nORST,  -1),
  Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OD_C, nORST,  -1),
  Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OD_C, nORST,  -1),
  Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OD_C, nORST,  -1),
  Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OD_C, nORST,  -1),
  Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OD_C, nORST,  -1),
  Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OD_C, nORST,  -1),
  Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OD_C, nORST,  -1),
  Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OD_C, nORST,  -1),
  Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OD_C, nORST,  -1),
  Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OD_C, nORST,  -1),
  Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OD_C, nORST,  -1),
  Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OD_C, nORST,  -1),
  Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OD_C, nORST,  -1),
  // 重複直到16個物件
}; */
// 定義OLED diaaplay物件，Reset 和 CS 不定義有外部使用者自行控制 使用軟體SPI 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, 11 /*&SPI*/,13 /*CLK*/, 8/*DC*/, -1 /*OLED_RESET*/, -1/*OLED_CS0*/);



// 狀態機的狀態
enum State { WAIT_FOR_KEY_PRESS, DISPLAY_FILL, DISPLAY_BORDER, WAIT_INTERVAL };
State state = WAIT_FOR_KEY_PRESS;

int currentOLED = 0;
bool keyPressed = false;
unsigned long lastUpdateTime = 0;
unsigned long interval = 250; // 每個 OLED 間隔 50ms
// LED 閃爍變數
unsigned long ledLastToggleTime = 0;
bool ledState = false;
const unsigned long ledInterval = 500; // 2Hz 閃爍，每次切換狀態間隔 500ms

// 定義WS2812全彩LED
#define NUM_LEDS 1
#define BRIGHTNESS 50   //2024/03/07 by leecw modify 60
#define DATA_PIN 23 // RP2040使用pin26 // Arduino UNO使用pin 6 by lee chin wei
//#define WS2812_PIN 23
//#define NUM_PIXELS 1
CRGB leds[NUM_LEDS];
//Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, WS2812_PIN, NEO_GRB + NEO_KHZ800);
// WS2812 閃爍變數
unsigned long ws2812LastToggleTime = 0;
bool ws2812State = false;
const unsigned long ws2812Interval = 1000; // 1Hz 閃爍，每次切換狀態間隔 1000ms
//********************
// 選擇要控制的 OLED
//********************
void selectOLED(int oledIndex) {
    if (oledIndex < 8) {
    digitalWrite(OEN_1, LOW); // 啟用 74LVC138A 1
    digitalWrite(OEN_2, HIGH);
    Serial.println("74LVC138A 1 enable pin setting !!");
  } else {
    digitalWrite(OEN_1, HIGH);
    digitalWrite(OEN_2, LOW); // 啟用 74LVC138A 2
    Serial.println("74LVC138A 2 enable pin setting !!");
  } 

  digitalWrite(OSEL0, oledIndex & 0x01);
  digitalWrite(OSEL1, (oledIndex >> 1) & 0x01);
  digitalWrite(OSEL2, (oledIndex >> 2) & 0x01);

}

void setup() {
  // setting serial port for debug
  Serial.begin(115200);
  Serial.println("Power On initial");
  Serial.println("ACVC_16OLED_20241028.ino");
  // 初始化引腳
  pinMode(OSEL0, OUTPUT);
  pinMode(OSEL1, OUTPUT);
  pinMode(OSEL2, OUTPUT);
  pinMode(OEN_1, OUTPUT);
  pinMode(OEN_2, OUTPUT);
  pinMode(nORST, OUTPUT);

	pinMode(key1, INPUT_PULLUP);
	pinMode(7, OUTPUT);
	pinMode(8, OUTPUT);
	pinMode(23, OUTPUT);

  
// 初始化 LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(nORST, HIGH); // 初始為關閉狀態

    // 初始化 WS2812
//  strip.begin();
//  strip.show(); // 初始化所有 LED 為關閉狀態
//     FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);  // GRB ordering is typical


  
//*********************************
//***** OLED reset 獨立出來   
//*********************************
digitalWrite(nORST,LOW); // 所有的 OLED reset 
delay(10);               //維持10ms
digitalWrite(nORST,HIGH); // 所有的OLED reset pin 拉HI
 
  // 初始化 OLED 顯示

  for (int i = 0; i <16;i++)
  {
    selectOLED(i);
  	display.begin(SSD1306_SWITCHCAPVCC);
    display.display();
    display.clearDisplay();  

  } 

}

void loop() {
  // 處理 LED 閃爍
//  handleLEDBlink();

  // 處理 WS2812 全彩 LED 閃爍
//  handleWS2812Blink();

  switch (state) {
    case WAIT_FOR_KEY_PRESS:
      if (digitalRead(key1) == LOW && !keyPressed) {
        keyPressed = true;
        currentOLED = 0;
        lastUpdateTime = millis();
        state = DISPLAY_FILL; // 切換到填滿畫面狀態
      } else if (digitalRead(key1) == HIGH) {
        keyPressed = false;
      }
      break;

    case DISPLAY_FILL:
      if (millis() - lastUpdateTime >= interval) {
        if (currentOLED < 16) {
          lastUpdateTime = millis();
          selectOLED(currentOLED);
          display.fillScreen(SSD1306_WHITE); // 填滿畫面
          display.display();
          currentOLED++;
        } else {
          currentOLED = 0;
          state = DISPLAY_BORDER; // 填滿畫面後，切換到畫邊框狀態
        }
      }
      break;

    case DISPLAY_BORDER:
      if (millis() - lastUpdateTime >= interval) {
        if (currentOLED < 16) {
          lastUpdateTime = millis();
          selectOLED(currentOLED);
          display.clearDisplay(); // 清除畫面
          display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE); // 畫邊框
          display.display();
          currentOLED++;
        } else {
          currentOLED = 0;
          state = DISPLAY_FILL; //WAIT_FOR_KEY_PRESS; // 完成後回到等待按鍵
        }
      }
      break;

    case WAIT_INTERVAL:
      if (millis() - lastUpdateTime >= interval) {
        lastUpdateTime = millis();
        // 這個狀態不再需要 mode 判斷
      }
      break;
  }
}

// 處理 LED 以 2Hz 閃爍
void handleLEDBlink() {
  unsigned long currentMillis = millis();
  if (currentMillis - ledLastToggleTime >= ledInterval) {
    ledLastToggleTime = currentMillis;
    ledState = !ledState; // 切換 LED 狀態
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
  }
}
// 處理 WS2812 全彩 LED 以 1Hz 閃爍粉紅色
void handleWS2812Blink() {
  unsigned long currentMillis = millis();
  if (currentMillis - ws2812LastToggleTime >= ws2812Interval) {
    ws2812LastToggleTime = currentMillis;
    FastLED.setBrightness(BRIGHTNESS);

    ws2812State = !ws2812State; // 切換 WS2812 狀態
    if (ws2812State) {
//      strip.setPixelColor(0, strip.Color(255, 20, 147)); // 粉紅色 (RGB: 255, 20, 147)
        leds[1] = CRGB::White;
    } else {
//      strip.setPixelColor(0, strip.Color(0, 0, 0)); // 關閉 LED
        leds[1] = CRGB::Black;
    }
//    strip.show(); // 更新 LED 顯示
      FastLED.show();
  }
}


