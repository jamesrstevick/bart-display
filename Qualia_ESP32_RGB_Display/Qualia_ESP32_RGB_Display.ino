/*
  Display code starting from Crash_McBang: https://forums.adafruit.com/viewtopic.php?t=209953
  
  Adapted from the Adafruit and Xark's PDQ graphicstest sketch.
  
  See end of file for original header text and MIT license info.
*/

/*
 *    Use Arduino IDE 1
 *    Must place board in boot mode to program
 *    Place board in boot mode by holding Boot0 and pressing reset
 *    After programming, press reset, and code will load and COM will switch
 * 
 *    Board Manager:
 *      esp32 by EspressifSystems v2.0.17 (v2 works, v3 may have compile issues)
 *    Libraries:
 *      GFX_Library_for_Arduino by Moon On Our Nation v1.5.6 (includes HD458002)
 * 
*/


#include <Arduino_GFX_Library.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
// #include <Fonts/FreeMono9pt7b.h> // TODO comment
// #include <Fonts/FreeSans9pt7b.h> // TODO comment



// #include <Adafruit_XCA9554.h>

// Create an instance of the Adafruit_XCA9554 class
// Adafruit_XCA9554 expander;

/*******************************************************************************
 * Start of Arduino_GFX setting
 ******************************************************************************/
Arduino_XCA9554SWSPI *expander = new Arduino_XCA9554SWSPI(
    PCA_TFT_RESET, PCA_TFT_CS, PCA_TFT_SCK, PCA_TFT_MOSI,
    &Wire, 0x3F);

//  Qualia S3 RGB-666 with HD458002C40 4.58" 320x960 Bar Display
Arduino_ESP32RGBPanel *rgbpanel_320x960 = new Arduino_ESP32RGBPanel(
    TFT_DE, TFT_VSYNC, TFT_HSYNC, TFT_PCLK,
    TFT_R1, TFT_R2, TFT_R3, TFT_R4, TFT_R5,
    TFT_G0, TFT_G1, TFT_G2, TFT_G3, TFT_G4, TFT_G5,
    TFT_B1, TFT_B2, TFT_B3, TFT_B4, TFT_B5,
    1 /* hsync_polarity */, 10 /* hsync_front_porch */, 10 /* hsync_pulse_width */, 50 /* hsync_back_porch */,
    1 /* vsync_polarity */, 15 /* vsync_front_porch */, 2 /* vsync_pulse_width */, 17 /* vsync_back_porch */,
    1 /* pclk_active_neg */, GFX_NOT_DEFINED /* prefer_speed */, false /* useBigEndian */, 0 /* de_idle_high */,
    0 /* pclk_idle_high */
    );

Arduino_RGB_Display *gfx = new Arduino_RGB_Display
(
// 4.58" 320x960 rectangle bar display, adapted from 320x820 display
    320 /* width */, 960 /* height */, rgbpanel_320x960, 0 /* rotation */, true /* auto_flush */,
    expander, GFX_NOT_DEFINED /* RST */, HD458002C40_init_operations, sizeof(HD458002C40_init_operations),
    80 /* col_offset1 */, 0 /* row_offset1 */, 8 /* col_offset2 */, 0 /* row_offset2 */
);
/*******************************************************************************
 * End of Arduino_GFX setting
 ******************************************************************************/



/*******************************************************************************
 * Start of WiFi Client settingsa
 ******************************************************************************/
// const char* ssid     = "JH"; // Change this to your WiFi SSID
// const char* password = "TooManyPlants"; // Change this to your WiFi password
// const char* ssid     = "FlyWithMe"; // Change this to your WiFi SSID
// const char* password = "tothemoon1636"; // Change this to your WiFi password
// const char* ssid     = "BEARmesh"; // Change this to your WiFi SSID
// const char* password = "kj#975mm"; // Change this to your WiFi password
// const char* ssid     = "CoralCove88_Mesh"; // Change this to your WiFi SSID
// const char* password = "CoralCove88"; // Change this to your WiFi password
const char* ssid     = "James Stevick's iPhone"; // Change this to your WiFi SSID
const char* password = "stevick5"; // Change this to

const char* serverName = "https://api.bart.gov/api/stn.aspx?cmd=stns&key=QWMH-PE9Y-9WST-DWE9&json=y";

#define STAT_ARR_SIZE 55UL
#define JSON_DOC_SIZE 20480UL
String station_name[STAT_ARR_SIZE];
String station_abbr[STAT_ARR_SIZE];
int numStations = 0;
String selected_station = ""; 
// char payload[10289];

#define SMALL_TXT 7
#define LARGE_TXT 8
// #define SMALL_TXT 4
// #define LARGE_TXT 5


// Button stuff
#define UP_BUTTON 5 
#define DN_BUTTON 6 
int buttonDebounceDelay = 20;
int medPressTime = 1500;
int longPressTime = 10000;



uint8_t one_station = 10;
int buttonID;
int buttonPress;

#ifdef ESP32
#undef F
#define F(s) (s)
#endif

int32_t w, h, n, n1, cx, cy, cx1, cy1, cn, cn1;
uint8_t tsa, tsb, tsc, ds;






// Check for button press for certain amount of time in milliseconds
// 0 - no press
// 1 - momentary press 
// 2 - short press ~2 sec
// 3 - long press ~10 sec
int buttonPressed(unsigned long timeMillis){
  while (!expander->digitalRead(UP_BUTTON)){
    delay(5);
  }
  bool isButton = false;
  unsigned long t0 = millis();
  while((millis() - t0) <= timeMillis){
    isButton = !expander->digitalRead(UP_BUTTON);
    if(isButton == true){
      delay(buttonDebounceDelay);
      isButton = !expander->digitalRead(UP_BUTTON);
      if(isButton == true){
        for(int i=1; i<=round(medPressTime/100); i++){
          delay(100);
          isButton = !expander->digitalRead(UP_BUTTON);
          if(isButton == false){
            break; 
          } else if (i==round(medPressTime/100)){
            // for(int i=round(medPressTime/100); i<=round(longPressTime/100); i++){
            //   delay(100);
            //   isButton = !expander->digitalRead(UP_BUTTON);
            //   if(isButton == false){
            //     break; 
            //   } else if (i==round(longPressTime/100)){
            //     return 3;
            //   }
            // }
            
            return 2;
          }   
        }       
        return 1;
      }
    }
    delay(5);
  }
  return 0;
}


// gfx->fillScreen(BLACK);
// delay(250);
// gfx->setRotation(1);
// gfx->setCursor(10, 10);
// gfx->setTextSize(5);
// gfx->setTextColor(RED, BLACK);
// gfx->print(F(destination.substring(0, 12)));
// gfx->print(F("     "));
// gfx->print(F(timeMinutes[0]));
// // if(sizeof(timeMinutes, 2)==1){
// if(timeMinutes[1]){
//   gfx->print(F(","));
//   gfx->print(F(timeMinutes[1]));
// }
// gfx->print(F(" MIN"));
// delay(2000);


void clearDisplay(){
  gfx->fillScreen(BLACK);
}


void stationSelect(){
  int index = 0;
  bool selected = false;
  delay(100);
  clearDisplay();
  gfx->setCursor(12, 132);
  // gfx->setCursor(12, 191);
  gfx->setTextSize(SMALL_TXT); 
  gfx->print(F("SELECT STATION"));

  while(buttonPressed(1000) == 0){
    delay(1);
  }

  clearDisplay();
  delay(200);
  gfx->setCursor(12, 132);
  // gfx->setCursor(12, 191);
  gfx->print(station_name[index]);

  while(!selected){
    if(index == numStations){
      index = 0;
    }
    int buttonType = buttonPressed(60000);
    if (buttonType==0){
      continue;
    } else if (buttonType==1){
      index++;
      clearDisplay();
      gfx->setCursor(12, 132);
      // gfx->setCursor(12, 191);
      delay(200);
      String station = station_name[index];
      station.toUpperCase();
      gfx->print(station);
    } else if (buttonType==2){
      selected_station = station_abbr[index];
      clearDisplay();
      gfx->setCursor(12, 12);
      // gfx->setCursor(12, 191);
      gfx->println(F("STATION SELECTED:"));
      gfx->print(selected_station);
      selected = true;
    }
  }

  delay(1000);
  clearDisplay();
  gfx->setCursor(12, 132);
  // gfx->setCursor(12, 191);
  gfx->print(F("STATION SELECTED!"));
  Serial.println("STATION SELECTED");
  delay(5000);
}
























