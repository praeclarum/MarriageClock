#include <vector>
#include <string.h>
#include <WiFi.h>
#include "MyHTTPClient.h"
#include <esp_wifi.h>
#include <esp_system.h>
#include "Display.h"
#include "timelib.h"

using namespace std;


#define SLEEP_SECONDS (10LL * 60LL)


void showTime() {
  time_t nowSecs = time(nullptr);
  struct tm timeinfo;
  gmtime_r(&nowSecs, &timeinfo);
  Serial.print(F("Current time: "));
  Serial.print(asctime(&timeinfo));
//  Serial.println(timeinfo.tm_year);
}

static int stricmp(char const *a, char const *b) {
    for (;; a++, b++) {
        int d = tolower((unsigned char)*a) - tolower((unsigned char)*b);
        if (d != 0 || !*a)
            return d;
    }
}

bool updateTimeFromDateString(const String &date)
{
  Serial.printf("DATE: %s\n", date.c_str());

  struct tm timeinfo = {0};

  auto s = (char*)date.c_str();
  auto end = s + date.length();
  auto b = s;
  
  while (b < end && !isdigit(*b))
    b++;
  auto e = b + 1;
  while (e < end && isdigit(*e))
    e++;
  *e = 0;
  timeinfo.tm_mday = atoi(b);
//  Serial.printf("DAY: %d\n", timeinfo.tm_mday);

  b = e + 1;
  while (b < end && !isalpha(*b))
    b++;
  e = b + 1;
  while (e < end && isalpha(*e))
    e++;
  *e = 0;
  if (stricmp(b, "feb") == 0)
    timeinfo.tm_mon = 1;
  else if (stricmp(b, "mar") == 0)
    timeinfo.tm_mon = 2;
  else if (stricmp(b, "apr") == 0)
    timeinfo.tm_mon = 3;
  else if (stricmp(b, "may") == 0)
    timeinfo.tm_mon = 4;
  else if (stricmp(b, "jun") == 0)
    timeinfo.tm_mon = 5;
  else if (stricmp(b, "jul") == 0)
    timeinfo.tm_mon = 6;
  else if (stricmp(b, "aug") == 0)
    timeinfo.tm_mon = 7;
  else if (stricmp(b, "sep") == 0)
    timeinfo.tm_mon = 8;
  else if (stricmp(b, "oct") == 0)
    timeinfo.tm_mon = 9;
  else if (stricmp(b, "nov") == 0)
    timeinfo.tm_mon = 10;
  else if (stricmp(b, "dec") == 0)
    timeinfo.tm_mon = 11;
//  Serial.printf("MONTH: %d\n", timeinfo.tm_mon);

  b = e + 1;
  while (b < end && !isdigit(*b))
    b++;
  e = b + 1;
  while (e < end && isdigit(*e))
    e++;
  *e = 0;
  timeinfo.tm_year = atoi(b) - 1900;
//  Serial.printf("YEAR: %d\n", timeinfo.tm_year);

  b = e + 1;
  while (b < end && !isdigit(*b))
    b++;
  e = b + 1;
  while (e < end && isdigit(*e))
    e++;
  *e = 0;
  timeinfo.tm_hour = atoi(b);

  b = e + 1;
  while (b < end && !isdigit(*b))
    b++;
  e = b + 1;
  while (e < end && isdigit(*e))
    e++;
  *e = 0;
  timeinfo.tm_min = atoi(b);

    b = e + 1;
  while (b < end && !isdigit(*b))
    b++;
  e = b + 1;
  while (e < end && isdigit(*e))
    e++;
  *e = 0;
  timeinfo.tm_sec = atoi(b);

//  strptime (s, "%a, %d %m %Y %H:%M:%S GMT", &timeinfo);
  Serial.print(F("Updating to server time: "));
  Serial.print(asctime(&timeinfo));

  auto nowSec = mktime(&timeinfo);

  timeval tv { nowSec, 0 };
  timezone tz = {0};
  settimeofday(&tv, &tz);
  return true;
}

