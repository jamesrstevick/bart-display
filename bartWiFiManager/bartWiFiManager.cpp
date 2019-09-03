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

#include "bartWiFiManager.h"

uint8_t CREDENTIAL_LOC[] = {EEPROM_CREDENTIAL_LOC, EEPROM_GOOD_CREDENTIAL_LOC};
uint8_t SSID_LOC[] = {EEPROM_SSID_LOC, EEPROM_GOOD_SSID_LOC};
uint8_t PASSWORD_LOC[] = {EEPROM_PASSWORD_LOC, EEPROM_GOOD_PASSWORD_LOC};

WiFiManager::WiFiManager()
{
}

bool WiFiManager::EEPROMisCredentialSaved(uint8_t iloc)
{

  if (iloc >= sizeof(CREDENTIAL_LOC))
  {
    return false;
  }

  EEPROM.begin(512);
  bool tmp = (1 == EEPROM.read(CREDENTIAL_LOC[iloc]));
  EEPROM.end();
  return tmp;
}

bool WiFiManager::EEPROMClearCredential(uint8_t iloc, boolean clearboth)
{

  if (iloc >= sizeof(CREDENTIAL_LOC))
  {
    return false;
  }

  DEBUG_WMa(F("clearing saved ssid/password at posi="));
  if (clearboth)
  {
    DEBUG_WMb(F("0, 1"));
  }
  else
  {
    if (iloc == 0)
    {
      DEBUG_WMb(F("0"));
    }
    else if (iloc == 1)
    {
      DEBUG_WMb(F("1"));
    }
  }

  EEPROM.begin(512);

  if (1 == EEPROM.read(CREDENTIAL_LOC[iloc]))
  {
    EEPROM.write(CREDENTIAL_LOC[iloc], 0);
  }

  if (clearboth)
  {
    if (1 == EEPROM.read(CREDENTIAL_LOC[1 - iloc]))
    {
      EEPROM.write(CREDENTIAL_LOC[1 - iloc], 0);
    }
  }

  EEPROM.commit();
  EEPROM.end();
  return true;
}

bool WiFiManager::EEPROMSaveCredential(const char *buf_ssid, const char *buf_password, uint8_t iloc)
{

  if (iloc >= sizeof(CREDENTIAL_LOC))
  {
    return false;
  }

  if (iloc == 0)
  {
    DEBUG_WMa(F("saving ssid/password to posi=0: "));
  }
  else if (iloc == 1)
  {
    DEBUG_WMa(F("saving ssid/password to posi=1: "));
  }
  else
  {
    DEBUG_WMa(F("saving ssid/password: "));
  }
  DEBUG_WMb(String(buf_ssid));

  EEPROM.begin(512);

  bool sofarsuccess = false;
  for (uint8_t n = 0; n < EEPROM_SSID_MAX; n++)
  {
    EEPROM.write(SSID_LOC[iloc] + n, buf_ssid[n]);
    if (buf_ssid[n] == 0x00)
    {
      sofarsuccess = true;
      break;
    }
  }

  if (sofarsuccess)
  {

    sofarsuccess = false;
    for (uint8_t n = 0; n < EEPROM_PASSWORD_MAX; n++)
    {
      EEPROM.write(PASSWORD_LOC[iloc] + n, buf_password[n]);
      if (buf_password[n] == 0x00)
      {
        sofarsuccess = true;
        break;
      }
    }
  }

  if (sofarsuccess)
  {
    EEPROM.write(CREDENTIAL_LOC[iloc], 1);
  }
  else
  {
    EEPROM.write(CREDENTIAL_LOC[iloc], 0);
    DEBUG_WM(F("failed to save ssid/password"));
  }

  EEPROM.commit();
  EEPROM.end();
  return sofarsuccess;
}

bool WiFiManager::EEPROMLoadCredential(char *buf_ssid, char *buf_password, uint8_t iloc)
{
  if (iloc >= sizeof(CREDENTIAL_LOC))
  {
    return false;
  }
  DEBUG_WMa(F("loading ssid/password from posi="));
  DEBUG_WMb(iloc);

  EEPROM.begin(512);

  bool sofarsuccess = false;
  for (uint8_t n = 0; n < EEPROM_SSID_MAX; n++)
  {
    buf_ssid[n] = EEPROM.read(SSID_LOC[iloc] + n);
    if (buf_ssid[n] == 0x00)
    {
      if (n == 0)
      {
        sofarsuccess = false;
      }
      else
      {
        DEBUG_WMa(F("got ssid: "));
        DEBUG_WMb(String(buf_ssid));
        sofarsuccess = true;
      }
      break;
    }
  }

  if (sofarsuccess)
  {
    sofarsuccess = false;
    for (uint8_t n = 0; n < EEPROM_PASSWORD_MAX; n++)
    {
      buf_password[n] = EEPROM.read(PASSWORD_LOC[iloc] + n);
      if (buf_password[n] == 0x00)
      {
        sofarsuccess = true;
        break;
      }
    }
  }

  if (!sofarsuccess)
  {
    DEBUG_WM(F("failed to load ssid/password"));
  }

  EEPROM.commit();
  EEPROM.end();
  return sofarsuccess;
}