void setup()
{
  Serial.begin(115200);
  // while (!Serial) delay(10); // Wait for port to open if native USB
  
  expander->pinMode(UP_BUTTON, INPUT); // UP
  expander->pinMode(DN_BUTTON, INPUT); // DN

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Serial.println("WiFi connected");
  // Serial.println("IP address: ");
  // Serial.println(WiFi.localIP());
  
  // Serial.setDebugOutput(true);

#ifdef GFX_EXTRA_PRE_INIT
  GFX_EXTRA_PRE_INIT();
#endif

  // Init Display
  if (!gfx->begin())
  // if (!gfx->begin(80000000)) /* specify data bus speed */
  {
    Serial.println("gfx->begin() failed!");
  }

  // gfx->setRotation(0); // Rotate to be landscape
  // h = gfx->width();
  // w = gfx->height();
  // gfx->setRotation(0); // Rotate to be landscape
  
  w = gfx->width();  
  h = gfx->height();
  n = min(w, h);
  n1 = n - 1;
  cx = w / 2;
  cy = h / 2;
  cx1 = cx - 1;
  cy1 = cy - 1;
  cn = min(cx1, cy1);
  cn1 = cn - 1;
  // Desired text size. 1 is default 6x8, 2 is 12x16, 3 is 18x24, etc
  tsa = ((w <= 176) || (h <= 160)) ? 1 : (((w <= 240) || (h <= 240)) ? 2 : 3); // text size A
  tsb = ((w <= 272) || (h <= 220)) ? 1 : 2;                                    // text size B
  tsc = ((w <= 220) || (h <= 220)) ? 1 : 2;                                    // text size C
  ds = (w <= 160) ? 9 : 12;                                                    // digit size

  clearDisplay();
  gfx->setRotation(3);
  gfx->setCursor(0, 0);
  gfx->setTextColor(RED, BLACK);
  gfx->setTextSize(1);
  // gfx->setFont(&FreeSans9pt7b); // TODO comment



#ifdef GFX_BL
 digitalWrite(GFX_BL, HIGH);
#endif

#ifdef PCA_TFT_BACKLIGHT
  digitalWrite(PCA_TFT_BACKLIGHT, HIGH);
#endif
}


static inline uint32_t micros_start() __attribute__((always_inline));
static inline uint32_t micros_start()
{
  uint8_t oms = millis();
  while ((uint8_t)millis() == oms);
  return micros();
  
}




void loop(void)
{

  delay(1000);

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverName);
    // http.setInsecure();
    int httpCode = http.GET();

    if (httpCode > 0) {
      String payload = http.getString();
      // Serial.println("HTTP Response code: " + String(httpCode));
      // Serial.println("Received JSON:");
      // Serial.println(payload);

      // Parse JSON
      DynamicJsonDocument doc(JSON_DOC_SIZE);
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        JsonArray stations = doc["root"]["stations"]["station"];
        for (JsonObject station : stations) {
      
          // Serial.println(station["name"].as<char *>());
          // Serial.println(station["abbr"].as<char *>());
          station_name[numStations] = station["name"].as<char *>();
          station_abbr[numStations] = station["abbr"].as<char *>();
          numStations ++;
        } 
        Serial.print("Stations: ");
        Serial.println(numStations);
      } else {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
      }
    } else {
      Serial.println("HTTP GET failed, code: " + String(httpCode));
    }

    http.end();
  }

  // selected_station = station_abbr[one_station];
  stationSelect();


  // payload[0] = '\0';



  if (WiFi.status() == WL_CONNECTED) {
    
    HTTPClient http;
    http.begin("https://api.bart.gov/api/etd.aspx?cmd=etd&orig="+selected_station+"&key=QWMH-PE9Y-9WST-DWE9&json=y");
    int httpCode = http.GET();
    if (httpCode > 0) {
      String payload = http.getString();
      // Serial.println("HTTP Response code: " + String(httpCode));
      // Serial.println("Received JSON:");
      // Serial.println(payload);

      DynamicJsonDocument doc(JSON_DOC_SIZE);
      DeserializationError error = deserializeJson(doc, payload);

      
      JsonArray stations = doc["root"]["station"];
      Serial.println("Created array");
      for (JsonObject station : stations) {
        // Serial.print("Name: ");
        // Serial.println(station["name"].as<char *>());
        JsonArray etds = station["etd"];
        
        uint8_t etd_index = 0;
        gfx->setTextColor(RED, BLACK);
        gfx->setRotation(3);

        for (JsonObject etd : etds) {
          String destination = (etd["destination"].as<char *>());
          destination.toUpperCase();
          Serial.print("Destination: ");
          Serial.println(destination);
          JsonArray estimates = etd["estimate"];
          uint8_t timeMinutes[3] = {0};
          uint8_t index = 0;
          uint8_t car_length;
          for (JsonObject estimate : estimates) {
            String tmpMin = estimate["minutes"].as<char *>();
            timeMinutes[index] = (uint8_t)(tmpMin.toInt());
            String carLength = estimate["length"].as<char *>();
            car_length = (uint8_t)(carLength.toInt());
            // Serial.print("Minutes: ");
            // Serial.println(timeMinutes[index]);    
            index++;
          }

          if(etd_index % 2 == 0) {
            gfx->fillScreen(BLACK);
            delay(1000);
          
            gfx->setCursor(10, 12);
            // gfx->setCursor(10, 80); // 72 (sz8) + 8
            gfx->setTextSize(LARGE_TXT);
            
            gfx->print(F(destination.substring(0, 11)));
            
            gfx->setCursor(570, 16); // 576 max to fit 9 characters // 12 + 4
            // gfx->setCursor(600, 76); // 72 (sz8) + 8 - 4(.5)
            gfx->setTextSize(SMALL_TXT);
            if(timeMinutes[0] < 10){
              gfx->print(F(" "));
            }
            if(timeMinutes[1] < 10){
              gfx->print(F(" "));
            }
            gfx->print(F(timeMinutes[0]));
            gfx->print(F(","));
            gfx->print(F(timeMinutes[1]));
            gfx->print(F(" MIN"));

            gfx->setCursor(10, 88); // 12 + 64 (sz8) + 12
            // gfx->setCursor(10, 151); // 72 (sz8) + 8 + 63 (sz7) + 8
            gfx->setTextSize(SMALL_TXT);

            gfx->print(F(car_length));
            gfx->print(F(" CAR TRAIN"));
          }
          else
          {
            gfx->setCursor(10, 172);
            // gfx->setCursor(10, 240); // 160 + 72 (sz8) + 8
            gfx->setTextSize(LARGE_TXT);
            
            gfx->print(F(destination.substring(0, 11))); 
            
            gfx->setCursor(570, 176); // 576 max to fit 9 characters // 160 + 12 + 4
            // gfx->setCursor(600, 236); // 160 + 72 (sz8) + 8 - 4(.5)
            gfx->setTextSize(SMALL_TXT);
            if(timeMinutes[0] < 10){
              gfx->print(F(" "));
            }
            if(timeMinutes[1] < 10){
              gfx->print(F(" "));
            }
            gfx->print(F(timeMinutes[0]));
            gfx->print(F(","));
            gfx->print(F(timeMinutes[1]));
            gfx->print(F(" MIN"));

            gfx->setCursor(10, 248); // 160 + 12 + 64 (sz8) + 12
            // gfx->setCursor(10, 311);// 160 + 72 (sz8) + 8 + 63 (sz7) + 8
            gfx->setTextSize(SMALL_TXT);

            gfx->print(F(car_length));
            gfx->print(F(" CAR TRAIN"));

            delay(5000);
          }

          etd_index++;

          // gfx->print(F(timeMinutes[0]));
          // if(sizeof(timeMinutes, 2)==1){
          // if(timeMinutes[1]){
          //   gfx->print(F(","));
          //   gfx->print(F(timeMinutes[1]));
          // }
          // gfx->print(F(" MIN"));
        }  
      }
    }
  }




  // for(int i = 0; i < numStations; i++)
  // {
  //   gfx->println(F(station_name[i]));
  // }
  delay(5000);



////////////////////////////////////////////////



//   Serial.println(F("Benchmark\tmicro-secs"));

//   int32_t usecFillScreen = testFillScreen();
//   serialOut(F("Screen fill\t"), usecFillScreen, 1000, true);
  
//   int32_t usecText = testText();
//   serialOut(F("Text\t"), usecText, 3000, true);

//   int32_t usecPixels = testPixels();
//   serialOut(F("Pixels\t"), usecPixels, 1000, true);

//   int32_t usecLines = testLines();
//   serialOut(F("Lines\t"), usecLines, 1000, true);

//   int32_t usecFastLines = testFastLines();
//   serialOut(F("Horiz/Vert Lines\t"), usecFastLines, 1000, true);

//   int32_t usecFilledRects = testFilledRects();
//   serialOut(F("Rectangles (filled)\t"), usecFilledRects, 1000, false);

//   int32_t usecRects = testRects();
//   serialOut(F("Rectangles (outline)\t"), usecRects, 1000, true);

//   int32_t usecFilledTrangles = testFilledTriangles();
//   serialOut(F("Triangles (filled)\t"), usecFilledTrangles, 1000, false);

//   int32_t usecTriangles = testTriangles();
//   serialOut(F("Triangles (outline)\t"), usecTriangles, 1000, true);

//   int32_t usecFilledCircles = testFilledCircles(10);
//   serialOut(F("Circles (filled)\t"), usecFilledCircles, 1000, false);

//   int32_t usecCircles = testCircles(10);
//   serialOut(F("Circles (outline)\t"), usecCircles, 1000, true);