bool updateTimeFromHttpResponseHeader(const String &url) {
  MyHTTPClient http;
  http.begin(url.c_str());
//  http.begin("http://bitrise.io");
  int httpCode = http.GET();

  auto date = http.getDate();
  if (date.length () > 0) {
    return updateTimeFromDateString (date);
  }
  
  auto location = http.getLocation();
  if (httpCode / 100 == 3 && location.length() > 0) {
    Serial.printf("Redirect to: %s\n", location.c_str());
    return updateTimeFromHttpResponseHeader (location);
  }
  
  return false;
}

struct Network {
  String ssid;
  int32_t channel;
  uint8_t bssid[6];
};

bool updateTimeFromNetwork(struct Network &network)
{
  Serial.printf("Connecting to: %s c=%d\n", network.ssid.c_str(), network.channel);
  unsigned long connectTimeout = 20 * 1000;
  
  WiFi.begin(network.ssid.c_str(), nullptr, network.channel, network.bssid);//, bestNetwork.passphrase, bestChannel, bestBSSID);  
//  WiFi.begin("testap", "testpw");
  
  auto status = WiFi.status();
  
  auto startTime = millis();
  // wait for connection, fail, or timeout  
  while(status != WL_CONNECTED && status != WL_NO_SSID_AVAIL && status != WL_CONNECT_FAILED && (millis() - startTime) <= connectTimeout) {
      delay(200);
      Serial.printf(".");
      status = WiFi.status();
  }
  Serial.printf("\n%s status = %d\n", network.ssid.c_str(), (int)status);
  String url = "http://neverssl.com";
  if (status == 3) {
    return updateTimeFromHttpResponseHeader (url);
  }
  //WiFi.disconnect();
  return false;
}

void updateTime() {
  Serial.printf("Scanning for networks\n");
  Serial.println( WiFi.softAPmacAddress() );
  WiFi.mode(WIFI_STA);
  auto numAP = WiFi.scanNetworks();
  vector<Network> openNets;
  Serial.printf("%d networks\n", (int)numAP);
  for (int i = 0; i < numAP; i++) {
    Network n;
    n.ssid = WiFi.SSID(i);
    n.channel = WiFi.channel(i);
    memcpy(n.bssid, WiFi.BSSID(i), 6);
    auto et = WiFi.encryptionType(i);
    Serial.printf("  %d: e=%d c=%d %s\n", i, (int)et, n.channel, n.ssid.c_str());
    if (et == WIFI_AUTH_OPEN && n.ssid.length() > 0) {
      openNets.push_back(n);
    }
  }
  Serial.printf("%d open networks\n", (int)openNets.size());  
  
  for (size_t i = 0; i < openNets.size(); i++) {
    if (updateTimeFromNetwork (openNets[i])) {
      break;
    }
  }
  showTime();
}


bool hasTime()
{
  time_t nowSecs = time(nullptr);
  struct tm timeinfo;
  gmtime_r(&nowSecs, &timeinfo);
  return timeinfo.tm_year > 100;
}

void setup() {
  Serial.begin(115200);

  displayBegin();

  pinMode(13, OUTPUT);
  
  digitalWrite(13, 1);
  while (!hasTime()) {
    displayText("Time...");
    updateTime();
    if (!hasTime()) {
      displayText("Time :-(");
      delay(60*1000);
    }
  }
  digitalWrite(13, 0);
}

// From https://www.epochconverter.com
const time_t weddingEpoch = 1566691200;

// TEST - wedding on previous year
//const time_t weddingEpoch = 967161600;

void loop() {
  showTime();

  auto now = time(nullptr);
  reltime rel = {0};
  diff_times(weddingEpoch, now, &rel);
  displayTime(rel);
  
  digitalWrite(13, 1);
  delay(1000);
  digitalWrite(13, 0);
  
  esp_deep_sleep(1000000LL * SLEEP_SECONDS);
}