void WiFiManager::setupConfigPortal()
{
  dnsServer.reset(new DNSServer());
  server.reset(new ESP8266WebServer(80));

  _configPortalStart = millis();

  DEBUG_WMa(F("configuring access point: "));
  DEBUG_WMb(_apName);

  WiFi.softAP(_apName);

  delay(500); // Without delay I've seen the IP address blank
  DEBUG_WMa(F("AP IP address: "));
  DEBUG_WMb(WiFi.softAPIP());

  /* Setup the DNS server redirecting all the domains to the apIP */
  dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer->start(DNS_PORT, "*", WiFi.softAPIP());

  /* Setup web pages: root, wifi config pages, SO captive portal detectors and not found. */
  server->on("/", std::bind(&WiFiManager::handleRoot, this));
  server->on("/wifi", std::bind(&WiFiManager::handleWifi, this, true));
  server->on("/connect", std::bind(&WiFiManager::handleWifiSave, this));
  server->on("/fwlink", std::bind(&WiFiManager::handleRoot, this)); //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
  server->onNotFound(std::bind(&WiFiManager::handleNotFound, this));
  server->begin(); // Web server start
  DEBUG_WM(F("HTTP server started"));
}

boolean WiFiManager::tryConnect(boolean try2ndfirst, boolean tryonlyonce)
{
  // attempt to connect; should it fail, fall back to AP
  WiFi.mode(WIFI_STA);

  char buf_ssid[EEPROM_SSID_MAX];
  char buf_password[EEPROM_PASSWORD_MAX];

  uint8_t tryfirst = 0;
  uint8_t trysecond = 1;

  if (try2ndfirst)
  {
    tryfirst = 1;
    trysecond = 0;
  }

  boolean itried = false;
  if (EEPROMisCredentialSaved(tryfirst))
  {
    DEBUG_WMa(F("try load credential found at posi="));
    DEBUG_WMb(tryfirst);
    if (EEPROMLoadCredential(buf_ssid, buf_password, tryfirst))
    {
      if (connectWifi(String(buf_ssid), String(buf_password)) == WL_CONNECTED)
      {
        DEBUG_WM(F("connection succeed"));
        return true;
      }
      else
      {
        DEBUG_WM(F("connection failed"));
      }
    }
    else
    {
      DEBUG_WM(F("failed to read EEPROM"));
    }
    itried = true;
  }

  if (!tryonlyonce)
  {
    if (EEPROMisCredentialSaved(trysecond))
    {
      DEBUG_WMa(F("try load credential found at posi="));
      DEBUG_WMb(trysecond);
      if (EEPROMLoadCredential(buf_ssid, buf_password, trysecond))
      {
        if (connectWifi(String(buf_ssid), String(buf_password)) == WL_CONNECTED)
        {
          DEBUG_WM(F("connection succeed"));
          return true;
        }
        else
        {
          DEBUG_WM(F("connection failed"));
        }
      }
      else
      {
        DEBUG_WM(F("failed to read EEPROM"));
      }
      itried = true;
    }
  }

  if (!itried)
  {
    DEBUG_WM(F("no saved credential"));
  }

  return false;
}