//   int32_t usecFilledArcs = testFillArcs();
//   serialOut(F("Arcs (filled)\t"), usecFilledArcs, 1000, false);

//   int32_t usecArcs = testArcs();
//   serialOut(F("Arcs (outline)\t"), usecArcs, 1000, true);

//   int32_t usecFilledRoundRects = testFilledRoundRects();
//   serialOut(F("Rounded rects (filled)\t"), usecFilledRoundRects, 1000, false);

//   int32_t usecRoundRects = testRoundRects();
//   serialOut(F("Rounded rects (outline)\t"), usecRoundRects, 1000, true);

// #ifdef CANVAS
//   uint32_t start = micros_start();
//   gfx->flush();
//   int32_t usecFlush = micros() - start;
//   serialOut(F("flush (Canvas only)\t"), usecFlush, 0, false);
// #endif

//   Serial.println(F("Done!"));

//   uint16_t c = 4;
//   int8_t d = 1;
//   for (int32_t i = 0; i < h; i++)
//   {
//     gfx->drawFastHLine(0, i, w, c);
//     c += d;
//     if (c <= 4 || c >= 11)
//     {
//       d = -d;
//     }
//   }

//   gfx->setCursor(0, 0);

//   gfx->setTextSize(tsa);
//   gfx->setTextColor(MAGENTA);
//   gfx->println(F("Arduino GFX PDQ"));

//   if (h > w)
//   {
//     gfx->setTextSize(tsb);
//     gfx->setTextColor(GREEN);
//     gfx->print(F("\nBenchmark "));
//     gfx->setTextSize(tsc);
//     if (ds == 12)
//     {
//       gfx->print(F("   "));
//     }
//     gfx->println(F("micro-secs"));
//   }

//   printnice(F("Screen fill "), usecFillScreen);
//   printnice(F("Text        "), usecText);
//   printnice(F("Pixels      "), usecPixels);
//   printnice(F("Lines       "), usecLines);
//   printnice(F("H/V Lines   "), usecFastLines);
//   printnice(F("Rectangles F"), usecFilledRects);
//   printnice(F("Rectangles  "), usecRects);
//   printnice(F("Triangles F "), usecFilledTrangles);
//   printnice(F("Triangles   "), usecTriangles);
//   printnice(F("Circles F   "), usecFilledCircles);
//   printnice(F("Circles     "), usecCircles);
//   printnice(F("Arcs F      "), usecFilledArcs);
//   printnice(F("Arcs        "), usecArcs);
//   printnice(F("RoundRects F"), usecFilledRoundRects);
//   printnice(F("RoundRects  "), usecRoundRects);

//   if ((h > w) || (h > 240))
//   {
//     gfx->setTextSize(tsc);
//     gfx->setTextColor(GREEN);
//     gfx->print(F("\nBenchmark Complete!"));
//   }

// #ifdef CANVAS
//   gfx->flush();
//   Serial.println(F("ITS A CANVAS !!!"));
// #endif

}

#ifdef ESP32
void serialOut(const char *item, int32_t v, uint32_t d, bool clear)
#else
void serialOut(const __FlashStringHelper *item, int32_t v, uint32_t d, bool clear)
#endif
{
#ifdef CANVAS
  gfx->flush();
#endif
  Serial.print(item);
  if (v < 0)
  {
    Serial.println(F("N/A"));
  }
  else
  {
    Serial.println(v);
  }
  delay(d);
  if (clear)
  {
    gfx->fillScreen(BLACK);
  }
}

#ifdef ESP32
void printnice(const char *item, long int v)
#else
void printnice(const __FlashStringHelper *item, long int v)
#endif
{
  gfx->setTextSize(tsb);
  gfx->setTextColor(CYAN);
  gfx->print(item);

  gfx->setTextSize(tsc);
  gfx->setTextColor(YELLOW);
  if (v < 0)
  {
    gfx->println(F("      N / A"));
  }
  else
  {
    char str[32] = {0};
#ifdef RTL8722DM
    sprintf(str, "%d", (int)v);
#else
    sprintf(str, "%ld", v);
#endif
    for (char *p = (str + strlen(str)) - 3; p > str; p -= 3)
    {
      memmove(p + 1, p, strlen(p) + 1);
      *p = ',';
    }
    while (strlen(str) < ds)
    {
      memmove(str + 1, str, strlen(str) + 1);
      *str = ' ';
    }
    gfx->println(str);
  }
}

int32_t testFillScreen()
{
  uint32_t start = micros_start();
  // Shortened this tedious test!
  gfx->fillScreen(WHITE);
  gfx->fillScreen(RED);
  gfx->fillScreen(GREEN);
  gfx->fillScreen(BLUE);
  gfx->fillScreen(BLACK);

  return micros() - start;
}

int32_t testText()
{
  uint32_t start = micros_start();
  gfx->setCursor(0, 0);

  gfx->setTextSize(1);
  gfx->setTextColor(WHITE, BLACK);
  gfx->println(F("Hello World!"));

  gfx->setTextSize(2);
  gfx->setTextColor(gfx->color565(0xff, 0x00, 0x00));
  gfx->print(F("RED "));
  gfx->setTextColor(gfx->color565(0x00, 0xff, 0x00));
  gfx->print(F("GREEN "));
  gfx->setTextColor(gfx->color565(0x00, 0x00, 0xff));
  gfx->println(F("BLUE"));

  gfx->setTextSize(tsa);
  gfx->setTextColor(YELLOW);
  gfx->println(1234.56);

  gfx->setTextColor(WHITE);
  gfx->println((w > 128) ? 0xDEADBEEF : 0xDEADBEE, HEX);

  gfx->setTextColor(CYAN, WHITE);
  gfx->println(F("Groop,"));

  gfx->setTextSize(tsc);
  gfx->setTextColor(MAGENTA, WHITE);
  gfx->println(F("I implore thee,"));

  gfx->setTextSize(1);
  gfx->setTextColor(NAVY, WHITE);
  gfx->println(F("my foonting turlingdromes."));

  gfx->setTextColor(DARKGREEN, WHITE);
  gfx->println(F("And hooptiously drangle me"));

  gfx->setTextColor(DARKCYAN, WHITE);
  gfx->println(F("with crinkly bindlewurdles,"));

  gfx->setTextColor(MAROON, WHITE);
  gfx->println(F("Or I will rend thee"));

  gfx->setTextColor(PURPLE, WHITE);
  gfx->println(F("in the gobberwartsb"));

  gfx->setTextColor(OLIVE, WHITE);
  gfx->println(F("with my blurglecruncheon,"));

  gfx->setTextColor(DARKGREY, WHITE);
  gfx->println(F("see if I don't!"));

  gfx->setTextSize(2);
  gfx->setTextColor(RED);
  gfx->println(F("Size 2"));

  gfx->setTextSize(3);
  gfx->setTextColor(ORANGE);
  gfx->println(F("Size 3"));

  gfx->setTextSize(4);
  gfx->setTextColor(YELLOW);
  gfx->println(F("Size 4"));

  gfx->setTextSize(5);
  gfx->setTextColor(GREENYELLOW);
  gfx->println(F("Size 5"));

  gfx->setTextSize(6);
  gfx->setTextColor(GREEN);
  gfx->println(F("Size 6"));

  gfx->setTextSize(7);
  gfx->setTextColor(BLUE);
  gfx->println(F("Size 7"));

  gfx->setTextSize(8);
  gfx->setTextColor(PURPLE);
  gfx->println(F("Size 8"));

  gfx->setTextSize(9);
  gfx->setTextColor(PALERED);
  gfx->println(F("Size 9"));

  return micros() - start;
}

int32_t testPixels()
{
  uint32_t start = micros_start();

  for (int16_t y = 0; y < h; y++)
  {
    for (int16_t x = 0; x < w; x++)
    {
      gfx->drawPixel(x, y, gfx->color565(x << 3, y << 3, x * y));
    }
#ifdef ESP8266
    yield(); // avoid long run triggered ESP8266 WDT restart
#endif
  }

  return micros() - start;
}

