/**************************************************************
   WiFiManager is a library for the ESP8266/Arduino platform
   (https://github.com/esp8266/Arduino) to enable easy
   configuration and reconfiguration of WiFi credentials using a Captive Portal
   inspired by:
   http://www.esp8266.com/viewtopic.php?f=29&t=2520
   https://github.com/chriscook8/esp-arduino-apboot
   https://github.com/esp8266/Arduino/tree/esp8266/hardware/esp8266com/esp8266/libraries/DNSServer/examples/CaptivePortalAdvanced
   Built by AlexT https://github.com/tzapu
   Licensed under MIT license
 **************************************************************/

#ifndef bartWiFiManager_h
#define bartWiFiManager_h

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <memory>
#include <EEPROM.h>

//Settings
#define TRY_ALSO_BAD_WIFI_CREDENTIAL false

//Parameter
#define EEPROM_CREDENTIAL_LOC 0
#define EEPROM_SSID_LOC 1
#define EEPROM_PASSWORD_LOC 35

#define EEPROM_GOOD_CREDENTIAL_LOC 100
#define EEPROM_GOOD_SSID_LOC 101
#define EEPROM_GOOD_PASSWORD_LOC 135

#define EEPROM_SSID_MAX 33
#define EEPROM_PASSWORD_MAX 64

#define AP_MODE_CONNECT_SUCCESS 1
#define AP_MODE_CONNECT_FAIL 2
#define AP_MODE_CONNECT_TIMEOUT 4

extern "C" {
#include "user_interface.h"
}

const char HTTP_HEADER[] PROGMEM = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/><title>{v}</title>";
const char HTTP_STYLE[] PROGMEM = "<style>.c{text-align: center;} div,input{padding:5px;font-size:1em;} input{width:95%;} body{text-align: center;font-family:verdana;} button{border:0;border-radius:0.3rem;background-color:#ff7434;color:#fff;line-height:2.4rem;font-size:1.2rem;width:100%;} .q{float: right;width: 64px;text-align: right;} .l{background: url(\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAMAAABEpIrGAAAALVBMVEX///8EBwfBwsLw8PAzNjaCg4NTVVUjJiZDRUUUFxdiZGSho6OSk5Pg4eFydHTCjaf3AAAAZElEQVQ4je2NSw7AIAhEBamKn97/uMXEGBvozkWb9C2Zx4xzWykBhFAeYp9gkLyZE0zIMno9n4g19hmdY39scwqVkOXaxph0ZCXQcqxSpgQpONa59wkRDOL93eAXvimwlbPbwwVAegLS1HGfZAAAAABJRU5ErkJggg==\") no-repeat left center;background-size: 1em;}</style>";
const char HTTP_SCRIPT[] PROGMEM = "<script>function c(l){document.getElementById('s').value=l.innerText||l.textContent;document.getElementById('p').focus();}</script>";
const char HTTP_HEAD_END[] PROGMEM = "</head><body><div style='text-align:left;display:inline-block;width:300px;'>";

const char HTTP_PORTAL_OPTIONS0[] PROGMEM = "<form action=\"/connect\" method='post'><button type=\"submit\" value=\"y\" name=\"to_connect\">Connect Now</button></form>";
const char HTTP_PORTAL_OPTIONS1[] PROGMEM = "<a href=\"/wifi\"><button>Configure WiFi</button></a>";
const char HTTP_PORTAL_OPTIONS2[] PROGMEM = "<form action=\"/\" method='post'><button type=\"submit\" value=\"y\" name=\"reset_pw\">Reset Password</button></form>";

const char HTTP_ITEM[] PROGMEM = "<div><a href='#p' onclick='c(this)'>{v}</a>&nbsp;<span class='q {i}'>{r}%</span></div>";
const char HTTP_FORM_START[] PROGMEM = "<form action=\"/connect\" method='post'><input id='s' name='s' length=32 placeholder='SSID'><br/><input id='p' name='p' length=64 type='password' placeholder='Password'><br/>";
const char HTTP_FORM_PARAM[] PROGMEM = "<br/><input id='{i}' name='{n}' length={l} placeholder='{p}' value='{v}' {c}>";
const char HTTP_FORM_END[] PROGMEM = "<br/><button type='submit'>Connect to WiFi</button></form>";
const char HTTP_SCAN_LINK[] PROGMEM = "<br/><div class=\"c\"><a href=\"/wifi\">re-scan available networks</a></div>";
const char HTTP_SAVED[] PROGMEM = "BART Display is connecting to Wi-Fi<br><br>Wait a few seconds and the deice will display Connected.";

const char HTTP_END[] PROGMEM = "</div></body></html>";

class WiFiManager
{
public:
  WiFiManager();

  boolean tryConnect(boolean try2ndfirst = false, boolean tryonlyonce = !TRY_ALSO_BAD_WIFI_CREDENTIAL);
  uint8_t startConfigPortal(char const *apName);
  void resetSettings();

  void setDeviceID(char const *deviceID);

  String _ssid = "";

  void setConfigPortalTimeout(unsigned long seconds);
  void setConnectTimeout(unsigned long seconds);
  void setDebugOutput(boolean debug);
  bool EEPROMClearCredential(uint8_t iloc, boolean clearboth = false);


private:
  bool EEPROMisCredentialSaved(uint8_t iloc);
  // bool EEPROMClearCredential(uint8_t iloc, boolean clearboth = false);
  bool EEPROMSaveCredential(const char *buf_ssid, const char *buf_password, uint8_t iloc);
  bool EEPROMLoadCredential(char *buf_ssid, char *buf_password, uint8_t iloc);

  boolean saved0 = false;
  boolean saved1 = false;

  String savedssid0 = "";
  String savedssid1 = "";

  std::unique_ptr<DNSServer> dnsServer;
  std::unique_ptr<ESP8266WebServer> server;

  void setupConfigPortal();

  const char *_apName = "no-net";

  String _pass = "";

  boolean _connect_with_old_pw = false;

  unsigned long _configPortalTimeout = 0;
  unsigned long _connectTimeout = 0;
  unsigned long _configPortalStart = 0;

  String _deviceID = "";

  const int _minimumQuality = -1;
  const boolean _removeDuplicateAPs = true;

  int status = WL_IDLE_STATUS;
  int connectWifi(String ssid, String pass);
  uint8_t waitForConnectResult();

  void handleRoot();
  void handleWifi(boolean scan);
  void handleWifiSave();
  void handleNotFound();
  void handle204();
  boolean captivePortal();

  // DNS server
  const byte DNS_PORT = 53;

  //helpers
  int getRSSIasQuality(int RSSI);
  boolean isIp(String str);
  String toStringIp(IPAddress ip);

  boolean connect = false;
  boolean _debug = true;

  template <typename Generic>
  void DEBUG_WM(Generic text);

  template <typename Generic>
  void DEBUG_WMa(Generic text);

  template <typename Generic>
  void DEBUG_WMb(Generic text);
};

#endif