uint8_t WiFiManager::startConfigPortal(char const *apName)
{

  saved0 = EEPROMisCredentialSaved(0);
  saved1 = EEPROMisCredentialSaved(1);
  saved1 = saved1 && TRY_ALSO_BAD_WIFI_CREDENTIAL;

  if (saved0 || saved1)
  {
    char buf_ssid[EEPROM_SSID_MAX];
    char buf_password[EEPROM_PASSWORD_MAX];
    if (saved0)
    {
      if (EEPROMLoadCredential(buf_ssid, buf_password, 0))
      {
        savedssid0 = String(buf_ssid);
      }
    }
    if (saved1)
    {
      if (EEPROMLoadCredential(buf_ssid, buf_password, 1))
      {
        savedssid1 = String(buf_ssid);
      }
    }
  }

  WiFi.mode(WIFI_AP_STA);
  DEBUG_WM("SET AP STA");

  _apName = apName;

  connect = false;
  setupConfigPortal();

  bool isTimeout = true;
  bool isConnect = false;
  while (_configPortalTimeout == 0 || millis() < _configPortalStart + _configPortalTimeout)
  {
    //DNS
    dnsServer->processNextRequest();
    //HTTP
    server->handleClient();

    if (connect)
    {
      connect = false;
      delay(2000);

      DEBUG_WM(F("connecting to new AP"));

      // using user-provided  _ssid, _pass in place of system-stored ssid and pass
      if (_connect_with_old_pw)
      {
        DEBUG_WM(F("using saved credential"));
        if (tryConnect())
        {
          isConnect = true;
        }
      }
      else
      {
        DEBUG_WM(F("using input credential"));
        if (connectWifi(_ssid, _pass) == WL_CONNECTED)
        {
          isConnect = true;
          EEPROMSaveCredential(_ssid.c_str(), _pass.c_str(), 0);
        }
        else
        {
          isTimeout = false;
          if (TRY_ALSO_BAD_WIFI_CREDENTIAL)
          {
            EEPROMSaveCredential(_ssid.c_str(), _pass.c_str(), 1);
          }
          break;
        }
      }

      if (isConnect)
      {
        //connected
        WiFi.mode(WIFI_STA);
        isTimeout = false;
        break;
      }
    }
    yield();
  }

  server.reset();
  dnsServer.reset();

  if (isTimeout)
  {
    if (tryConnect())
    {
      isConnect = true;
      isTimeout = false;
    }
  }

  if (isConnect)
  {
    return AP_MODE_CONNECT_SUCCESS;
  }
  else if (isTimeout)
  {
    return AP_MODE_CONNECT_TIMEOUT;
  }
  else
  {
    return AP_MODE_CONNECT_FAIL;
  }
}

int WiFiManager::connectWifi(String ssid, String pass)
{

  WiFi.mode(WIFI_STA);
  if (WiFi.status() == WL_CONNECTED)
  {
    if ((ssid == "") || (WiFi.SSID() == ssid))
    {
      DEBUG_WMa("already connected to: ");
      DEBUG_WMb(WiFi.SSID());
      return WL_CONNECTED;
    }
    else
    {
      DEBUG_WMa("disconnecting from: ");
      DEBUG_WMb(WiFi.SSID());
      DEBUG_WMa("use new credential: ");
      DEBUG_WMb(ssid);

      //trying to fix connection in progress hanging
      ETS_UART_INTR_DISABLE();
      wifi_station_disconnect();
      ETS_UART_INTR_ENABLE();
    }
  }

  //check if we have ssid and pass and force those, if not, try with last saved values
  if (ssid != "")
  {
    WiFi.begin(ssid.c_str(), pass.c_str());
    DEBUG_WMa("connecting to: ");
    DEBUG_WMb(ssid.c_str());
  }
  else
  {
    // if (WiFi.SSID())
    // {
    //   DEBUG_WM("using previous credential");
    //   DEBUG_WMa("connecting to: ");
    //   DEBUG_WMb(WiFi.SSID());

    //   //trying to fix connection in progress hanging
    //   ETS_UART_INTR_DISABLE();
    //   wifi_station_disconnect();
    //   ETS_UART_INTR_ENABLE();

    //   WiFi.begin();
    // }
    // else
    // {
    //   DEBUG_WM("no saved credentials");
    // }
  }

  unsigned long t0 = millis();
  int connRes = waitForConnectResult();
  DEBUG_WMa("connection result: ");
  DEBUG_WMb(connRes);

  int counter = 4;
  while (connRes != WL_CONNECTED)
  {
    connRes = WiFi.status();
    DEBUG_WMa("recheck connection result: ");
    DEBUG_WMb(connRes);
    counter--;
    if (counter <= 0)
    {
      break;
    }
    delay(1000);
  }

  DEBUG_WMa("wait time: ");
  DEBUG_WMb(millis() - t0);

  return connRes;
}

uint8_t WiFiManager::waitForConnectResult()
{
  if (_connectTimeout == 0)
  {
    return WiFi.waitForConnectResult();
  }
  else
  {
    DEBUG_WM(F("waiting for connection result with time out"));
    unsigned long start = millis();
    boolean keepConnecting = true;
    uint8_t status;
    while (keepConnecting)
    {
      status = WiFi.status();
      if (millis() > start + _connectTimeout)
      {
        keepConnecting = false;
        DEBUG_WM(F("connection timed out"));
      }
      if (status == WL_CONNECTED || status == WL_CONNECT_FAILED)
      {
        keepConnecting = false;
      }
      delay(100);
    }
    return status;
  }
}

