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
#include "Qualia_ESP32_RGB_Display.h"
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
const char* ssid     = "CoralCove88_Mesh"; // Change this to your WiFi SSID
const char* password = "CoralCove88"; // Change this to your WiFi password
// const char* ssid     = "James Stevick's iPhone"; // Change this to your WiFi SSID
// const char* password = "stevick5"; // Change this to

const char* serverName = BART_API_STATIONS_URL;

String station_name[STAT_ARR_SIZE];
String station_abbr[STAT_ARR_SIZE];
int numStations = 0;
String selected_station = "";
String selected_station_name = ""; 
// char payload[10289];

// Button timing variables
int buttonDebounceDelay = BUTTON_DEBOUNCE_DELAY;
int medPressTime = MED_PRESS_TIME;
int longPressTime = LONG_PRESS_TIME;



uint8_t one_station = 10;
int buttonID;
int buttonPress;

#ifdef ESP32
#undef F
#define F(s) (s)
#endif

int32_t w, h, n, n1, cx, cy, cx1, cy1, cn, cn1;
uint8_t tsa, tsb, tsc, ds;






// Calculate X position to center text horizontally
// Returns X coordinate for setCursor to center the text
// Text size must be set before calling this function
int16_t getCenteredX(const String& text, uint8_t textSize) {
  // Get actual display width (in case rotation changed it)
  int16_t actualWidth = gfx->width();
  
  // Calculate text width: each character is CHAR_WIDTH * textSize pixels wide
  // For monospace fonts, this is accurate
  int16_t textWidth = text.length() * CHAR_WIDTH * textSize;
  
  // Calculate centered X position
  int16_t x = (actualWidth - textWidth) / 2;
  
  // Ensure we stay within margins
  if(x < DISPLAY_MARGIN) {
    x = DISPLAY_MARGIN;
  }
  // Check if text would overflow on the right
  int16_t maxX = actualWidth - textWidth - DISPLAY_MARGIN;
  if(x > maxX && maxX >= DISPLAY_MARGIN) {
    x = maxX;
  }
  
  return x;
}

