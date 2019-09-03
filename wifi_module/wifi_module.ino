#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

#include <ESP8266HTTPClient.h>

#include <DNSServer.h>
#include <WiFiManager.h>

extern "C" {
#include "user_interface.h"
}

// #define DEBUG
// #define IGNORE_CKSUM

#define MAX_WIFI_CONNECT_RETRY 1
#define MAX_CLOUD_CONNECT_RETRY 1

#define HOST_BUF_SIZE 50UL
#define FURL_BUF_SIZE 128UL
#define NID_BUF_SIZE 16UL
#define AP_BUF_SIZE 25UL
#define MES_BUF_SIZE 750UL

#define SERIAL_COMMAND_TIMEOUT 1000UL
#define REQUEST_TIMEOUT 5000UL

#ifdef DEBUG
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.print(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

const int httpPort = 80;

WiFiManager wifiManager;
WiFiClient client;

uint8_t command[6];
uint16_t datalen = 0;

char host[HOST_BUF_SIZE] = "";
char *url;

uint8_t buf_url[FURL_BUF_SIZE];
uint8_t buf_mes[MES_BUF_SIZE];
uint8_t buf_nid[NID_BUF_SIZE];

bool isNodeIDSet = false;
bool isURLSet = false;

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
  wifiManager.setNodeID((char *)buf_nid);
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
  system_update_cpu_freq(80);
//  system_update_cpu_freq(160);

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
  // wifiManager.resetSettings();
  // wifiManager.EEPROMClearCredential(0, true);

  // Disable debug print
#ifdef DEBUG
  wifiManager.setDebugOutput(1);
#else
  wifiManager.setDebugOutput(0);
#endif

  delay(3000);

  Serial.print("\r\nRDY\r\n");
}

void loop()
{

  if (!read_command())
  {
    return;
  }

  if (cmpcmd("URL"))
  {
    if (handle_url())
    {
      Serial.print("\r\nOKK\r\n");
    }
  }
  else if (cmpcmd("NID"))
  {
    if (handle_nid())
    {
      Serial.print("\r\nOKK\r\n");
    }
  }
  else if (cmpcmd("STA"))
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.print("\r\nOKK\r\n");
    }
    else
    {
      Serial.print("\r\nE01\r\n");
    }
  }
  else if (cmpcmd("CON"))
  {
    if (wifiManager.tryConnect())
    {
      Serial.print("\r\nOKK\r\n");
    }
    else
    {
      Serial.print("\r\nE01\r\n");
    }
  }
  else if (cmpcmd("GET"))
  {
    if (handle_get())
    {
      Serial.print("\r\nOKK\r\n");
    }
  }
  else if (cmpcmd("POS"))
  {
    if (handle_pos())
    {
      Serial.print("\r\nOKK\r\n");
    }
  }
  else if (cmpcmd("APM"))
  {
    if (isNodeIDSet == false)
    {
        Serial.print("\r\nE42\r\n");
    }
    else
    {
      wifiManager.setConfigPortalTimeout(((uint16_t)command[3]) * 256 + ((uint16_t)command[4]));
      char ap_name[AP_BUF_SIZE] = "clarity_";
      strcat(ap_name, (char*)buf_nid);
      
      uint8_t result = wifiManager.startConfigPortal(ap_name);
      if (result == AP_MODE_CONNECT_SUCCESS)
      {
        Serial.print("\r\nOKK\r\n");
      }
      else if (result == AP_MODE_CONNECT_TIMEOUT)
      {
        Serial.print("\r\nE22\r\n");
      }
      else
      {
        Serial.print("\r\nE21\r\n");
      }
      wifiManager.setConfigPortalTimeout(0);
    }
  }
  else
  {
    Serial.print("\r\nE17\r\n");
  }
}