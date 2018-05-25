#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>

#include <string.h>

ESP8266WiFiMulti wifiMulti;
boolean connectioWasAlive = true;
boolean internetWasAlive = false;
boolean goodAP = false;
String ssid;
int waitingCount = 0;
int waitingCountTotal = 0;
int errorCountTotal = 0;
uint8_t encryptionType;
int32_t RSSI;
uint8_t* BSSID;
int32_t channel;
bool isHidden;
  
// Settings
const char* myhostname = "wifidbmeter";
String blacklistAP[] = { "MitWiFi" };


void setup() {
  delay(100);
  Serial.begin(115200);
  Serial.println();

  uint32_t realSize = ESP.getFlashChipRealSize();
  uint32_t ideSize = ESP.getFlashChipSize();
  FlashMode_t ideMode = ESP.getFlashChipMode();

  Serial.println();
  Serial.println("Wifi dB Meter booting (version-1.0)");
  Serial.println("------");
  Serial.printf("Flash real id:   %08X\n", ESP.getFlashChipId());
  Serial.printf("Flash real size: %u\n\n", realSize);

  Serial.printf("Flash ide  size: %u\n", ideSize);
  Serial.printf("Flash ide speed: %u\n", ESP.getFlashChipSpeed());
  Serial.printf("Flash ide mode:  %s\n", (ideMode == FM_QIO ? "QIO" : ideMode == FM_QOUT ? "QOUT" : ideMode == FM_DIO ? "DIO" : ideMode == FM_DOUT ? "DOUT" : "UNKNOWN"));
  Serial.println();
  if(ideSize != realSize) {
      Serial.println("Flash Chip configuration wrong!\n");
  } else {
      Serial.println("Flash Chip configuration ok.\n");
  }
  Serial.print("Hostname: ");
  Serial.println(myhostname);
  WiFi.hostname(myhostname);
  Serial.println("------");
  Serial.println("System booted, preparing wifi connection");
  Serial.println();
  Serial.println();
  addOpenNetworks();
  Serial.println();
  Serial.print("Adding \"known\" wifi networks: ");

  // This is our "KNOWN" wifi networks that we add staticly...
  // ----------------------------------------
  wifiMulti.addAP("Private AP", "xxx");
  
  // More "KNOWN" (OPEN) networks that we can use staticly
  wifiMulti.addAP("stogfriwifi");
  wifiMulti.addAP("CommuteNet");
  // ----------------------------------------
  
  Serial.println("Done");
  Serial.println();
  
}

const char* encryptionTypeStr(uint8_t authmode) {
    switch(authmode) {
        case ENC_TYPE_NONE:
            return "Open";
        case ENC_TYPE_WEP:
            return "WEP";
        case ENC_TYPE_TKIP:
            return "WPA-TKIP";
        case ENC_TYPE_CCMP:
            return "WPA2-CCMP";
        case ENC_TYPE_AUTO:
            return "Auto";
        default:
            return "?";
    }
}

const char* httpCodeStr(int response) {
    switch(response) {
        case HTTPC_ERROR_CONNECTION_REFUSED:
            return "Connection lost (refused)";
        case HTTPC_ERROR_SEND_HEADER_FAILED:
            return "Connection lost (header failed)";
        case HTTPC_ERROR_SEND_PAYLOAD_FAILED:
            return "Connection lost (failed payload)";
        case HTTPC_ERROR_NOT_CONNECTED:
            return "Connection lost (disconnected)";
        case HTTPC_ERROR_CONNECTION_LOST:
            return "Connection lost";
        case HTTPC_ERROR_NO_STREAM:
            return "Connection lost (no stream)";
        case HTTPC_ERROR_NO_HTTP_SERVER:
            return "Connection lost (no server)";
        case HTTPC_ERROR_READ_TIMEOUT:
            return "Connection lost (timeout)";
        case HTTP_CODE_FOUND:
            return "HTTP 302 (Redirect)";
        case HTTP_CODE_FORBIDDEN:
            return "HTTP 403 (Forbidden)";
        case HTTP_CODE_NOT_FOUND:
            return "HTTP 404 (Not found)";
        case HTTP_CODE_MOVED_PERMANENTLY:
            return "HTTP 302 (Moved)";
        case HTTP_CODE_INTERNAL_SERVER_ERROR:
            return "HTTP 500 (Error)";
        default:
            return (const char*)response;
    }
}