void WiFiManager::resetSettings()
{
  DEBUG_WM(F("settings invalidated"));
  DEBUG_WM(F("THIS MAY CAUSE AP NOT TO START UP PROPERLY. YOU NEED TO COMMENT IT OUT AFTER ERASING THE DATA."));
  WiFi.disconnect(true);
}

void WiFiManager::setDeviceID(char const *deviceID)
{
  _deviceID = String(deviceID);
}

void WiFiManager::setConfigPortalTimeout(unsigned long seconds)
{
  _configPortalTimeout = seconds * 1000;
}

void WiFiManager::setConnectTimeout(unsigned long seconds)
{
  _connectTimeout = seconds * 1000;
}

void WiFiManager::setDebugOutput(boolean debug)
{
  _debug = debug;
}

/** Handle root or redirect to captive portal */
void WiFiManager::handleRoot()
{

  String to_reset = server->arg("reset_pw").c_str();

  if (captivePortal())
  { // If caprive portal redirect instead of displaying the page.
    return;
  }

  String page = FPSTR(HTTP_HEAD);
  page.replace("{v}", "BART Display Configuration");
  // page += FPSTR(HTTP_SCRIPT);
  page += FPSTR(HTTP_STYLE);
  page += FPSTR(HTTP_HEAD_END);
  page += F("<h1>BART Display</h1>");

  boolean printbutton02 = true;

  String saved_ssid_name = "";

  if (to_reset == "y")
  {
    EEPROMClearCredential(0, true);
    printbutton02 = false;
    saved0 = false;
    saved1 = false;
  }
  else
  {
    if (saved0 || saved1)
    {
      if (saved0)
      {
        saved_ssid_name += savedssid0;
      }
      if (saved1)
      {
        if (saved0)
        {
          if (savedssid0 != savedssid1)
          {
            saved_ssid_name += "</br>";
          }
          saved_ssid_name += savedssid1;
        }
      }
    }
    else
    {
      printbutton02 = false;
    }
  }

  if (printbutton02)
  {
    page += FPSTR(HTTP_PORTAL_OPTIONS0);
    page += "</br>";
  }

  page += FPSTR(HTTP_PORTAL_OPTIONS1);

  if (printbutton02)
  {
    page += "</br></br>";
    page += FPSTR(HTTP_PORTAL_OPTIONS2);

    page += "<h3>Saved WiFi Network</h3>";
    page += saved_ssid_name;
  }
  else
  {
    page += "<h3>No WiFi Credentials</h3>Please configure WiFi.";
  }

  if (_deviceID != "")
  {
    page += F("<h3>Device ID</h3>");
    page += _deviceID;
  }

  page += F("<h3>MAC Address</h3>");
  page += WiFi.macAddress();

  page += FPSTR(HTTP_END);

  server->send(200, "text/html", page);
}

/** Wifi config page handler */
void WiFiManager::handleWifi(boolean scan)
{

  String page = FPSTR(HTTP_HEAD);
  page.replace("{v}", "WiFi Configuration");
  page += FPSTR(HTTP_SCRIPT);
  page += FPSTR(HTTP_STYLE);
  page += FPSTR(HTTP_HEAD_END);
  page += F("<h1>WiFi Settings</h1>");

  int n = WiFi.scanNetworks();
  DEBUG_WM(F("scan done"));
  if (n == 0)
  {
    DEBUG_WM(F("no networks found"));
    page += F("no networks found. Refresh to scan again.");
  }
  else
  {

    //sort networks
    int indices[n];
    for (int i = 0; i < n; i++)
    {
      indices[i] = i;
    }

    // old sort
    for (int i = 0; i < n; i++)
    {
      for (int j = i + 1; j < n; j++)
      {
        if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i]))
        {
          std::swap(indices[i], indices[j]);
        }
      }
    }

    // remove duplicates ( must be RSSI sorted )
    if (_removeDuplicateAPs)
    {
      String cssid;
      for (int i = 0; i < n; i++)
      {
        if (indices[i] == -1)
          continue;
        cssid = WiFi.SSID(indices[i]);
        for (int j = i + 1; j < n; j++)
        {
          if (cssid == WiFi.SSID(indices[j]))
          {
            DEBUG_WM("DUP AP: " + WiFi.SSID(indices[j]));
            indices[j] = -1; // set dup aps to index -1
          }
        }
      }
    }

    page += FPSTR(HTTP_FORM_START);
    page += FPSTR(HTTP_FORM_END);

    page += F("<h3>Available Networks</h3>");

    //display networks in page
    for (int i = 0; i < n; i++)
    {
      if (indices[i] == -1)
        continue; // skip dups
      DEBUG_WM(WiFi.SSID(indices[i]));
      DEBUG_WM(WiFi.RSSI(indices[i]));
      int quality = getRSSIasQuality(WiFi.RSSI(indices[i]));

      if (_minimumQuality == -1 || _minimumQuality < quality)
      {
        String item = FPSTR(HTTP_ITEM);
        String rssiQ;
        rssiQ += quality;
        item.replace("{v}", WiFi.SSID(indices[i]));
        item.replace("{r}", rssiQ);
        if (WiFi.encryptionType(indices[i]) != ENC_TYPE_NONE)
        {
          item.replace("{i}", "l");
        }
        else
        {
          item.replace("{i}", "");
        }
        page += item;
        delay(0);
      }
      else
      {
        // DEBUG_WM(F("skipping due to quality"));
      }
    }
  }

  page += FPSTR(HTTP_SCAN_LINK);

  page += FPSTR(HTTP_END);

  server->send(200, "text/html", page);

  DEBUG_WM(F("sent config page"));
}

