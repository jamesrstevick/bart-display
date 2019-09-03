

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

#include <ESP8266HTTPClient.h>

#include <DNSServer.h>
#include <bartWiFiManager.h>

#include <ArduinoJson.h> 

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

extern "C" {
#include "user_interface.h"
}

#define DEBUG

#define OLED_RESET -1
// OLED Screen Settings
#define SCREEN_HEIGHT 32 
#define SCREEN_WIDTH  128

#define MAX_WIFI_CONNECT_RETRY 1
#define MAX_CLOUD_CONNECT_RETRY 1

#define HOST_BUF_SIZE 50UL
#define FURL_BUF_SIZE 128UL
#define NID_BUF_SIZE 16UL
#define AP_BUF_SIZE 25UL
#define MES_BUF_SIZE 750UL


#define SER_BUF_SIZE 6UL
#define AP_BUF_SIZE 18UL


#define SERIAL_COMMAND_TIMEOUT 1000UL
#define REQUEST_TIMEOUT 5000UL

#ifdef DEBUG
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.print(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int httpPort = 80;

WiFiManager wifiManager;
WiFiClient client;

uint8_t command[6];
uint16_t datalen = 0;

char host[HOST_BUF_SIZE] = "";
char *url;

char ap_name[AP_BUF_SIZE] = "BART_DISPLAY_";
char serial_number[SER_BUF_SIZE] = "00001";

uint8_t buf_url[FURL_BUF_SIZE];
uint8_t buf_mes[MES_BUF_SIZE];
uint8_t buf_nid[NID_BUF_SIZE];

bool isNodeIDSet = false;
bool isURLSet = false;
bool isWifi = false;

bool read_command()
{

  uint32_t t0;

  while (!Serial.available())
  {
    delay(5);
  }

  t0 = millis();
  for (uint8_t n = 0; n < 6; n++)
  {
    while (!Serial.available())
    {
      if (millis() - t0 > SERIAL_COMMAND_TIMEOUT)
      {
        Serial.print("\r\nE14\r\n");
        return false;
      }
    }
    command[n] = Serial.read();
  }

  if (command[3] + command[4] == command[5])
  {
    return true;
  }
  else
  {
    Serial.print("\r\nE12\r\n");
    return false;
  }
}

bool read_data(uint8_t *buf, uint16_t buf_size)
{
  uint16_t len = ((uint16_t)command[3]) * 256 + ((uint16_t)command[4]);

  if (len >= buf_size - 1)
  {
    Serial.print("\r\nE38\r\n");
    return false;
  }

  datalen = len;

  uint32_t t0;
  uint16_t cksum;
  uint8_t to_read;

  uint16_t m = 0;

  Serial.print("\r\nACK\r\n");
  t0 = millis();

  while (m < len)
  {
    cksum = 0;

    if (m + 60 < len)
    {
      to_read = 60;
    }
    else
    {
      to_read = len - m;
    }

    while (!Serial.available())
    {
      if (millis() - t0 > SERIAL_COMMAND_TIMEOUT)
      {
        Serial.print("\r\nE14\r\n");
        return false;
      }
    }

    t0 = millis();
    for (uint8_t n = 0; n < to_read; n++)
    {

      while (!Serial.available())
      {
        if (millis() - t0 > SERIAL_COMMAND_TIMEOUT)
        {
          Serial.print("\r\nE14\r\n");
          return false;
        }
      }
      buf[m] = Serial.read();
      cksum += (uint16_t)buf[m];
      m++;
    }

    while (Serial.available() < 2)
    {
      if (millis() - t0 > SERIAL_COMMAND_TIMEOUT)
      {
        Serial.print("\r\nE14\r\n");
        return false;
      }
    }

    uint16_t tmp = (uint16_t)Serial.read();
    tmp *= 256UL;
    tmp += (uint16_t)Serial.read();

#ifdef IGNORE_CKSUM
    tmp = cksum;
#endif

    if (tmp != cksum)
    {
      m -= to_read;
      Serial.print("\r\nE12\r\n");
    }
    else
    {
      Serial.print("\r\nACK\r\n");
    }
    t0 = millis();
  }
  buf[m] = '\0';
  return true;
}

bool handle_url()
{

  if (!read_data(buf_url, sizeof(buf_url)))
  {
    return false;
  }

  /* TODO: Hacky solution. Advice is to add separate command for
          host and url.... but whatever... I wanna test node fw asap
  */
  char *hostPtr = strstr((char *)buf_url, "//");
  hostPtr += 2;
  url = strstr(hostPtr, "/");
  uint8_t host_size = (uint8_t)(url - hostPtr);

  // We can use safer version of memcpy instead.
  if (host_size < HOST_BUF_SIZE)
  {
    memcpy(host, hostPtr, host_size);
  }
  else
  {
    Serial.print("\r\nE38\r\n");
    return false;
  }

  isURLSet = true;
  return true;
}

bool handle_nid()
{

  if (!read_data(buf_nid, sizeof(buf_nid)))
  {
    return false;
  }

  isNodeIDSet = true;
  // wifiManager.setNodeID((char *)buf_nid);
  return true;
}

bool handle_get()
{

  if (!isURLSet)
  {
    Serial.print("\r\nE31\r\n");
    return false;
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("\r\nE01\r\n");
    return false;
  }

  if (!client.connected())
  {
    uint8_t no_of_tries = 0;
    while (!client.connect(host, httpPort))
    {
      no_of_tries++;
      if (no_of_tries > MAX_CLOUD_CONNECT_RETRY)
      {
        Serial.print("\r\nE02\r\n");
        return false;
      }
      delay(1000);
    }
  }

  /* Send a GET request */
  client.print(String("GET ") + String(url) + " HTTP/1.1\r\n" +
               "Host: " + String(host) + "\r\n" +
               "Connection: close\r\n\r\n");

  /* Wait for the request to get through */
  unsigned long timeout = millis();
  while (client.available() == 0)
  {
    if (millis() - timeout > REQUEST_TIMEOUT)
    {
      Serial.print("\r\nE03\r\n");
      client.stop();
      return false;
    }
  }

  /* Read the server response */
  while (client.available())
  {
    uint8_t ch = client.read();
    Serial.write(ch);
  }

  return true;
}

bool handle_pos()
{

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("\r\nE01\r\n");
    return false;
  }

  if (!client.connected())
  {
    uint8_t no_of_tries = 0;
    while (!client.connect(host, httpPort))
    {
      no_of_tries++;
      if (no_of_tries > MAX_CLOUD_CONNECT_RETRY)
      {
        Serial.print("\r\nE02\r\n");
        return false;
      }
      delay(1000);
    }
  }

  if (!read_data(buf_mes, sizeof(buf_mes)))
  {
    return false;
  }

  client.write((const uint8_t *)buf_mes, datalen);

  /* Wait for the request to get through */
  unsigned long timeout = millis();
  while (client.available() == 0)
  {
    if (millis() - timeout > REQUEST_TIMEOUT)
    {
      Serial.print("\r\nE04\r\n");
      client.stop();
      return false;
    }
  }

  /* Read the server response */
  while (client.available())
  {
    uint8_t ch = client.read();
    Serial.write(ch);
  }

  return true;
}