int32_t testLines()
{
  uint32_t start;
  int32_t x1, y1, x2, y2;

  start = micros_start();

  x1 = y1 = 0;
  y2 = h - 1;
  for (x2 = 0; x2 < w; x2 += 6)
  {
    gfx->drawLine(x1, y1, x2, y2, BLUE);
  }
#ifdef ESP8266
  yield(); // avoid long run triggered ESP8266 WDT restart
#endif

  x2 = w - 1;
  for (y2 = 0; y2 < h; y2 += 6)
  {
    gfx->drawLine(x1, y1, x2, y2, BLUE);
  }
#ifdef ESP8266
  yield(); // avoid long run triggered ESP8266 WDT restart
#endif

  x1 = w - 1;
  y1 = 0;
  y2 = h - 1;
  for (x2 = 0; x2 < w; x2 += 6)
  {
    gfx->drawLine(x1, y1, x2, y2, BLUE);
  }
#ifdef ESP8266
  yield(); // avoid long run triggered ESP8266 WDT restart
#endif

  x2 = 0;
  for (y2 = 0; y2 < h; y2 += 6)
  {
    gfx->drawLine(x1, y1, x2, y2, BLUE);
  }
#ifdef ESP8266
  yield(); // avoid long run triggered ESP8266 WDT restart
#endif

  x1 = 0;
  y1 = h - 1;
  y2 = 0;
  for (x2 = 0; x2 < w; x2 += 6)
  {
    gfx->drawLine(x1, y1, x2, y2, BLUE);
  }
#ifdef ESP8266
  yield(); // avoid long run triggered ESP8266 WDT restart
#endif

  x2 = w - 1;
  for (y2 = 0; y2 < h; y2 += 6)
  {
    gfx->drawLine(x1, y1, x2, y2, BLUE);
  }
#ifdef ESP8266
  yield(); // avoid long run triggered ESP8266 WDT restart
#endif

  x1 = w - 1;
  y1 = h - 1;
  y2 = 0;
  for (x2 = 0; x2 < w; x2 += 6)
  {
    gfx->drawLine(x1, y1, x2, y2, BLUE);
  }
#ifdef ESP8266
  yield(); // avoid long run triggered ESP8266 WDT restart
#endif

  x2 = 0;
  for (y2 = 0; y2 < h; y2 += 6)
  {
    gfx->drawLine(x1, y1, x2, y2, BLUE);
  }
#ifdef ESP8266
  yield(); // avoid long run triggered ESP8266 WDT restart
#endif

  return micros() - start;
}

int32_t testFastLines()
{
  uint32_t start;
  int32_t x, y;

  start = micros_start();

  for (y = 0; y < h; y += 5)
  {
    gfx->drawFastHLine(0, y, w, RED);
  }
  for (x = 0; x < w; x += 5)
  {
    gfx->drawFastVLine(x, 0, h, BLUE);
  }

  return micros() - start;
}

int32_t testFilledRects()
{
  uint32_t start;
  int32_t i, i2;

  start = micros_start();

  for (i = n; i > 0; i -= 6)
  {
    i2 = i / 2;

    gfx->fillRect(cx - i2, cy - i2, i, i, gfx->color565(i, i, 0));
  }

  return micros() - start;
}

int32_t testRects()
{
  uint32_t start;
  int32_t i, i2;

  start = micros_start();
  for (i = 2; i < n; i += 6)
  {
    i2 = i / 2;
    gfx->drawRect(cx - i2, cy - i2, i, i, GREEN);
  }

  return micros() - start;
}

int32_t testFilledCircles(uint8_t radius)
{
  uint32_t start;
  int32_t x, y, r2 = radius * 2;

  start = micros_start();

  for (x = radius; x < w; x += r2)
  {
    for (y = radius; y < h; y += r2)
    {
      gfx->fillCircle(x, y, radius, MAGENTA);
    }
  }

  return micros() - start;
}

int32_t testCircles(uint8_t radius)
{
  uint32_t start;
  int32_t x, y, r2 = radius * 2;
  int32_t w1 = w + radius;
  int32_t h1 = h + radius;

  // Screen is not cleared for this one -- this is
  // intentional and does not affect the reported time.
  start = micros_start();

  for (x = 0; x < w1; x += r2)
  {
    for (y = 0; y < h1; y += r2)
    {
      gfx->drawCircle(x, y, radius, WHITE);
    }
  }

  return micros() - start;
}

int32_t testFillArcs()
{
  int16_t i, r = 360 / cn;
  uint32_t start = micros_start();

  for (i = 6; i < cn; i += 6)
  {
    gfx->fillArc(cx1, cy1, i, i - 3, 0, i * r, RED);
  }

  return micros() - start;
}

int32_t testArcs()
{
  int16_t i, r = 360 / cn;
  uint32_t start = micros_start();

  for (i = 6; i < cn; i += 6)
  {
    gfx->drawArc(cx1, cy1, i, i - 3, 0, i * r, WHITE);
  }

  return micros() - start;
}

int32_t testFilledTriangles()
{
  uint32_t start;
  int32_t i;

  start = micros_start();

  for (i = cn1; i > 10; i -= 5)
  {
    gfx->fillTriangle(cx1, cy1 - i, cx1 - i, cy1 + i, cx1 + i, cy1 + i,
                      gfx->color565(0, i, i));
  }

  return micros() - start;
}

int32_t testTriangles()
{
  uint32_t start;
  int32_t i;

  start = micros_start();

  for (i = 0; i < cn; i += 5)
  {
    gfx->drawTriangle(
        cx1, cy1 - i,     // peak
        cx1 - i, cy1 + i, // bottom left
        cx1 + i, cy1 + i, // bottom right
        gfx->color565(0, 0, i));
  }

  return micros() - start;
}

int32_t testFilledRoundRects()
{
  uint32_t start;
  int32_t i, i2;

  start = micros_start();

  for (i = n1; i > 20; i -= 6)
  {
    i2 = i / 2;
    gfx->fillRoundRect(cx - i2, cy - i2, i, i, i / 8, gfx->color565(0, i, 0));
  }

  return micros() - start;
}

int32_t testRoundRects()
{
  uint32_t start;
  int32_t i, i2;

  start = micros_start();

  for (i = 20; i < n1; i += 6)
  {
    i2 = i / 2;
    gfx->drawRoundRect(cx - i2, cy - i2, i, i, i / 8, gfx->color565(i, 0, 0));
  }

  return micros() - start;
}