/** Handle the WLAN save form and redirect to WLAN config page again */
void WiFiManager::handleWifiSave()
{
  //SAVE/connect here
  _ssid = server->arg("s").c_str();
  _pass = server->arg("p").c_str();

  if ((server->arg("to_connect")) == "y")
  {
    _connect_with_old_pw = true;
  }
  else
  {
    _connect_with_old_pw = false;
  }

  String page = FPSTR(HTTP_HEAD);
  page.replace("{v}", "WiFi Credentials Saved");
  page += FPSTR(HTTP_STYLE);
  page += FPSTR(HTTP_HEAD_END);
  page += F("<h1>Connecting...</h1>");
  page += FPSTR(HTTP_SAVED);
  page += FPSTR(HTTP_END);

  server->send(200, "text/html", page);

  DEBUG_WM(F("sent wifi save page"));

  connect = true; //signal ready to connect/reset
}
void WiFiManager::handleNotFound()
{
  if (captivePortal())
  {
    return;
  }
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server->uri();
  message += "\nMethod: ";
  message += (server->method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server->args();
  message += "\n";

  for (uint8_t i = 0; i < server->args(); i++)
  {
    message += " " + server->argName(i) + ": " + server->arg(i) + "\n";
  }
  server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server->sendHeader("Pragma", "no-cache");
  server->sendHeader("Expires", "-1");
  server->send(404, "text/plain", message);
}

boolean WiFiManager::captivePortal()
{
  if (!isIp(server->hostHeader()))
  {
    DEBUG_WM(F("Request redirected to captive portal"));
    server->sendHeader("Location", String("http://") + toStringIp(server->client().localIP()), true);
    server->send(302, "text/plain", "");
    server->client().stop();
    return true;
  }
  return false;
}

template <typename Generic>
void WiFiManager::DEBUG_WM(Generic text)
{
  if (_debug)
  {
    Serial.print("*WM: ");
    Serial.println(text);
  }
}

template <typename Generic>
void WiFiManager::DEBUG_WMa(Generic text)
{
  if (_debug)
  {
    Serial.print("*WM: ");
    Serial.print(text);
  }
}

template <typename Generic>
void WiFiManager::DEBUG_WMb(Generic text)
{
  if (_debug)
  {
    Serial.println(text);
  }
}

int WiFiManager::getRSSIasQuality(int RSSI)
{
  int quality = 0;

  if (RSSI <= -100)
  {
    quality = 0;
  }
  else if (RSSI >= -50)
  {
    quality = 100;
  }
  else
  {
    quality = 2 * (RSSI + 100);
  }
  return quality;
}

/** Is this an IP? */
boolean WiFiManager::isIp(String str)
{
  for (int i = 0; i < str.length(); i++)
  {
    int c = str.charAt(i);
    if (c != '.' && (c < '0' || c > '9'))
    {
      return false;
    }
  }
  return true;
}

/** IP to String? */
String WiFiManager::toStringIp(IPAddress ip)
{
  String res = "";
  for (int i = 0; i < 3; i++)
  {
    res += String((ip >> (8 * i)) & 0xFF) + ".";
  }
  res += String(((ip >> 8 * 3)) & 0xFF);
  return res;
}