bool cmpcmd(const char *cmd)
{
  if (command[0] == cmd[0])
  {
    if (command[1] == cmd[1])
    {
      if (command[2] == cmd[2])
      {
        return true;
      }
    }
  }
  return false;
}


void setup()
{
  // Setup Screen
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); 
  //display.setRotation(2);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.clearDisplay();
  display.display();

  display.setCursor(0,0);
  display.println("BART");
  display.print("Display");
  display.display();
  
  // Setup Wifi
  system_update_cpu_freq(80);
  // Disable autoconnect
  if (WiFi.getAutoConnect())
  {
    WiFi.setAutoConnect(false);
  }
  
  // Do not let ESP8266WiFiGenericClass writing to flash to avoid wear.
  WiFi.persistent(false);

  // start serial communication
  Serial.begin(115200);

  // Reset saved settings
  wifiManager.resetSettings(); // Just use this to reset settings
  wifiManager.EEPROMClearCredential(0, true);

  // Disable debug print
#ifdef DEBUG
  wifiManager.setDebugOutput(1);
#else
  wifiManager.setDebugOutput(0);
#endif

  delay(1500);
  clearDisplay();
  delay(100);
  display.setCursor(0,0);
  display.println("Connecting");
  display.print("to Wi-Fi...");
  display.display();

  isWifi = wifiManager.tryConnect();
  if(!isWifi){
    apMode();
    delay(1000);
  }

  if (WiFi.status() == WL_CONNECTED) {
      clearDisplay();
      delay(100);
      display.setCursor(0,0);
      display.println("Connected");
      display.print(wifiManager._ssid);
      display.display();
      delay(1500);
      clearDisplay();
  } 
}