void addOpenNetworks() {
  goodAP = false;
  int blacklistCount = sizeof(blacklistAP);
  
  Serial.println("Scanning for wifi networks");
  int n = WiFi.scanNetworks(false, true);
  Serial.print(" - Found ");
  Serial.print(n);
  Serial.println(" wifi networks");
  for (int i = 0; i < n; i++) {
    WiFi.getNetworkInfo(i, ssid, encryptionType, RSSI, BSSID, channel, isHidden);
    Serial.printf("  - %s, Ch:%d (%ddBm) Encryption: %s %s\n", ssid.c_str(), channel, RSSI, encryptionTypeStr(encryptionType), isHidden ? "(Hidden)" : "");
  }
  Serial.println();
  Serial.println("Adding \"Open\" wifi networks");
  for (int i = 0; i < n; i++) {
    WiFi.getNetworkInfo(i, ssid, encryptionType, RSSI, BSSID, channel, isHidden);
    if ( encryptionType == ENC_TYPE_NONE ) {
      bool readyToAdd = true;
      for ( int ii = 0; ii < blacklistCount; ++ii ) {
        if ( (String)ssid.c_str() == blacklistAP[ii] ) readyToAdd = false;
      }
      if ( readyToAdd ) {
       Serial.printf(" - %s, Ch:%d (%ddBm)\n", ssid.c_str(), channel, RSSI);
       wifiMulti.addAP(ssid.c_str());
       goodAP = true;
      } else {
        Serial.printf(" - %s, Ch:%d (%ddBm) (Not added, BLACKLISTED)\n", ssid.c_str(), channel, RSSI);
      }
    }
  }
  if ( goodAP != true ) {
    Serial.println(" - No open wifi networks where found, skipping");
  }
}

void giveOp() {
  WiFi.disconnect();
}

void monitorWiFi()
{
  if (wifiMulti.run() != WL_CONNECTED)
  {
    if (waitingCountTotal==6) {
       Serial.println();
       giveOp();
       addOpenNetworks();
       waitingCountTotal=0;
       waitingCount=0;
       connectioWasAlive = true;
       Serial.println();
    }
    
    if (connectioWasAlive == true)
    {
      connectioWasAlive = false;
      if (waitingCountTotal < 10) Serial.print("ZzzzzzZzz ");
    }
    
    Serial.print(".");
    waitingCount = waitingCount+1;
     
    if ( waitingCount == 10) {
      Serial.println();
      waitingCount=0;
      waitingCountTotal=waitingCountTotal+1;
      if (waitingCountTotal < 6) Serial.print("ZzzzzzZzz ");
    }
    delay(1000);
  }
  else if (connectioWasAlive == false)
  {
    connectioWasAlive = true;
    Serial.println();
    Serial.println();
    Serial.print("Connected to: ");
    Serial.print(WiFi.SSID());
    Serial.print(" ");
    Serial.print(WiFi.RSSI());
    Serial.print("dBm ( ");
    Serial.print(WiFi.localIP());
    Serial.println(" )");
  }
}

void loop() {
  
  monitorWiFi();
  
  if ( connectioWasAlive ) {
    //CONNECTED
    HTTPClient http;
    http.begin("http://wifidbmeter.darknet.dk/dbmeter.php"); // Maybe change to IP for no DNS usage ?
    int httpCode = http.GET();
    if(httpCode > 0 && httpCode == HTTP_CODE_OK) {
     String payload = http.getString();
     // For now we dont send anything, we just recieve a unix timestamp to see that we have a connection to the server ...
     Serial.print(" - Unixtime from server: ");
     Serial.println(payload);
    } else {
      errorCountTotal = errorCountTotal+1;
      Serial.print(" - No internet access: ");
      Serial.println(httpCodeStr(httpCode));
      if (errorCountTotal==10) {
        Serial.println();
        giveOp();
        addOpenNetworks();
        errorCountTotal=0;
        connectioWasAlive = true;
        Serial.println();
      }
    }
    delay(2000);
  }
  
}