/***************************************************
  Original sketch text:

  This is an example sketch for the Adafruit 2.2" SPI display.
  This library works with the Adafruit 2.2" TFT Breakout w/SD card
  ----> http://www.adafruit.com/products/1480

  Check out the links above for our tutorials and wiring diagrams
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional)
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/




















































///*
//  Adapted from the Adafruit and Xark's PDQ graphicstest sketch.
//
//  See end of file for original header text and MIT license info.
//*/
//
///*******************************************************************************
// * Start of Arduino_GFX setting
// ******************************************************************************/
//#include <Arduino_GFX_Library.h>
//
//
//#define PCA_TFT_RESET 2
//#define PCA_TFT_CS 1
//#define PCA_TFT_SCK 0
//#define PCA_TFT_MOSI 7
//
//#define TFT_DE 2
//#define TFT_VSYNC 42
//#define TFT_HSYNC 41
//#define TFT_PCLK 1
//#define TFT_R1 11
//#define TFT_R2 10
//#define TFT_R3 9
//#define TFT_R4 46
//#define TFT_G0 48
//#define TFT_G1 47
//#define TFT_G2 21
//#define TFT_G3 14
//#define TFT_G4 13
//#define TFT_G5 12
//#define TFT_B1 40
//#define TFT_B2 39
//#define TFT_B3 38
//#define TFT_B4 0
//
//
//Arduino_XCA9554SWSPI *expander = new Arduino_XCA9554SWSPI(
//    PCA_TFT_RESET, PCA_TFT_CS, PCA_TFT_SCK, PCA_TFT_MOSI,
//    &Wire, 0x3F);
//
////  Qualia S3 RGB-666 with HD458002C40 4.58" 320x960 Bar Display
//Arduino_ESP32RGBPanel *rgbpanel_320x960 = new Arduino_ESP32RGBPanel(
//    TFT_DE, TFT_VSYNC, TFT_HSYNC, TFT_PCLK,
//    TFT_R1, TFT_R2, TFT_R3, TFT_R4, TFT_R5,
//    TFT_G0, TFT_G1, TFT_G2, TFT_G3, TFT_G4, TFT_G5,
//    TFT_B1, TFT_B2, TFT_B3, TFT_B4, TFT_B5,
//    1 /* hsync_polarity */, 30 /* hsync_front_porch */, 10 /* hsync_pulse_width */, 50 /* hsync_back_porch */,
//    1 /* vsync_polarity */, 15 /* vsync_front_porch */, 2 /* vsync_pulse_width */, 17 /* vsync_back_porch */,
//    1 /* pclk_active_neg */, GFX_NOT_DEFINED /* prefer_speed */, false /* useBigEndian */, 0 /* de_idle_high */,
//    0 /* pclk_idle_high */
//    );
//
//
//
//const uint8_t init_sequence_hd458[] = {
//  0xFF, 0x05, 0x77, 0x01, 0x00, 0x00, 0x13,
//  0xEF, 0x01, 0x08,
//  0xFF, 0x05, 0x77, 0x01, 0x00, 0x00, 0x10,
//  0xC0, 0x02, 0x77, 0x00,
//  0xC1, 0x02, 0x09, 0x08,
//  0xC2, 0x02, 0x01, 0x02,
//  0xC3, 0x01, 0x02,
//  0xCC, 0x01, 0x10,
//  0xB0, 0x10, 0x40, 0x14, 0x59, 0x10, 0x12, 0x08, 0x03, 0x09, 0x05, 0x1E, 0x05, 0x14, 0x10, 0x68, 0x33, 0x15,
//  0xB1, 0x10, 0x40, 0x08, 0x53, 0x09, 0x11, 0x09, 0x02, 0x07, 0x09, 0x1A, 0x04, 0x12, 0x12, 0x64, 0x29, 0x29,
//  0xFF, 0x05, 0x77, 0x01, 0x00, 0x00, 0x11,
//  0xB0, 0x01, 0x6D,
//  0xB1, 0x01, 0x1D,
//  0xB2, 0x01, 0x87,
//  0xB3, 0x01, 0x80,
//  0xB5, 0x01, 0x49,
//  0xB7, 0x01, 0x85,
//  0xB8, 0x01, 0x20,
//  0xC1, 0x01, 0x78,
//  0xC2, 0x01, 0x78,
//  0xD0, 0x01, 0x88,
//  0xE0, 0x03, 0x00, 0x00, 0x02,
//  0xE1, 0x0B, 0x02, 0x8C, 0x00, 0x00, 0x03, 0x8C, 0x00, 0x00, 0x00, 0x33, 0x33,
//  0xE2, 0x0D, 0x33, 0x33, 0x33, 0x33, 0xC9, 0x3C, 0x00, 0x00, 0xCA, 0x3C, 0x00, 0x00, 0x00,
//  0xE3, 0x04, 0x00, 0x00, 0x33, 0x33,
//  0xE4, 0x02, 0x44, 0x44,
//  0xE5, 0x10, 0x05, 0xCD, 0x82, 0x82, 0x01, 0xC9, 0x82, 0x82, 0x07, 0xCF, 0x82, 0x82, 0x03, 0xCB, 0x82, 0x82,
//  0xE6, 0x04, 0x00, 0x00, 0x33, 0x33,
//  0xE7, 0x02, 0x44, 0x44,
//  0xE8, 0x10, 0x06, 0xCE, 0x82, 0x82, 0x02, 0xCA, 0x82, 0x82, 0x08, 0xD0, 0x82, 0x82, 0x04, 0xCC, 0x82, 0x82,
//  0xEB, 0x07, 0x08, 0x01, 0xE4, 0xE4, 0x88, 0x00, 0x40,
//  0xEC, 0x03, 0x00, 0x00, 0x00,
//  0xED, 0x10, 0xFF, 0xF0, 0x07, 0x65, 0x4F, 0xFC, 0xC2, 0x2F, 0xF2, 0x2C, 0xCF, 0xF4, 0x56, 0x70, 0x0F, 0xFF,
//  0xEF, 0x06, 0x10, 0x0D, 0x04, 0x08, 0x3F, 0x1F,
//  0xFF, 0x05, 0x77, 0x01, 0x00, 0x00, 0x00,
//  0x11, 0x80, 0x78,
//  0x35, 0x01, 0x00,
//  0x3A, 0x81, 0x66, 0x64,
//  0x29, 0x00
//};
//
//
//
//
//
//
//Arduino_RGB_Display *gfx = new Arduino_RGB_Display
//(
//// 4.58" 320x960 rectangle bar display, adapted from 320x820 display
//    320 /* width */, 960 /* height */, rgbpanel_320x960, 0 /* rotation */, true /* auto_flush */,
//    expander, GFX_NOT_DEFINED /* RST */, init_sequence_hd458, sizeof(init_sequence_hd458),
//    80 /* col_offset1 */, 0 /* row_offset1 */, 8 /* col_offset2 */, 0 /* row_offset2 */
//);
///*******************************************************************************
// * End of Arduino_GFX setting
// ******************************************************************************/
//
//#ifdef ESP32
//#undef F
//#define F(s) (s)
//#endif
//
//int32_t w, h, n, n1, cx, cy, cx1, cy1, cn, cn1;
//uint8_t tsa, tsb, tsc, ds;
//
//void setup()
//{
//  Serial.begin(115200);
//  // Serial.setDebugOutput(true);
//  while (!Serial) delay(100);
//  Serial.println("Arduino_GFX PDQgraphicstest example!");
//
//#ifdef GFX_EXTRA_PRE_INIT
//  GFX_EXTRA_PRE_INIT();
//#endif
//
//  // Init Display
//  if (!gfx->begin())
//  // if (!gfx->begin(80000000)) /* specify data bus speed */
//  {
//    Serial.println("gfx->begin() failed!");
//  }
//
//  w = gfx->width();
//  h = gfx->height();
//  n = min(w, h);
//  n1 = n - 1;
//  cx = w / 2;
//  cy = h / 2;
//  cx1 = cx - 1;
//  cy1 = cy - 1;
//  cn = min(cx1, cy1);
//  cn1 = cn - 1;
//  tsa = ((w <= 176) || (h <= 160)) ? 1 : (((w <= 240) || (h <= 240)) ? 2 : 3); // text size A
//  tsb = ((w <= 272) || (h <= 220)) ? 1 : 2;                                    // text size B
//  tsc = ((w <= 220) || (h <= 220)) ? 1 : 2;                                    // text size C
//  ds = (w <= 160) ? 9 : 12;                                                    // digit size
//
//  Serial.println(w);
//  Serial.println(h);
//  Serial.println(cx);
//  Serial.println(cy);
//
//#ifdef GFX_BL
//  pinMode(GFX_BL, OUTPUT);
//  digitalWrite(GFX_BL, HIGH);
//#endif
//}
//
//static inline uint32_t micros_start() __attribute__((always_inline));
//static inline uint32_t micros_start()
//{
//  uint8_t oms = millis();
//  while ((uint8_t)millis() == oms)
//    ;
//  return micros();
//}
//
//void loop(void)
//{
//  Serial.println(F("Benchmark\tmicro-secs"));
//
//  int32_t usecFillScreen = testFillScreen();
//  serialOut(F("Screen fill\t"), usecFillScreen, 1000, true);
//
//  int32_t usecText = testText();
//  serialOut(F("Text\t"), usecText, 3000, true);
//
//  int32_t usecPixels = testPixels();
//  serialOut(F("Pixels\t"), usecPixels, 1000, true);
//
//  int32_t usecLines = testLines();
//  serialOut(F("Lines\t"), usecLines, 1000, true);
//
//  int32_t usecFastLines = testFastLines();
//  serialOut(F("Horiz/Vert Lines\t"), usecFastLines, 1000, true);
//
//  int32_t usecFilledRects = testFilledRects();
//  serialOut(F("Rectangles (filled)\t"), usecFilledRects, 1000, false);
//
//  int32_t usecRects = testRects();
//  serialOut(F("Rectangles (outline)\t"), usecRects, 1000, true);
//
//  int32_t usecFilledTrangles = testFilledTriangles();
//  serialOut(F("Triangles (filled)\t"), usecFilledTrangles, 1000, false);
//
//  int32_t usecTriangles = testTriangles();
//  serialOut(F("Triangles (outline)\t"), usecTriangles, 1000, true);
//
//  int32_t usecFilledCircles = testFilledCircles(10);
//  serialOut(F("Circles (filled)\t"), usecFilledCircles, 1000, false);
//
//  int32_t usecCircles = testCircles(10);
//  serialOut(F("Circles (outline)\t"), usecCircles, 1000, true);
//
//  int32_t usecFilledArcs = testFillArcs();
//  serialOut(F("Arcs (filled)\t"), usecFilledArcs, 1000, false);
//
//  int32_t usecArcs = testArcs();
//  serialOut(F("Arcs (outline)\t"), usecArcs, 1000, true);
//
//  int32_t usecFilledRoundRects = testFilledRoundRects();
//  serialOut(F("Rounded rects (filled)\t"), usecFilledRoundRects, 1000, false);
//
//  int32_t usecRoundRects = testRoundRects();
//  serialOut(F("Rounded rects (outline)\t"), usecRoundRects, 1000, true);
//
//#ifdef CANVAS
//  uint32_t start = micros_start();
//  gfx->flush();
//  int32_t usecFlush = micros() - start;
//  serialOut(F("flush (Canvas only)\t"), usecFlush, 0, false);
//#endif
//
//  Serial.println(F("Done!"));
//
//  uint16_t c = 4;
//  int8_t d = 1;
//  for (int32_t i = 0; i < h; i++)
//  {
//    gfx->drawFastHLine(0, i, w, c);
//    c += d;
//    if (c <= 4 || c >= 11)
//    {
//      d = -d;
//    }
//  }
//
//  gfx->setCursor(0, 0);
//
//  gfx->setTextSize(tsa);
//  gfx->setTextColor(MAGENTA);
//  gfx->println(F("Arduino GFX PDQ"));
//
//  if (h > w)
//  {
//    gfx->setTextSize(tsb);
//    gfx->setTextColor(GREEN);
//    gfx->print(F("\nBenchmark "));
//    gfx->setTextSize(tsc);
//    if (ds == 12)
//    {
//      gfx->print(F("   "));
//    }
//    gfx->println(F("micro-secs"));
//  }
//
//  printnice(F("Screen fill "), usecFillScreen);
//  printnice(F("Text        "), usecText);
//  printnice(F("Pixels      "), usecPixels);
//  printnice(F("Lines       "), usecLines);
//  printnice(F("H/V Lines   "), usecFastLines);
//  printnice(F("Rectangles F"), usecFilledRects);
//  printnice(F("Rectangles  "), usecRects);
//  printnice(F("Triangles F "), usecFilledTrangles);
//  printnice(F("Triangles   "), usecTriangles);
//  printnice(F("Circles F   "), usecFilledCircles);
//  printnice(F("Circles     "), usecCircles);
//  printnice(F("Arcs F      "), usecFilledArcs);
//  printnice(F("Arcs        "), usecArcs);
//  printnice(F("RoundRects F"), usecFilledRoundRects);
//  printnice(F("RoundRects  "), usecRoundRects);
//
//  if ((h > w) || (h > 240))
//  {
//    gfx->setTextSize(tsc);
//    gfx->setTextColor(GREEN);
//    gfx->print(F("\nBenchmark Complete!"));
//  }
//
//#ifdef CANVAS
//  gfx->flush();
//#endif
//
//  delay(60 * 1000L);
//}
//
//#ifdef ESP32
//void serialOut(const char *item, int32_t v, uint32_t d, bool clear)
//#else
//void serialOut(const __FlashStringHelper *item, int32_t v, uint32_t d, bool clear)
//#endif
//{
//#ifdef CANVAS
//  gfx->flush();
//#endif
//  Serial.print(item);
//  if (v < 0)
//  {
//    Serial.println(F("N/A"));
//  }
//  else
//  {
//    Serial.println(v);
//  }
//  delay(d);
//  if (clear)
//  {
//    gfx->fillScreen(BLACK);
//  }
//}
//
//#ifdef ESP32
//void printnice(const char *item, long int v)
//#else
//void printnice(const __FlashStringHelper *item, long int v)
//#endif
//{
//  gfx->setTextSize(tsb);
//  gfx->setTextColor(CYAN);
//  gfx->print(item);
//
//  gfx->setTextSize(tsc);
//  gfx->setTextColor(YELLOW);
//  if (v < 0)
//  {
//    gfx->println(F("      N / A"));
//  }
//  else
//  {
//    char str[32] = {0};
//#ifdef RTL8722DM
//    sprintf(str, "%d", (int)v);
//#else
//    sprintf(str, "%ld", v);
//#endif
//    for (char *p = (str + strlen(str)) - 3; p > str; p -= 3)
//    {
//      memmove(p + 1, p, strlen(p) + 1);
//      *p = ',';
//    }
//    while (strlen(str) < ds)
//    {
//      memmove(str + 1, str, strlen(str) + 1);
//      *str = ' ';
//    }
//    gfx->println(str);
//  }
//}
//
//int32_t testFillScreen()
//{
//  uint32_t start = micros_start();
//  // Shortened this tedious test!
//  gfx->fillScreen(WHITE);
//  gfx->fillScreen(RED);
//  gfx->fillScreen(GREEN);
//  gfx->fillScreen(BLUE);
//  gfx->fillScreen(BLACK);
//
//  return micros() - start;
//}
//
//int32_t testText()
//{
//  uint32_t start = micros_start();
//  gfx->setCursor(0, 0);
//
//  gfx->setTextSize(1);
//  gfx->setTextColor(WHITE, BLACK);
//  gfx->println(F("Hello World!"));
//
//  gfx->setTextSize(2);
//  gfx->setTextColor(gfx->color565(0xff, 0x00, 0x00));
//  gfx->print(F("RED "));
//  gfx->setTextColor(gfx->color565(0x00, 0xff, 0x00));
//  gfx->print(F("GREEN "));
//  gfx->setTextColor(gfx->color565(0x00, 0x00, 0xff));
//  gfx->println(F("BLUE"));
//
//  gfx->setTextSize(tsa);
//  gfx->setTextColor(YELLOW);
//  gfx->println(1234.56);
//
//  gfx->setTextColor(WHITE);
//  gfx->println((w > 128) ? 0xDEADBEEF : 0xDEADBEE, HEX);
//
//  gfx->setTextColor(CYAN, WHITE);
//  gfx->println(F("Groop,"));
//
//  gfx->setTextSize(tsc);
//  gfx->setTextColor(MAGENTA, WHITE);
//  gfx->println(F("I implore thee,"));
//
//  gfx->setTextSize(1);
//  gfx->setTextColor(NAVY, WHITE);
//  gfx->println(F("my foonting turlingdromes."));
//
//  gfx->setTextColor(DARKGREEN, WHITE);
//  gfx->println(F("And hooptiously drangle me"));
//
//  gfx->setTextColor(DARKCYAN, WHITE);
//  gfx->println(F("with crinkly bindlewurdles,"));
//
//  gfx->setTextColor(MAROON, WHITE);
//  gfx->println(F("Or I will rend thee"));
//
//  gfx->setTextColor(PURPLE, WHITE);
//  gfx->println(F("in the gobberwartsb"));
//
//  gfx->setTextColor(OLIVE, WHITE);
//  gfx->println(F("with my blurglecruncheon,"));
//
//  gfx->setTextColor(DARKGREY, WHITE);
//  gfx->println(F("see if I don't!"));
//
//  gfx->setTextSize(2);
//  gfx->setTextColor(RED);
//  gfx->println(F("Size 2"));
//
//  gfx->setTextSize(3);
//  gfx->setTextColor(ORANGE);
//  gfx->println(F("Size 3"));
//
//  gfx->setTextSize(4);
//  gfx->setTextColor(YELLOW);
//  gfx->println(F("Size 4"));
//
//  gfx->setTextSize(5);
//  gfx->setTextColor(GREENYELLOW);
//  gfx->println(F("Size 5"));
//
//  gfx->setTextSize(6);
//  gfx->setTextColor(GREEN);
//  gfx->println(F("Size 6"));
//
//  gfx->setTextSize(7);
//  gfx->setTextColor(BLUE);
//  gfx->println(F("Size 7"));
//
//  gfx->setTextSize(8);
//  gfx->setTextColor(PURPLE);
//  gfx->println(F("Size 8"));
//
//  gfx->setTextSize(9);
//  gfx->setTextColor(RED);
//  gfx->println(F("Size 9"));
//
//  return micros() - start;
//}
//
//int32_t testPixels()
//{
//  uint32_t start = micros_start();
//
//  for (int16_t y = 0; y < h; y++)
//  {
//    for (int16_t x = 0; x < w; x++)
//    {
//      gfx->drawPixel(x, y, gfx->color565(x << 3, y << 3, x * y));
//    }
//#ifdef ESP8266
//    yield(); // avoid long run triggered ESP8266 WDT restart
//#endif
//  }
//
//  return micros() - start;
//}
//
//int32_t testLines()
//{
//  uint32_t start;
//  int32_t x1, y1, x2, y2;
//
//  start = micros_start();
//
//  x1 = y1 = 0;
//  y2 = h - 1;
//  for (x2 = 0; x2 < w; x2 += 6)
//  {
//    gfx->drawLine(x1, y1, x2, y2, BLUE);
//  }
//#ifdef ESP8266
//  yield(); // avoid long run triggered ESP8266 WDT restart
//#endif
//
//  x2 = w - 1;
//  for (y2 = 0; y2 < h; y2 += 6)
//  {
//    gfx->drawLine(x1, y1, x2, y2, BLUE);
//  }
//#ifdef ESP8266
//  yield(); // avoid long run triggered ESP8266 WDT restart
//#endif
//
//  x1 = w - 1;
//  y1 = 0;
//  y2 = h - 1;
//  for (x2 = 0; x2 < w; x2 += 6)
//  {
//    gfx->drawLine(x1, y1, x2, y2, BLUE);
//  }
//#ifdef ESP8266
//  yield(); // avoid long run triggered ESP8266 WDT restart
//#endif
//
//  x2 = 0;
//  for (y2 = 0; y2 < h; y2 += 6)
//  {
//    gfx->drawLine(x1, y1, x2, y2, BLUE);
//  }
//#ifdef ESP8266
//  yield(); // avoid long run triggered ESP8266 WDT restart
//#endif
//
//  x1 = 0;
//  y1 = h - 1;
//  y2 = 0;
//  for (x2 = 0; x2 < w; x2 += 6)
//  {
//    gfx->drawLine(x1, y1, x2, y2, BLUE);
//  }
//#ifdef ESP8266
//  yield(); // avoid long run triggered ESP8266 WDT restart
//#endif
//
//  x2 = w - 1;
//  for (y2 = 0; y2 < h; y2 += 6)
//  {
//    gfx->drawLine(x1, y1, x2, y2, BLUE);
//  }
//#ifdef ESP8266
//  yield(); // avoid long run triggered ESP8266 WDT restart
//#endif
//
//  x1 = w - 1;
//  y1 = h - 1;
//  y2 = 0;
//  for (x2 = 0; x2 < w; x2 += 6)
//  {
//    gfx->drawLine(x1, y1, x2, y2, BLUE);
//  }
//#ifdef ESP8266
//  yield(); // avoid long run triggered ESP8266 WDT restart
//#endif
//
//  x2 = 0;
//  for (y2 = 0; y2 < h; y2 += 6)
//  {
//    gfx->drawLine(x1, y1, x2, y2, BLUE);
//  }
//#ifdef ESP8266
//  yield(); // avoid long run triggered ESP8266 WDT restart
//#endif
//
//  return micros() - start;
//}
//
//int32_t testFastLines()
//{
//  uint32_t start;
//  int32_t x, y;
//
//  start = micros_start();
//
//  for (y = 0; y < h; y += 5)
//  {
//    gfx->drawFastHLine(0, y, w, RED);
//  }
//  for (x = 0; x < w; x += 5)
//  {
//    gfx->drawFastVLine(x, 0, h, BLUE);
//  }
//
//  return micros() - start;
//}
//
//int32_t testFilledRects()
//{
//  uint32_t start;
//  int32_t i, i2;
//
//  start = micros_start();
//
//  for (i = n; i > 0; i -= 6)
//  {
//    i2 = i / 2;
//
//    gfx->fillRect(cx - i2, cy - i2, i, i, gfx->color565(i, i, 0));
//  }
//
//  return micros() - start;
//}
//
//int32_t testRects()
//{
//  uint32_t start;
//  int32_t i, i2;
//
//  start = micros_start();
//  for (i = 2; i < n; i += 6)
//  {
//    i2 = i / 2;
//    gfx->drawRect(cx - i2, cy - i2, i, i, GREEN);
//  }
//
//  return micros() - start;
//}
//
//int32_t testFilledCircles(uint8_t radius)
//{
//  uint32_t start;
//  int32_t x, y, r2 = radius * 2;
//
//  start = micros_start();
//
//  for (x = radius; x < w; x += r2)
//  {
//    for (y = radius; y < h; y += r2)
//    {
//      gfx->fillCircle(x, y, radius, MAGENTA);
//    }
//  }
//
//  return micros() - start;
//}
//
//int32_t testCircles(uint8_t radius)
//{
//  uint32_t start;
//  int32_t x, y, r2 = radius * 2;
//  int32_t w1 = w + radius;
//  int32_t h1 = h + radius;
//
//  // Screen is not cleared for this one -- this is
//  // intentional and does not affect the reported time.
//  start = micros_start();
//
//  for (x = 0; x < w1; x += r2)
//  {
//    for (y = 0; y < h1; y += r2)
//    {
//      gfx->drawCircle(x, y, radius, WHITE);
//    }
//  }
//
//  return micros() - start;
//}
//
//int32_t testFillArcs()
//{
//  int16_t i, r = 360 / cn;
//  uint32_t start = micros_start();
//
//  for (i = 6; i < cn; i += 6)
//  {
//    gfx->fillArc(cx1, cy1, i, i - 3, 0, i * r, RED);
//  }
//
//  return micros() - start;
//}
//
//int32_t testArcs()
//{
//  int16_t i, r = 360 / cn;
//  uint32_t start = micros_start();
//
//  for (i = 6; i < cn; i += 6)
//  {
//    gfx->drawArc(cx1, cy1, i, i - 3, 0, i * r, WHITE);
//  }
//
//  return micros() - start;
//}
//
//int32_t testFilledTriangles()
//{
//  uint32_t start;
//  int32_t i;
//
//  start = micros_start();
//
//  for (i = cn1; i > 10; i -= 5)
//  {
//    gfx->fillTriangle(cx1, cy1 - i, cx1 - i, cy1 + i, cx1 + i, cy1 + i,
//                      gfx->color565(0, i, i));
//  }
//
//  return micros() - start;
//}
//
//int32_t testTriangles()
//{
//  uint32_t start;
//  int32_t i;
//
//  start = micros_start();
//
//  for (i = 0; i < cn; i += 5)
//  {
//    gfx->drawTriangle(
//        cx1, cy1 - i,     // peak
//        cx1 - i, cy1 + i, // bottom left
//        cx1 + i, cy1 + i, // bottom right
//        gfx->color565(0, 0, i));
//  }
//
//  return micros() - start;
//}
//
//int32_t testFilledRoundRects()
//{
//  uint32_t start;
//  int32_t i, i2;
//
//  start = micros_start();
//
//  for (i = n1; i > 20; i -= 6)
//  {
//    i2 = i / 2;
//    gfx->fillRoundRect(cx - i2, cy - i2, i, i, i / 8, gfx->color565(0, i, 0));
//  }
//
//  return micros() - start;
//}
//
//int32_t testRoundRects()
//{
//  uint32_t start;
//  int32_t i, i2;
//
//  start = micros_start();
//
//  for (i = 20; i < n1; i += 6)
//  {
//    i2 = i / 2;
//    gfx->drawRoundRect(cx - i2, cy - i2, i, i, i / 8, gfx->color565(i, 0, 0));
//  }
//
//  return micros() - start;
//}