void displayText(String text){
  display.setCursor(0,0);
  display.clearDisplay();
  display.print(text);
  display.display();
  
}

void clearDisplay(){
  display.clearDisplay();
  display.display();
}

void apMode()
{
  wifiManager.setDeviceID(serial_number);
  wifiManager.setConfigPortalTimeout(600);

  strcat(ap_name, serial_number);
  uint8_t result = wifiManager.startConfigPortal(ap_name);

  if (result == AP_MODE_CONNECT_SUCCESS)
  {
    Serial.println("SUCCESSFULLY CONNECTED");
  }
  else if (result == AP_MODE_CONNECT_TIMEOUT)
  {
    Serial.println("CONNECTION TIMED OUT");
  }
  else
  {
    Serial.println("USER INPUT CREDENTIALS FAILED");
  }
  wifiManager.setConfigPortalTimeout(0);
}

void loop()
{
  delay(1000);

  if (WiFi.status() == WL_CONNECTED) {

    HTTPClient http;  //Object of class HTTPClient
    http.begin("http://api.bart.gov/api/stn.aspx?cmd=stns&key=MW9S-E7SL-26DU-VV8V&json=y");
    int httpCode = http.GET();
    //Check the returning code                                                                  
    if (httpCode > 0) {

      Stream& response = http.getStream();
      // Allocate the JsonDocument in the heap
      DynamicJsonDocument doc(2048);
      // Deserialize the JSON document in the response
      deserializeJson(doc, response);

      JsonArray repos = doc["root"]["station"];

      // // Parsing
      // const size_t bufferSize = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(8) + 370;
      // DynamicJsonDocument doc(bufferSize);
      // DeserializationError error = deserializeJson(doc, client);
      // // Parameters
      // const char* minutes = doc["root"]["station"]["etd"]["estimate"]["minutes"].as<char*>(); // "Leanne Graham"

      // // Output to serial monitor
      // display.setCursor(0,0);
      // display.println("Time:");
      // display.print(doc["root"]["station"]["etd"]["estimate"]["minutes"].as<char*>());
      // display.display();


    }
    http.end();   //Close connection
  }
  // Delay
  delay(60000);

  // if (!read_command())
  // {
  //   return;
  // }

  // if (cmpcmd("URL"))
  // {
  //   if (handle_url())
  //   {
  //     Serial.print("\r\nOKK\r\n");
  //   }
  // }
  // else if (cmpcmd("NID"))
  // {
  //   if (handle_nid())
  //   {
  //     Serial.print("\r\nOKK\r\n");
  //   }
  // }
  // else if (cmpcmd("STA"))
  // {
  //   if (WiFi.status() == WL_CONNECTED)
  //   {
  //     Serial.print("\r\nOKK\r\n");
  //   }
  //   else
  //   {
  //     Serial.print("\r\nE01\r\n");
  //   }
  // }
  // else if (cmpcmd("CON"))
  // {
  //   if (wifiManager.tryConnect())
  //   {
  //     Serial.print("\r\nOKK\r\n");
  //   }
  //   else
  //   {
  //     Serial.print("\r\nE01\r\n");
  //   }
  // }
  // else if (cmpcmd("GET"))
  // {
  //   if (handle_get())
  //   {
  //     Serial.print("\r\nOKK\r\n");
  //   }
  // }
  // else if (cmpcmd("POS"))
  // {
  //   if (handle_pos())
  //   {
  //     Serial.print("\r\nOKK\r\n");
  //   }
  // }
}
