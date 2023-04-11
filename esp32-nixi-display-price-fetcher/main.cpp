#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJSON.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <time.h>
#include <string.h>

// WiFi Settings
const char *ssid     = "<SSID>";
const char *password = "<PASSWORD>";

const char* root_ca= \
"-----BEGIN CERTIFICATE-----\n"\
"MIICGzCCAaGgAwIBAgIQQdKd0XLq7qeAwSxs6S+HUjAKBggqhkjOPQQDAzBPMQsw\n"\
"CQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJuZXQgU2VjdXJpdHkgUmVzZWFyY2gg\n"\
"R3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBYMjAeFw0yMDA5MDQwMDAwMDBaFw00\n"\
"MDA5MTcxNjAwMDBaME8xCzAJBgNVBAYTAlVTMSkwJwYDVQQKEyBJbnRlcm5ldCBT\n"\
"ZWN1cml0eSBSZXNlYXJjaCBHcm91cDEVMBMGA1UEAxMMSVNSRyBSb290IFgyMHYw\n"\
"EAYHKoZIzj0CAQYFK4EEACIDYgAEzZvVn4CDCuwJSvMWSj5cz3es3mcFDR0HttwW\n"\
"+1qLFNvicWDEukWVEYmO6gbf9yoWHKS5xcUy4APgHoIYOIvXRdgKam7mAHf7AlF9\n"\
"ItgKbppbd9/w+kHsOdx1ymgHDB/qo0IwQDAOBgNVHQ8BAf8EBAMCAQYwDwYDVR0T\n"\
"AQH/BAUwAwEB/zAdBgNVHQ4EFgQUfEKWrt5LSDv6kviejM9ti6lyN5UwCgYIKoZI\n"\
"zj0EAwMDaAAwZQIwe3lORlCEwkSHRhtFcP9Ymd70/aTSVaYgLXTWNLxBo1BfASdW\n"\
"tL4ndQavEi51mI38AjEAi/V3bNTIZargCyzuFJ0nN6T5U6VR5CmD1/iQMVtCnwr1\n"\
"/q4AaOeMSQ+2b1tbFfLn\n"\
"-----END CERTIFICATE-----\n";


// URL
// const char *base_URL = "api.porssisahko.net";
// const char *endpoint = "/v1/price.json";
const char *base_URL = "https://api.porssisahko.net/v1/price.json";

const char *ntpServer = "europe.pool.ntp.org";
const long gmtOffset_sec = 7200;
const int daylightOffset_sec = 3600;

// DynamicJsonDocument doc(200);
StaticJsonDocument<200> doc;

// Nixietube display
// Pin connected to ST_CP of 74HC595
#define latchPin 26
// Pin connected to SH_CP of 74HC595
#define clockPin 27
//Pin connected to DS of 74HC595
#define dataPin 14

const int byteSize = 32;
const int digits = 3;
int outPut[32];



void startWiFi()
{
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
}

void writeD(int val) {
    digitalWrite(dataPin, val);
    digitalWrite(clockPin, 1);
    digitalWrite(clockPin, 0);
};

void shuffle() {
  digitalWrite(latchPin, LOW);
  // shuffle 5 times
  for (int t = 0; t < 5; t++) {
    
    for (int end = 0; end < 10; end++){
      for (int i = 32; i > 0; i--)
      {
        if (i % 10 == end)
        {
          writeD(1);
        }
        else
        {
          writeD(0);
        }
      }
      digitalWrite(latchPin, HIGH);
      delay(100);
    }
  }
}

void writeDigits(int price) {
  int priceArr[digits];
  priceArr[2] = (price / 100) % 10 ;
  priceArr[1] = (price / 10) % 10;
  priceArr[0] = price % 10;
  digitalWrite(latchPin, LOW);
  for(int bit = byteSize; bit > 0; bit --) {
        if ( bit <= 30 && bit > 20 && priceArr[0] == bit % 10) {
            writeD(1);
            continue;
        }
        if ( bit <= 20 && bit > 10 && priceArr[1] == bit % 10) {
            writeD(1);
            continue;
        }
        if ( bit <=10 && priceArr[2] == bit % 10) {
            writeD(1);
            continue;
        }
        if ( bit == 31) {
            writeD(1);
            continue;
        }
        writeD(0);
    }
  digitalWrite(latchPin, HIGH);



}


void setup()
{
  Serial.begin(115200);
  startWiFi();
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
}

char newHour[3];
char date[12];
String previousHour_str;

void loop()
{
  // Update time every loop
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to get time");
  }

  // format date and hour to variables
  strftime(newHour, 3, "%H", &timeinfo);
  strftime(date, 12, "%Y-%m-%d", &timeinfo);

  String newHour_str = newHour;

  // if hour has changed
  if (newHour_str != previousHour_str)
  {
    Serial.println();
    Serial.println("Hour has changed, fetching new price data...");
    // concatenate URL
    std::string parameters = std::string("?date=") + date + std::string("&hour=") + newHour;
    std::string URL = base_URL + parameters;
    // Serial.println(URL.c_str());

    HTTPClient http;
    http.begin(URL.c_str(), root_ca);
    int responseCode = http.GET();
    if(responseCode > 0) {
      String payload = http.getString();
      // Serial.println(responseCode);
      // Serial.println(payload);

      // Deserialize the JSON document
      DeserializationError error = deserializeJson(doc, payload);

      // Test if parsing succeeds.
      if (error)
      {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }

      int price = doc["price"];
      shuffle();
      writeDigits(price);

      Serial.print("Price + VAT = ");
      Serial.print(price);
      Serial.println("c/kWh");
    } else {
      Serial.println("Error on HTTPS request");
    }

    http.end();
    previousHour_str = newHour_str;
  }
  // delay(60000);
}