/***************************************************
  Original sketch text:

  This is an example sketch for the Adafruit 2.2" SPI display.
  This library works with the Adafruit 2.2" TFT Breakout w/SD card
  ----> http://www.adafruit.com/products/1480

  Check out the links above for our tutorials and wiring diagrams
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional)
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/
































//
//// SPDX-FileCopyrightText: 2023 Limor Fried for Adafruit Industries
////
//// SPDX-License-Identifier: MIT
//
//#include <Arduino_GFX_Library.h>
//#include <Adafruit_FT6206.h>
//#include <Adafruit_CST8XX.h>
//
//Arduino_XCA9554SWSPI *expander = new Arduino_XCA9554SWSPI(
//    PCA_TFT_RESET, PCA_TFT_CS, PCA_TFT_SCK, PCA_TFT_MOSI,
//    &Wire, 0x3F);
//
//#define PCA_TFT_RESET 2
//#define PCA_TFT_CS 1
//#define PCA_TFT_SCK 0
//#define PCA_TFT_MOSI 7
//
//#define TFT_DE 2
//#define TFT_VSYNC 42
//#define TFT_HSYNC 41
//#define TFT_PCLK 1
//#define TFT_R1 11
//#define TFT_R2 10
//#define TFT_R3 9
//#define TFT_R4 46
//#define TFT_G0 48
//#define TFT_G1 47
//#define TFT_G2 21
//#define TFT_G3 14
//#define TFT_G4 13
//#define TFT_G5 12
//#define TFT_B1 40
//#define TFT_B2 39
//#define TFT_B3 38
//#define TFT_B4 0
//
//    
//Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
//    TFT_DE, TFT_VSYNC, TFT_HSYNC, TFT_PCLK,
//    TFT_R1, TFT_R2, TFT_R3, TFT_R4, TFT_R5,
//    TFT_G0, TFT_G1, TFT_G2, TFT_G3, TFT_G4, TFT_G5,
//    TFT_B1, TFT_B2, TFT_B3, TFT_B4, TFT_B5,
//    0 /* hsync_polarity */, 30 /* hsync_front_porch */, 10 /* hsync_pulse_width */, 50 /* hsync_back_porch */,
//    0 /* vsync_polarity */, 15 /* vsync_front_porch */, 2 /* vsync_pulse_width */, 17 /* vsync_back_porch */
////    ,1, 16000000
//    );
//
//
//const uint8_t init_sequence_hd458[] = {
//  0xFF, 0x05, 0x77, 0x01, 0x00, 0x00, 0x13,
//  0xEF, 0x01, 0x08,
//  0xFF, 0x05, 0x77, 0x01, 0x00, 0x00, 0x10,
//  0xC0, 0x02, 0x77, 0x00,
//  0xC1, 0x02, 0x09, 0x08,
//  0xC2, 0x02, 0x01, 0x02,
//  0xC3, 0x01, 0x02,
//  0xCC, 0x01, 0x10,
//  0xB0, 0x10, 0x40, 0x14, 0x59, 0x10, 0x12, 0x08, 0x03, 0x09, 0x05, 0x1E, 0x05, 0x14, 0x10, 0x68, 0x33, 0x15,
//  0xB1, 0x10, 0x40, 0x08, 0x53, 0x09, 0x11, 0x09, 0x02, 0x07, 0x09, 0x1A, 0x04, 0x12, 0x12, 0x64, 0x29, 0x29,
//  0xFF, 0x05, 0x77, 0x01, 0x00, 0x00, 0x11,
//  0xB0, 0x01, 0x6D,
//  0xB1, 0x01, 0x1D,
//  0xB2, 0x01, 0x87,
//  0xB3, 0x01, 0x80,
//  0xB5, 0x01, 0x49,
//  0xB7, 0x01, 0x85,
//  0xB8, 0x01, 0x20,
//  0xC1, 0x01, 0x78,
//  0xC2, 0x01, 0x78,
//  0xD0, 0x01, 0x88,
//  0xE0, 0x03, 0x00, 0x00, 0x02,
//  0xE1, 0x0B, 0x02, 0x8C, 0x00, 0x00, 0x03, 0x8C, 0x00, 0x00, 0x00, 0x33, 0x33,
//  0xE2, 0x0D, 0x33, 0x33, 0x33, 0x33, 0xC9, 0x3C, 0x00, 0x00, 0xCA, 0x3C, 0x00, 0x00, 0x00,
//  0xE3, 0x04, 0x00, 0x00, 0x33, 0x33,
//  0xE4, 0x02, 0x44, 0x44,
//  0xE5, 0x10, 0x05, 0xCD, 0x82, 0x82, 0x01, 0xC9, 0x82, 0x82, 0x07, 0xCF, 0x82, 0x82, 0x03, 0xCB, 0x82, 0x82,
//  0xE6, 0x04, 0x00, 0x00, 0x33, 0x33,
//  0xE7, 0x02, 0x44, 0x44,
//  0xE8, 0x10, 0x06, 0xCE, 0x82, 0x82, 0x02, 0xCA, 0x82, 0x82, 0x08, 0xD0, 0x82, 0x82, 0x04, 0xCC, 0x82, 0x82,
//  0xEB, 0x07, 0x08, 0x01, 0xE4, 0xE4, 0x88, 0x00, 0x40,
//  0xEC, 0x03, 0x00, 0x00, 0x00,
//  0xED, 0x10, 0xFF, 0xF0, 0x07, 0x65, 0x4F, 0xFC, 0xC2, 0x2F, 0xF2, 0x2C, 0xCF, 0xF4, 0x56, 0x70, 0x0F, 0xFF,
//  0xEF, 0x06, 0x10, 0x0D, 0x04, 0x08, 0x3F, 0x1F,
//  0xFF, 0x05, 0x77, 0x01, 0x00, 0x00, 0x00,
//  0x11, 0x80, 0x78,
//  0x35, 0x01, 0x00,
//  0x3A, 0x81, 0x66, 0x64,
//  0x29, 0x00
//};
//
//
//
//Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
//
//// 4.58" 320x960 rectangle bar display
//    320  /* width */, 960 /* height */, rgbpanel, 0 /* rotation */, true /* auto_flush */,
//    expander, GFX_NOT_DEFINED, init_sequence_hd458, sizeof(init_sequence_hd458));
//  
//// 2.1" 480x480 round display
////    480 /* width */, 480 /* height */, rgbpanel, 0 /* rotation */, true /* auto_flush */,
////    expander, GFX_NOT_DEFINED /* RST */, TL021WVC02_init_operations, sizeof(TL021WVC02_init_operations));
//
//// 2.8" 480x480 round display
////    480 /* width */, 480 /* height */, rgbpanel, 0 /* rotation */, true /* auto_flush */,
////    expander, GFX_NOT_DEFINED /* RST */, TL028WVC01_init_operations, sizeof(TL028WVC01_init_operations));
//
//// 3.4" 480x480 square display
////    480 /* width */, 480 /* height */, rgbpanel, 0 /* rotation */, true /* auto_flush */,
////    expander, GFX_NOT_DEFINED /* RST */, tl034wvs05_b1477a_init_operations, sizeof(tl034wvs05_b1477a_init_operations));
//
//// 3.2" 320x820 rectangle bar display
////    320 /* width */, 820 /* height */, rgbpanel, 0 /* rotation */, true /* auto_flush */,
////    expander, GFX_NOT_DEFINED /* RST */, tl032fwv01_init_operations, sizeof(tl032fwv01_init_operations));
//
//// 3.7" 240x960 rectangle bar display
////    240 /* width */, 960 /* height */, rgbpanel, 0 /* rotation */, true /* auto_flush */,
////    expander, GFX_NOT_DEFINED /* RST */, HD371001C40_init_operations, sizeof(HD371001C40_init_operations), 120 /* col_offset1 */);
//
//// 4.0" 720x720 square display
////    720 /* width */, 720 /* height */, rgbpanel, 0 /* rotation */, true /* auto_flush */,
////    expander, GFX_NOT_DEFINED /* RST */, NULL, 0);
//
//// 4.0" 720x720 round display
////    720 /* width */, 720 /* height */, rgbpanel, 0 /* rotation */, true /* auto_flush */,
////    expander, GFX_NOT_DEFINED /* RST */, hd40015c40_init_operations, sizeof(hd40015c40_init_operations));
//// needs also the rgbpanel to have these pulse/sync values:
////    1 /* hync_polarity */, 46 /* hsync_front_porch */, 2 /* hsync_pulse_width */, 44 /* hsync_back_porch */,
////    1 /* vsync_polarity */, 50 /* vsync_front_porch */, 16 /* vsync_pulse_width */, 16 /* vsync_back_porch */
//
//uint16_t *colorWheel;
//
//// The Capacitive touchscreen overlays uses hardware I2C (SCL/SDA)
//
//// Most touchscreens use FocalTouch with I2C Address often but not always 0x48!
//#define I2C_TOUCH_ADDR 0x48
//
//// 2.1" 480x480 round display use CST826 touchscreen with I2C Address at 0x15
////#define I2C_TOUCH_ADDR 0x15  // often but not always 0x48!
//
//Adafruit_FT6206 focal_ctp = Adafruit_FT6206();  // this library also supports FT5336U!
//Adafruit_CST8XX cst_ctp = Adafruit_CST8XX();
//bool touchOK = false;        // we will check if the touchscreen exists
//bool isFocalTouch = false;
//
//void setup(void)
//{  
//  Serial.begin(115200);
//  while (!Serial) delay(100);
//  
////#ifdef GFX_EXTRA_PRE_INIT
////  GFX_EXTRA_PRE_INIT();
////#endif
//
//  Serial.println("Beginning");
//  // Init Display
//
//  Wire.setClock(1000000); // speed up I2C 
//  if (!gfx->begin()) {
//    Serial.println("gfx->begin() failed!");
//  }
//
//  Serial.println("Initialized!");
//
//  gfx->fillScreen(0xAA);
//  gfx->fillCircle(5, 10, 5, 0xAA);
//
//   Serial.println("Initialized!");
//
//  expander->pinMode(PCA_TFT_BACKLIGHT, OUTPUT);
//  expander->digitalWrite(PCA_TFT_BACKLIGHT, HIGH);
//
//  colorWheel = (uint16_t *) ps_malloc(gfx->width() * gfx->height() * sizeof(uint16_t));
//  if (colorWheel) {
//    generateColorWheel(colorWheel);
//    gfx->draw16bitRGBBitmap(0, 0, colorWheel, gfx->width(), gfx->height());
//  }
//
//  if (!focal_ctp.begin(0, &Wire, I2C_TOUCH_ADDR)) {
//    // Try the CST826 Touch Screen
//    if (!cst_ctp.begin(&Wire, I2C_TOUCH_ADDR)) {
//      Serial.print("No Touchscreen found at address 0x");
//      Serial.println(I2C_TOUCH_ADDR, HEX);
//      touchOK = false;
//    } else {
//      Serial.println("CST826 Touchscreen found");
//      touchOK = true;
//      isFocalTouch = false;
//    }
//  } else {
//    Serial.println("Focal Touchscreen found");
//    touchOK = true;
//    isFocalTouch = true;
//  }
//}
//
//void loop()
//{
//  if (touchOK) {
//    if (isFocalTouch && focal_ctp.touched()) {
//      TS_Point p = focal_ctp.getPoint(0);
//      Serial.printf("(%d, %d)\n", p.x, p.y);
//      gfx->fillRect(p.x, p.y, 5, 5, WHITE);
//    } else if (!isFocalTouch && cst_ctp.touched()) {
//      CST_TS_Point p = cst_ctp.getPoint(0);
//      Serial.printf("(%d, %d)\n", p.x, p.y);
//      gfx->fillRect(p.x, p.y, 5, 5, WHITE);
//    }
//  }
//   
//  // use the buttons to turn off
//  if (! expander->digitalRead(PCA_BUTTON_DOWN)) {
//    expander->digitalWrite(PCA_TFT_BACKLIGHT, LOW);
//  }
//  // and on the backlight
//  if (! expander->digitalRead(PCA_BUTTON_UP)) {
//    expander->digitalWrite(PCA_TFT_BACKLIGHT, HIGH);
//  }
//}
//
//// https://chat.openai.com/share/8edee522-7875-444f-9fea-ae93a8dfa4ec
//void generateColorWheel(uint16_t *colorWheel) {
//  int width = gfx->width();
//  int height = gfx->height();
//  int half_width = width / 2;
//  int half_height = height / 2;
//  float angle;
//  uint8_t r, g, b;
//  int index, scaled_index;
//
//  for(int y = 0; y < half_height; y++) {
//    for(int x = 0; x < half_width; x++) {
//      index = y * half_width + x;
//      angle = atan2(y - half_height / 2, x - half_width / 2);
//      r = uint8_t(127.5 * (cos(angle) + 1));
//      g = uint8_t(127.5 * (sin(angle) + 1));
//      b = uint8_t(255 - (r + g) / 2);
//      uint16_t color = RGB565(r, g, b);
//
//      // Scale this pixel into 4 pixels in the full buffer
//      for(int dy = 0; dy < 2; dy++) {
//        for(int dx = 0; dx < 2; dx++) {
//          scaled_index = (y * 2 + dy) * width + (x * 2 + dx);
//          colorWheel[scaled_index] = color;
//        }
//      }
//    }
//  }
//}