// Check for button press for certain amount of time in milliseconds
// Returns: 0 = no press, 1 = UP short press, 2 = UP medium press (1.5s), 
//          -1 = DOWN short press, -2 = DOWN medium press (1.5s)
// Note: Long press (10s) reserved for future features
int buttonPressed(unsigned long timeMillis){
  // Wait for both buttons to be released first
  while (!expander->digitalRead(UP_BUTTON) || !expander->digitalRead(DN_BUTTON)){
    delay(5);
  }
  
  unsigned long t0 = millis();
  
  while((millis() - t0) <= timeMillis){
    bool isUpButton = !expander->digitalRead(UP_BUTTON);
    bool isDownButton = !expander->digitalRead(DN_BUTTON);
    
    // Check UP button first
    if(isUpButton){
      delay(buttonDebounceDelay);
      isUpButton = !expander->digitalRead(UP_BUTTON);
      if(isUpButton){
        // Check for medium press - wait and see if button stays pressed
        // Use MED_PRESS_TIME (1250ms) for medium press (station selection)
        // LONG_PRESS_TIME (10000ms) reserved for future features
        int iterations = round(medPressTime / 100);
        
        for(int i = 0; i < iterations; i++){
          delay(100);
          isUpButton = !expander->digitalRead(UP_BUTTON);
          if(!isUpButton){
            // Button released before medium press time
            return 1; // Short press UP
          }
        }
        // If we get here, button was held for the full medium press time
        return 2; // Medium press UP (station selection)
      }
    }
    
    // Check DOWN button
    if(isDownButton){
      delay(buttonDebounceDelay);
      isDownButton = !expander->digitalRead(DN_BUTTON);
      if(isDownButton){
        // Check for medium press - wait and see if button stays pressed
        // Use MED_PRESS_TIME (1250ms) for medium press (station selection)
        // LONG_PRESS_TIME (10000ms) reserved for future features
        int iterations = round(medPressTime / 100);
        
        for(int i = 0; i < iterations; i++){
          delay(100);
          isDownButton = !expander->digitalRead(DN_BUTTON);
          if(!isDownButton){
            // Button released before medium press time
            return -1; // Short press DOWN
          }
        }
        // If we get here, button was held for the full medium press time
        return -2; // Medium press DOWN (station selection)
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

// Display station name, handling "/" splitting into two lines if needed
// Centers text both horizontally and vertically
void displayStationName(const String& stationName, uint8_t textSize) {
  int16_t actualWidth = gfx->width();
  int16_t actualHeight = gfx->height();
  int16_t charHeight = CHAR_HEIGHT * textSize;
  
  // Check if name contains "/" (not at start or end)
  int slashIndex = stationName.indexOf('/');
  
  if(slashIndex > 0 && slashIndex < stationName.length() - 1) {
    // Split into two lines
    String line1 = stationName.substring(0, slashIndex);
    String line2 = stationName.substring(slashIndex + 1);
    line1.trim();
    line2.trim();
    
    // Only split if both parts have content
    if(line1.length() > 0 && line2.length() > 0) {
      // Calculate Y positions to center the two-line block vertically
      int16_t lineSpacing = charHeight / 2; // Space between lines
      int16_t totalHeight = (charHeight * 2) + lineSpacing;
      int16_t y1 = (actualHeight / 2) - (totalHeight / 2);
      int16_t y2 = y1 + charHeight + lineSpacing;
      
      // Display first line (centered)
      int16_t x1 = getCenteredX(line1, textSize);
      gfx->setCursor(x1, y1);
      gfx->print(line1);
      
      // Display second line (centered)
      int16_t x2 = getCenteredX(line2, textSize);
      gfx->setCursor(x2, y2);
      gfx->print(line2);
      
      Serial.print(F("TWO-LINE DISPLAY - LINE1: \""));
      Serial.print(line1);
      Serial.print(F("\" at (")); Serial.print(x1); Serial.print(F(",")); Serial.print(y1);
      Serial.print(F("), LINE2: \""));
      Serial.print(line2);
      Serial.print(F("\" at (")); Serial.print(x2); Serial.print(F(",")); Serial.print(y2);
      Serial.println(F(")"));
      return;
    }
  }
  
  // Single line - center horizontally and vertically
  int16_t x = getCenteredX(stationName, textSize);
  int16_t y = (actualHeight / 2) - (charHeight / 2);
  gfx->setCursor(x, y);
  gfx->print(stationName);
  
  Serial.print(F("SINGLE-LINE DISPLAY: \""));
  Serial.print(stationName);
  Serial.print(F("\" at (")); Serial.print(x); Serial.print(F(",")); Serial.print(y);
  Serial.print(F("), DISPLAY SIZE: "));
  Serial.print(actualWidth);
  Serial.print(F("x"));
  Serial.print(actualHeight);
  Serial.println(F(")"));
}

// Display arriving train screen
// Shows destination (size 8) and "X CAR TRAIN ARRIVING" (size 5)
// Both centered horizontally and vertically
void displayArrivingTrain(const String& destination, uint8_t carLength) {
  clearDisplay();
  gfx->setRotation(DISPLAY_ROTATION);
  gfx->setTextColor(RED, BLACK);
  
  int16_t actualWidth = gfx->width();
  int16_t actualHeight = gfx->height();
  
  // Display destination (size 8)
  String destDisplay = destination;
  destDisplay.toUpperCase();
  gfx->setTextSize(ARRIVING_STATION_TXT);
  int16_t destTextHeight = CHAR_HEIGHT * ARRIVING_STATION_TXT;
  int16_t destX = getCenteredX(destDisplay, ARRIVING_STATION_TXT);
  
  // Display car train arriving (size 5)
  String carText = String(carLength) + F(STR_CAR_TRAIN) + F(STR_ARRIVING);
  gfx->setTextSize(ARRIVING_CAR_TXT);
  int16_t carTextHeight = CHAR_HEIGHT * ARRIVING_CAR_TXT;
  int16_t carX = getCenteredX(carText, ARRIVING_CAR_TXT);
  
  // Calculate spacing and center vertically
  int16_t lineSpacing = 20;  // Space between destination and car text
  int16_t totalHeight = destTextHeight + lineSpacing + carTextHeight;
  int16_t destY = (actualHeight / 2) - (totalHeight / 2);
  int16_t carY = destY + destTextHeight + lineSpacing;
  
  // Display destination
  gfx->setTextSize(ARRIVING_STATION_TXT);
  gfx->setCursor(destX, destY);
  gfx->print(destDisplay);
  
  // Display car train arriving
  gfx->setTextSize(ARRIVING_CAR_TXT);
  gfx->setCursor(carX, carY);
  gfx->print(carText);
  
  Serial.print(F("ARRIVING TRAIN - DESTINATION: \""));
  Serial.print(destDisplay);
  Serial.print(F("\" at (")); Serial.print(destX); Serial.print(F(",")); Serial.print(destY);
  Serial.print(F("), CAR TEXT: \""));
  Serial.print(carText);
  Serial.print(F("\" at (")); Serial.print(carX); Serial.print(F(",")); Serial.print(carY);
  Serial.println(F(")"));
}


void stationSelect(){
  Serial.println(F("=== STATION SELECT START ==="));
  Serial.print(F("NUM STATIONS AVAILABLE: "));
  Serial.println(numStations);
  
  int index = 0;
  bool selected = false;
  delay(100);
  clearDisplay();
  
  // Ensure display settings
  gfx->setRotation(DISPLAY_ROTATION);
  gfx->setTextColor(RED, BLACK);
  
  // Ensure horizontal orientation
  gfx->setRotation(DISPLAY_ROTATION);
  gfx->setTextColor(RED, BLACK);
  
  // Display "SELECT STATION" centered horizontally, near top
  gfx->setTextSize(SMALL_TXT);
  String selectText = String(STR_SELECT_STATION);
  int16_t x1 = getCenteredX(selectText, SMALL_TXT);
  int16_t y1 = DISPLAY_MARGIN + (CHAR_HEIGHT * SMALL_TXT);
  Serial.print(F("SELECT STATION COORDS - X: "));
  Serial.print(x1);
  Serial.print(F(", Y: "));
  Serial.println(y1);
  gfx->setCursor(x1, y1);
  gfx->print(F(STR_SELECT_STATION));
  
  // Display "PRESS UP OR DOWN TO SELECT" centered below
  gfx->setTextSize(INSTRUCTION_TXT);
  String instructionText = String(STR_PRESS_UP_OR_DOWN);
  int16_t x2 = getCenteredX(instructionText, INSTRUCTION_TXT);
  int16_t y2 = y1 + (CHAR_HEIGHT * SMALL_TXT) + DISPLAY_MARGIN;
  Serial.print(F("INSTRUCTION COORDS - X: "));
  Serial.print(x2);
  Serial.print(F(", Y: "));
  Serial.println(y2);
  gfx->setCursor(x2, y2);
  gfx->print(F(STR_PRESS_UP_OR_DOWN));

  Serial.println(F("WAITING FOR BUTTON PRESS TO CONTINUE..."));
  // Wait for UP or DOWN button to continue
  int buttonPress = 0;
  while(buttonPress == 0){
    buttonPress = buttonPressed(1000);
    delay(10);
  }
  Serial.print(F("BUTTON PRESSED: "));
  Serial.println(buttonPress);

  // Now show station selection - display first station immediately
  clearDisplay();
  delay(200);
  
  // Ensure index is valid
  if(index >= numStations){
    Serial.println(F("WARNING: INDEX >= NUM STATIONS, RESETTING TO 0"));
    index = 0;
  }
  if(index < 0){
    Serial.println(F("WARNING: INDEX < 0, RESETTING TO 0"));
    index = 0;
  }
  
  if(numStations == 0) {
    Serial.println(F("ERROR: NO STATIONS AVAILABLE!"));
    gfx->setTextSize(SMALL_TXT);
    gfx->setCursor(DISPLAY_MARGIN, DISPLAY_HEIGHT / 2);
    gfx->print(F("NO STATIONS LOADED"));
    delay(5000);
    return;
  }
  
  Serial.println(F("ENTERING STATION SELECTION LOOP"));
  while(!selected){
    // Wrap around
    if(index >= numStations){
      Serial.println(F("WRAPPING: INDEX >= NUM STATIONS"));
      index = 0;
    }
    if(index < 0){
      Serial.println(F("WRAPPING: INDEX < 0"));
      index = numStations - 1;
    }
    
    Serial.print(F("DISPLAYING STATION "));
    Serial.print(index);
    Serial.print(F(": "));
    Serial.println(station_name[index]);
    
    // Display current station centered
    String station = station_name[index];
    station.toUpperCase();
    
    // Ensure display settings are correct for horizontal orientation
    gfx->setRotation(DISPLAY_ROTATION);
    gfx->setTextColor(RED, BLACK);
    gfx->setTextSize(STATION_NAME_TXT);
    
    // Use the new display function that handles "/" splitting
    displayStationName(station, STATION_NAME_TXT);
    
    // Force display update if needed
    #ifdef CANVAS
    gfx->flush();
    #endif
    
    // Wait for button press
    Serial.println(F("WAITING FOR BUTTON PRESS..."));
    buttonPress = buttonPressed(60000);
    Serial.print(F("BUTTON PRESS: "));
    Serial.println(buttonPress);
    
    if(buttonPress == 1){
      // UP short press - scroll forward
      Serial.println(F("SCROLLING FORWARD"));
      index++;
      clearDisplay();
      delay(200);
    } else if(buttonPress == -1){
      // DOWN short press - scroll backward
      Serial.println(F("SCROLLING BACKWARD"));
      index--;
      clearDisplay();
      delay(200);
    } else if(buttonPress == 2 || buttonPress == -2){
      // Medium press (1.25s) on either button - select station
      Serial.print(F("STATION SELECTED: "));
      Serial.println(station_abbr[index]);
      selected_station = station_abbr[index];
      selected_station_name = station_name[index];
      selected = true;
    }
    // If buttonPress == 0 (timeout), loop continues and re-displays the same station
  }
  
  Serial.println(F("=== STATION SELECT COMPLETE ==="));

  // Show confirmation with station name
  delay(1000);
  clearDisplay();
  gfx->setRotation(DISPLAY_ROTATION);
  gfx->setTextColor(RED, BLACK);
  
  int16_t actualWidth = gfx->width();
  int16_t actualHeight = gfx->height();
  
  // Display "STATION SELECTED!" centered
  String confirmText = String(STR_STATION_SELECTED_CONFIRM);
  gfx->setTextSize(STATION_NAME_TXT);
  int16_t confirmTextHeight = CHAR_HEIGHT * STATION_NAME_TXT;
  int16_t confirmX = getCenteredX(confirmText, STATION_NAME_TXT);
  
  // Prepare station name (handle "/" splitting if needed)
  String stationDisplayName = selected_station_name;
  stationDisplayName.toUpperCase();
  int16_t stationTextHeight = CHAR_HEIGHT * STATION_NAME_TXT;
  
  // Check if station name contains "/" for splitting
  int slashIndex = stationDisplayName.indexOf('/');
  bool hasSlash = (slashIndex > 0 && slashIndex < stationDisplayName.length() - 1);
  
  int16_t lineSpacing = 15;  // Space between confirmation and station name
  int16_t stationBlockHeight;
  
  if(hasSlash) {
    // Station name will be split into two lines
    stationBlockHeight = (stationTextHeight * 2) + (stationTextHeight / 2);
  } else {
    // Single line station name
    stationBlockHeight = stationTextHeight;
  }
  
  // Calculate total height and center vertically
  int16_t totalHeight = confirmTextHeight + lineSpacing + stationBlockHeight;
  int16_t confirmY = (actualHeight / 2) - (totalHeight / 2);
  
  // Display "STATION SELECTED!"
  gfx->setCursor(confirmX, confirmY);
  gfx->print(confirmText);
  
  // Display station name (handle "/" splitting)
  if(hasSlash) {
    String line1 = stationDisplayName.substring(0, slashIndex);
    String line2 = stationDisplayName.substring(slashIndex + 1);
    line1.trim();
    line2.trim();
    
    if(line1.length() > 0 && line2.length() > 0) {
      int16_t stationLineSpacing = stationTextHeight / 2;
      int16_t stationY1 = confirmY + confirmTextHeight + lineSpacing;
      int16_t stationY2 = stationY1 + stationTextHeight + stationLineSpacing;
      
      int16_t stationX1 = getCenteredX(line1, STATION_NAME_TXT);
      int16_t stationX2 = getCenteredX(line2, STATION_NAME_TXT);
      
      gfx->setCursor(stationX1, stationY1);
      gfx->print(line1);
      gfx->setCursor(stationX2, stationY2);
      gfx->print(line2);
    }
  } else {
    // Single line station name
    int16_t stationX = getCenteredX(stationDisplayName, STATION_NAME_TXT);
    int16_t stationY = confirmY + confirmTextHeight + lineSpacing;
    gfx->setCursor(stationX, stationY);
    gfx->print(stationDisplayName);
  }
  
  Serial.println(SERIAL_MSG_STATION_SELECTED);
  Serial.print(F("SELECTED STATION NAME: "));
  Serial.println(selected_station_name);
  delay(2500);  // Half of original 5000ms delay
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
    Serial.print(F("."));
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
    Serial.println(F("GFX->BEGIN() FAILED!"));
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
  gfx->setRotation(DISPLAY_ROTATION);
  
  // Debug: Print actual display dimensions after rotation
  Serial.print(F("DISPLAY DIMENSIONS AFTER ROTATION - WIDTH: "));
  Serial.print(gfx->width());
  Serial.print(F(", HEIGHT: "));
  Serial.println(gfx->height());
  Serial.print(F("EXPECTED (HORIZONTAL): 960x320"));
  Serial.println();
  
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

  Serial.println(F("=== LOOP START ==="));
  Serial.print(F("WIFI STATUS: "));
  Serial.println(WiFi.status());
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F("WIFI CONNECTED"));
    Serial.print(F("ATTEMPTING TO FETCH STATIONS FROM: "));
    Serial.println(serverName);
    
    HTTPClient http;
    http.begin(serverName);
    http.setTimeout(10000); // 10 second timeout
    // http.setInsecure(); // Uncomment if SSL issues
    
    Serial.println(F("SENDING HTTP GET REQUEST..."));
    int httpCode = http.GET();
    Serial.print(F("HTTP RESPONSE CODE: "));
    Serial.println(httpCode);

    if (httpCode > 0) {
      Serial.println(F("HTTP REQUEST SUCCESSFUL"));
      String payload = http.getString();
      Serial.print(F("PAYLOAD LENGTH: "));
      Serial.println(payload.length());
      Serial.println(F("PAYLOAD PREVIEW (first 200 chars):"));
      Serial.println(payload.substring(0, 200));

      // Reset station count
      numStations = 0;
      
      // Parse JSON
      DynamicJsonDocument doc(JSON_DOC_SIZE);
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        Serial.println(F("JSON PARSING SUCCESSFUL"));
        JsonArray stations = doc["root"]["stations"]["station"];
        Serial.print(F("NUMBER OF STATIONS IN JSON: "));
        Serial.println(stations.size());
        
        for (JsonObject station : stations) {
          if(numStations < STAT_ARR_SIZE) {
            String name = station["name"].as<String>();
            // Replace "San Francisco International Airport" with "SFO Airport"
            if(name == "San Francisco International Airport") {
              name = "SFO Airport";
              Serial.print(F("REPLACED STATION NAME: \"San Francisco International Airport\" -> \"SFO Airport\""));
              Serial.println();
            }
            station_name[numStations] = name;
            station_abbr[numStations] = station["abbr"].as<String>();
            Serial.print(F("STATION "));
            Serial.print(numStations);
            Serial.print(F(": "));
            Serial.print(station_name[numStations]);
            Serial.print(F(" ("));
            Serial.print(station_abbr[numStations]);
            Serial.println(F(")"));
            numStations++;
          } else {
            Serial.println(F("WARNING: STATION ARRAY FULL!"));
            break;
          }
        } 
        Serial.print(SERIAL_MSG_STATIONS);
        Serial.println(numStations);
      } else {
        Serial.print(SERIAL_MSG_DESERIALIZE_FAILED);
        Serial.println(error.c_str());
        Serial.print(F("ERROR CODE: "));
        Serial.println(error.code());
      }
    } else {
      Serial.print(SERIAL_MSG_HTTP_GET_FAILED);
      Serial.println(httpCode);
      if(httpCode == -1) {
        Serial.println(F("ERROR: CONNECTION FAILED (CHECK WIFI/URL)"));
      } else if(httpCode == -11) {
        Serial.println(F("ERROR: CONNECTION TIMEOUT"));
      }
    }

    http.end();
  } else {
    Serial.println(F("WIFI NOT CONNECTED!"));
  }
  
  Serial.println(F("=== CALLING STATION SELECT ==="));

  // selected_station = station_abbr[one_station];
  stationSelect();

  // Main arrival display loop - continues until button press
  bool returnToStationSelect = false;
  
  while(!returnToStationSelect && WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String etdUrl = String(BART_API_ETD_URL_BASE) + selected_station + "&key=" + BART_API_KEY + "&json=y";
    Serial.print(F("FETCHING ARRIVAL DATA FROM: "));
    Serial.println(etdUrl);
    
    http.begin(etdUrl);
    http.setTimeout(10000);
    int httpCode = http.GET();
    
    Serial.print(F("HTTP GET CODE: "));
    Serial.println(httpCode);
    
    if (httpCode > 0) {
      String payload = http.getString();
      Serial.print(F("RECEIVED PAYLOAD (LENGTH "));
      Serial.print(payload.length());
      Serial.println(F("):"));
      Serial.println(payload);
      Serial.println(F("=== END PAYLOAD ==="));

      DynamicJsonDocument doc(JSON_DOC_SIZE);
      DeserializationError error = deserializeJson(doc, payload);

      if (error) {
        Serial.print(F("JSON DESERIALIZATION ERROR: "));
        Serial.println(error.c_str());
      } else {
        Serial.println(F("JSON DESERIALIZATION SUCCESSFUL"));
        
        // Check for and display arriving trains (0-minute North-bound arrivals)
        bool hasArrivingTrain = true;
        while(hasArrivingTrain && !returnToStationSelect) {
          hasArrivingTrain = false;
          String arrivingDestination = "";
          uint8_t arrivingCarLength = 0;
          
          JsonArray stations = doc["root"]["station"];
          for (JsonObject station : stations) {
            JsonArray etds = station["etd"];
            for (JsonObject etd : etds) {
              JsonArray estimates = etd["estimate"];
              for (JsonObject estimate : estimates) {
                String direction = estimate["direction"].as<String>();
                if(direction == "North") {
                  String tmpMin = estimate["minutes"].as<char *>();
                  uint8_t minutes = (uint8_t)(tmpMin.toInt());
                  if(minutes == 0) {
                    hasArrivingTrain = true;
                    arrivingDestination = etd["destination"].as<String>();
                    String carLength = estimate["length"].as<char *>();
                    arrivingCarLength = (uint8_t)(carLength.toInt());
                    Serial.print(F("FOUND ARRIVING TRAIN: "));
                    Serial.print(arrivingDestination);
                    Serial.print(F(", "));
                    Serial.print(arrivingCarLength);
                    Serial.println(F(" CARS"));
                    break;
                  }
                }
              }
              if(hasArrivingTrain) break;
            }
            if(hasArrivingTrain) break;
          }
          
          if(hasArrivingTrain) {
            // Display arriving train screen
            displayArrivingTrain(arrivingDestination, arrivingCarLength);
            
            // Display for 20 seconds with button check
            unsigned long displayStart = millis();
            int buttonPress = 0;
            while((millis() - displayStart) < ARRIVING_DISPLAY_DELAY && buttonPress == 0) {
              buttonPress = buttonPressed(100);  // Check for button every 100ms
              if(buttonPress != 0) {
                Serial.print(F("BUTTON PRESSED DURING ARRIVING DISPLAY: "));
                Serial.println(buttonPress);
                returnToStationSelect = true;
                break;
              }
              delay(100);
            }
            
            if(returnToStationSelect) break;
            
            // Fetch new data to check if train is still arriving
            Serial.println(F("CHECKING FOR UPDATED ARRIVAL DATA..."));
            http.end();
            http.begin(etdUrl);
            http.setTimeout(10000);
            httpCode = http.GET();
            
            if (httpCode > 0) {
              payload = http.getString();
              DeserializationError newError = deserializeJson(doc, payload);
              
              if (newError) {
                Serial.print(F("ERROR PARSING UPDATE: "));
                Serial.println(newError.c_str());
                hasArrivingTrain = false;  // Exit loop on error
              }
            } else {
              Serial.print(F("HTTP GET FAILED FOR UPDATE: "));
              Serial.println(httpCode);
              hasArrivingTrain = false;  // Exit loop on error
            }
          }
        }
        
        if(returnToStationSelect) break;
        
        // Now proceed with normal arrival display
        JsonArray stations = doc["root"]["station"];
        Serial.print(F("NUMBER OF STATIONS IN RESPONSE: "));
        Serial.println(stations.size());
        
        for (JsonObject station : stations) {
          Serial.print(F("STATION NAME: "));
          Serial.println(station["name"].as<char *>());
          Serial.print(F("STATION ABBR: "));
          Serial.println(station["abbr"].as<char *>());
          
          JsonArray etds = station["etd"];
          Serial.print(F("TOTAL NUMBER OF DESTINATIONS (ETDs): "));
          Serial.println(etds.size());
          
          // Filter ETDs to only include North-bound ones (excluding 0-minute arrivals)
          // First pass: count North-bound ETDs with non-zero minutes
          uint8_t northEtdCount = 0;
          for (JsonObject etd : etds) {
            JsonArray estimates = etd["estimate"];
            bool hasNorthNonZero = false;
            for (JsonObject estimate : estimates) {
              String direction = estimate["direction"].as<String>();
              if(direction == "North") {
                String tmpMin = estimate["minutes"].as<char *>();
                uint8_t minutes = (uint8_t)(tmpMin.toInt());
                if(minutes > 0) {  // Only count non-zero minute North estimates
                  hasNorthNonZero = true;
                  break;
                }
              }
            }
            if(hasNorthNonZero) {
              northEtdCount++;
            }
          }
          
          Serial.print(F("NUMBER OF NORTH-BOUND DESTINATIONS: "));
          Serial.println(northEtdCount);
          
          if(northEtdCount == 0) {
            Serial.println(F("NO NORTH-BOUND DESTINATIONS FOUND"));
            // Display message and wait for button
            gfx->fillScreen(BLACK);
            gfx->setTextColor(RED, BLACK);
            gfx->setRotation(DISPLAY_ROTATION);
            gfx->setTextSize(STATION_NAME_TXT);
            String noNorthMsg = "NO NORTH TRAINS";
            int16_t x = getCenteredX(noNorthMsg, STATION_NAME_TXT);
            int16_t y = (gfx->height() / 2) - ((CHAR_HEIGHT * STATION_NAME_TXT) / 2);
            gfx->setCursor(x, y);
            gfx->print(noNorthMsg);
            
            // Wait for button press or timeout
            int buttonPress = buttonPressed(30000);
            if(buttonPress != 0) {
              returnToStationSelect = true;
              break;
            }
            continue;
          }
          
          gfx->setTextColor(RED, BLACK);
          gfx->setRotation(DISPLAY_ROTATION);
          
          // Layout constants for arrival display
          int16_t displayHeight = gfx->height();
          int16_t stationTextHeight = CHAR_HEIGHT * ARRIVAL_STATION_TXT;
          int16_t carTextHeight = CHAR_HEIGHT * ARRIVAL_CARS_TXT;
          int16_t spaceBetweenStationAndCar = 8;  // Space between station name and car count
          
          // Calculate total content height
          int16_t firstSectionHeight = stationTextHeight + spaceBetweenStationAndCar + carTextHeight;
          int16_t secondSectionHeight = stationTextHeight + spaceBetweenStationAndCar + carTextHeight;
          int16_t totalContentHeight = firstSectionHeight + secondSectionHeight;
          
          // Calculate available space for margins and spacing
          int16_t availableSpace = displayHeight - totalContentHeight;
          
          // Distribute space: equal top/bottom margins, rest between sections
          int16_t topBottomMargin = availableSpace / 3;  // Equal margins at top and bottom
          int16_t spaceBetweenSections = availableSpace - (2 * topBottomMargin);  // Remaining space between sections
          
          // Calculate Y positions
          int16_t stationY1 = topBottomMargin;  // First station Y position
          int16_t firstCarY = stationY1 + stationTextHeight + spaceBetweenStationAndCar;
          int16_t stationY2 = firstCarY + carTextHeight + spaceBetweenSections;  // Second station Y position
          int16_t secondCarY = stationY2 + stationTextHeight + spaceBetweenStationAndCar;
          
          int16_t margin = ARRIVAL_MARGIN;  // Left margin
          
          // Process North-bound ETDs in pairs (excluding 0-minute arrivals)
          uint8_t northEtdIndex = 0;
          for (JsonObject etd : etds) {
            // Check if this ETD has North-bound estimates with non-zero minutes
            JsonArray estimates = etd["estimate"];
            bool hasNorthNonZero = false;
            for (JsonObject estimate : estimates) {
              String direction = estimate["direction"].as<String>();
              if(direction == "North") {
                String tmpMin = estimate["minutes"].as<char *>();
                uint8_t minutes = (uint8_t)(tmpMin.toInt());
                if(minutes > 0) {  // Only process non-zero minute North estimates
                  hasNorthNonZero = true;
                  break;
                }
              }
            }
            
            if(!hasNorthNonZero) {
              continue;  // Skip non-North or 0-minute North ETDs
            }
            
            // Only process pairs starting at even indices (0, 2, 4...)
            // Or if it's the last one and total is odd, process it alone
            if(northEtdIndex % 2 == 0 || (northEtdIndex == northEtdCount - 1 && northEtdCount % 2 == 1)) {
              // Clear screen for new pair (or single if last and odd)
              gfx->fillScreen(BLACK);
              
              // Process first ETD in pair
              String destination = (etd["destination"].as<char *>());
              destination.toUpperCase();
              
              Serial.print(F("  NORTH-BOUND DESTINATION "));
              Serial.print(northEtdIndex);
              Serial.print(F(": "));
              Serial.println(destination);
              
              JsonArray estimates = etd["estimate"];
              Serial.print(F("    NUMBER OF ESTIMATES: "));
              Serial.println(estimates.size());
              
              // Get up to 2 time estimates (minutes only) from North-bound estimates and car length
              uint8_t timeMinutes[2] = {0, 0};
              uint8_t numTimes = 0;
              uint8_t car_length = 0;
              
              // Collect up to 2 North-bound estimates
              for (JsonObject estimate : estimates) {
                if(numTimes >= 2) break;
                
                String direction = estimate["direction"].as<String>();
                if(direction != "North") {
                  continue;  // Skip non-North estimates
                }
                
                String tmpMin = estimate["minutes"].as<char *>();
                uint8_t minutes = (uint8_t)(tmpMin.toInt());
                if(minutes == 0) {
                  continue;  // Skip 0-minute estimates (handled by arriving screen)
                }
                
                timeMinutes[numTimes] = minutes;
                
                // Get car length from first North estimate
                if(numTimes == 0) {
                  String carLength = estimate["length"].as<char *>();
                  car_length = (uint8_t)(carLength.toInt());
                }
                
                Serial.print(F("      NORTH ESTIMATE "));
                Serial.print(numTimes);
                Serial.print(F(": "));
                Serial.print(timeMinutes[numTimes]);
                Serial.print(F(" MIN"));
                if(numTimes == 0) {
                  Serial.print(F(", "));
                  Serial.print(car_length);
                  Serial.print(F(" CARS"));
                }
                Serial.println();
                
                numTimes++;
              }
              
              // Sort times so smallest is first (simple bubble sort for 2 elements)
              if(numTimes == 2 && timeMinutes[0] > timeMinutes[1]) {
                uint8_t temp = timeMinutes[0];
                timeMinutes[0] = timeMinutes[1];
                timeMinutes[1] = temp;
              }
              
              // Display first ETD in pair (or single if last and odd)
              // Station name (size 8, left-aligned)
              gfx->setTextSize(ARRIVAL_STATION_TXT);
              gfx->setCursor(margin, stationY1);
              String stationText = destination.substring(0, DESTINATION_MAX_LENGTH);
              gfx->print(stationText);
              
              // Time (size 7, right-aligned, vertically centered with station)
              String timeStr = "";
              // Only add leading space for single time if < 10 (for right alignment)
              // If two times, don't add spaces even if both are < 10
              if(numTimes == 1 && timeMinutes[0] < 10) {
                timeStr += " ";
              }
              timeStr += String(timeMinutes[0]);
              if(numTimes > 1) {
                timeStr += ",";
                timeStr += String(timeMinutes[1]);
              }
              timeStr += " MIN";
              
              int16_t timeWidth = timeStr.length() * CHAR_WIDTH * ARRIVAL_TIME_TXT;
              int16_t timeX = DISPLAY_WIDTH - margin - timeWidth;
              int16_t timeTextHeight = CHAR_HEIGHT * ARRIVAL_TIME_TXT;
              int16_t timeY = stationY1 + (stationTextHeight - timeTextHeight) / 2;
              
              gfx->setTextSize(ARRIVAL_TIME_TXT);
              gfx->setCursor(timeX, timeY);
              gfx->print(timeStr);
              
              // Car count (size 6, below station name)
              gfx->setTextSize(ARRIVAL_CARS_TXT);
              gfx->setCursor(margin, firstCarY);
              gfx->print(car_length);
              gfx->print(F(STR_CAR_TRAIN));
              
              // Check if there's a second North-bound ETD in this pair
              if(northEtdIndex + 1 < northEtdCount) {
                // Find next North-bound ETD
                uint8_t nextNorthIndex = 0;
                JsonObject nextEtd;
                bool foundNext = false;
                
                for (JsonObject checkEtd : etds) {
                  JsonArray checkEstimates = checkEtd["estimate"];
                  bool hasNorth = false;
                  for (JsonObject estimate : checkEstimates) {
                    String direction = estimate["direction"].as<String>();
                    if(direction == "North") {
                      hasNorth = true;
                      break;
                    }
                  }
                  
                  if(hasNorth) {
                    if(nextNorthIndex == northEtdIndex + 1) {
                      nextEtd = checkEtd;
                      foundNext = true;
                      break;
                    }
                    nextNorthIndex++;
                  }
                }
                
                if(foundNext) {
                  // Process second ETD in pair
                  String nextDestination = (nextEtd["destination"].as<char *>());
                  nextDestination.toUpperCase();
                  
                  Serial.print(F("  NORTH-BOUND DESTINATION "));
                  Serial.print(northEtdIndex + 1);
                  Serial.print(F(": "));
                  Serial.println(nextDestination);
                  
                  JsonArray nextEstimates = nextEtd["estimate"];
                  Serial.print(F("    NUMBER OF ESTIMATES: "));
                  Serial.println(nextEstimates.size());
                  
                  // Get up to 2 time estimates for second ETD (North-bound only)
                  uint8_t nextTimeMinutes[2] = {0, 0};
                  uint8_t nextNumTimes = 0;
                  uint8_t nextCarLength = 0;
                  
                  for (JsonObject estimate : nextEstimates) {
                    if(nextNumTimes >= 2) break;
                    
                    String direction = estimate["direction"].as<String>();
                    if(direction != "North") {
                      continue;  // Skip non-North estimates
                    }
                    
                    String tmpMin = estimate["minutes"].as<char *>();
                    uint8_t minutes = (uint8_t)(tmpMin.toInt());
                    if(minutes == 0) {
                      continue;  // Skip 0-minute estimates (handled by arriving screen)
                    }
                    
                    nextTimeMinutes[nextNumTimes] = minutes;
                    
                    if(nextNumTimes == 0) {
                      String carLength = estimate["length"].as<char *>();
                      nextCarLength = (uint8_t)(carLength.toInt());
                    }
                    
                    Serial.print(F("      NORTH ESTIMATE "));
                    Serial.print(nextNumTimes);
                    Serial.print(F(": "));
                    Serial.print(nextTimeMinutes[nextNumTimes]);
                    Serial.print(F(" MIN"));
                    if(nextNumTimes == 0) {
                      Serial.print(F(", "));
                      Serial.print(nextCarLength);
                      Serial.print(F(" CARS"));
                    }
                    Serial.println();
                    
                    nextNumTimes++;
                  }
                  
                  // Sort times
                  if(nextNumTimes == 2 && nextTimeMinutes[0] > nextTimeMinutes[1]) {
                    uint8_t temp = nextTimeMinutes[0];
                    nextTimeMinutes[0] = nextTimeMinutes[1];
                    nextTimeMinutes[1] = temp;
                  }
                  
                  // Display second ETD in pair
                  // Station name (size 8, left-aligned)
                  gfx->setTextSize(ARRIVAL_STATION_TXT);
                  gfx->setCursor(margin, stationY2);
                  String nextStationText = nextDestination.substring(0, DESTINATION_MAX_LENGTH);
                  gfx->print(nextStationText);
                  
                  // Time (size 7, right-aligned, vertically centered with station)
                  String nextTimeStr = "";
                  // Only add leading space for single time if < 10 (for right alignment)
                  // If two times, don't add spaces even if both are < 10
                  if(nextNumTimes == 1 && nextTimeMinutes[0] < 10) {
                    nextTimeStr += " ";
                  }
                  nextTimeStr += String(nextTimeMinutes[0]);
                  if(nextNumTimes > 1) {
                    nextTimeStr += ",";
                    nextTimeStr += String(nextTimeMinutes[1]);
                  }
                  nextTimeStr += " MIN";
                  
                  int16_t nextTimeWidth = nextTimeStr.length() * CHAR_WIDTH * ARRIVAL_TIME_TXT;
                  int16_t nextTimeX = DISPLAY_WIDTH - margin - nextTimeWidth;
                  int16_t nextTimeY = stationY2 + (stationTextHeight - timeTextHeight) / 2;
                  
                  gfx->setTextSize(ARRIVAL_TIME_TXT);
                  gfx->setCursor(nextTimeX, nextTimeY);
                  gfx->print(nextTimeStr);
                  
                  // Car count (size 6, below station name)
                  gfx->setTextSize(ARRIVAL_CARS_TXT);
                  gfx->setCursor(margin, secondCarY);
                  gfx->print(nextCarLength);
                  gfx->print(F(STR_CAR_TRAIN));
                }
              }
              
              // Display delay with button check
              unsigned long displayStart = millis();
              int buttonPress = 0;
              while((millis() - displayStart) < ARRIVAL_DISPLAY_DELAY && buttonPress == 0) {
                buttonPress = buttonPressed(100);  // Check for button every 100ms
                if(buttonPress != 0) {
                  Serial.print(F("BUTTON PRESSED DURING DISPLAY: "));
                  Serial.println(buttonPress);
                  returnToStationSelect = true;
                  break;
                }
                delay(100);
              }
              
              if(returnToStationSelect) break;
              
              // Blank screen between sets (except after last set) with button check
              if(northEtdIndex + 2 < northEtdCount) {
                gfx->fillScreen(BLACK);
                unsigned long blankStart = millis();
                buttonPress = 0;
                while((millis() - blankStart) < ARRIVAL_BLANK_DELAY && buttonPress == 0) {
                  buttonPress = buttonPressed(50);  // Check for button every 50ms
                  if(buttonPress != 0) {
                    Serial.print(F("BUTTON PRESSED DURING BLANK: "));
                    Serial.println(buttonPress);
                    returnToStationSelect = true;
                    break;
                  }
                  delay(50);
                }
                if(returnToStationSelect) break;
              }
            }
            
            northEtdIndex++;
          }
          
          // After displaying all North-bound ETDs, check for button before fetching new data
          if(!returnToStationSelect) {
            Serial.println(F("ALL NORTH-BOUND ARRIVALS DISPLAYED, CHECKING FOR BUTTON BEFORE REFRESH"));
            int buttonPress = buttonPressed(1000);  // Brief check for button
            if(buttonPress != 0) {
              Serial.print(F("BUTTON PRESSED BEFORE REFRESH: "));
              Serial.println(buttonPress);
              returnToStationSelect = true;
            }
          }
        }
      }
    } else {
      Serial.print(F("HTTP GET FAILED, CODE: "));
      Serial.println(httpCode);
    }
    
    http.end();
    
    if(returnToStationSelect) {
      Serial.println(F("RETURNING TO STATION SELECTION"));
      break;
    }
    
    // Small delay before fetching new data
    delay(500);
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
