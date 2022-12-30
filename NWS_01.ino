/*

  pinouts

  ESP32     display
  3v3         VCC
  GND         GND
  5           TFT_CS
  25          TFT_RESET
  2           TFT_DC
  23          MOSI
  18          SCK
  3v3         LED
  19          MISO
  12          T_CS
  26          TFT LED
*/

#include "Adafruit_GFX.h"       // https://github.com/adafruit/Adafruit-GFX-Library
#include "Adafruit_ILI9341.h"   // https://github.com/adafruit/Adafruit_ILI9341
#include <WiFi.h>               // Use in additional boards manager https://dl.espressif.com/dl/package_esp32_index.json
#include <WebServer.h>          // Use in additional boards manager https://dl.espressif.com/dl/package_esp32_index.json
#include <HTTPClient.h>         // Use in additional boards manager https://dl.espressif.com/dl/package_esp32_index.json
#include "time.h"               // https://github.com/lattera/glibc/blob/master/time/time.h
#include "elapsedMillis.h"      // https://github.com/pfeerick/elapsedMillis/blob/master/elapsedMillis.h

#include <ArduinoJson.h>        // https://github.com/bblanchon/ArduinoJson
#include "fonts\FreeSans18pt7b.h"   // fonts that deliver with Adafruit_GFX
#include "fonts\FreeSans12pt7b.h"   // fonts that deliver with Adafruit_GFX
#include "fonts\FreeSans9pt7b.h"    // fonts that deliver with Adafruit_GFX
#include "FlickerFreePrint.h"       // https://github.com/KrisKasprzak/FlickerFreePrint

// --------------------------------------------
// enter your GMT Time offset
#define GMT_OFFSET -6

// enter your local intranet SSID and password
#define HOME_SSID "HomeSweetHome"
#define PASSWORD "1234567899"

// enter your lat / long corrdinates here
//#define LAT "34.6993"
//#define LON "-86.7483"

// garden of the gods
//#define LAT "38.8784"
//#define LON "-104.8698"

// the bean in Chicago
#define LAT "41.8827"
#define LON "-87.6233"
//----------------------------------------------------


#define NWS_SITE_POINTS "https://api.weather.gov/points/"
#define NWS_SITE_DAYFORECAST "https://api.weather.gov/gridpoints/"
#define NPT_TIME_SERVER "pool.ntp.org"

// tweek to get different colors for the printed temp values (in deg c)
#define MINTEMP  0
#define MAXTEMP  30

// pin defines
#define T_CS      12
#define T_IRQ     27
#define TFT_DC    2
#define TFT_CS    5
#define TFT_RST   25
#define PIN_LED   26
#define PIN_ITS   15

// fonts for better data display
#define FONT_TIME  FreeSans18pt7b
#define FONT_TITLE  FreeSans12pt7b            // font for menus
#define FONT_TEXT  FreeSans9pt7b

// color definition values
// http://www.barth-dev.de/online/rgb565-color-picker/
#define C_WHITE       0xFFFF
#define C_BLACK       0x0000
#define C_GREY        0xC618
#define C_BLUE        0x001F
#define C_RED         0xF800
#define C_GREEN       0x07E0
#define C_CYAN        0x07FF
#define C_MAGENTA     0xF81F
#define C_YELLOW      0xFFE0
#define C_TEAL      0x0438
#define C_ORANGE        0xFD20
#define C_PINK          0xF81F
#define C_PURPLE    0x801F
#define C_LTGREY        0xE71C
#define C_LTBLUE    0x73DF
#define C_LTRED         0xFBAE
#define C_LTGREEN       0x7FEE
#define C_LTCYAN    0x77BF
#define C_LTMAGENTA     0xFBB7
#define C_LTYELLOW      0xF7EE
#define C_LTTEAL    0x77FE
#define C_LTORANGE      0xFDEE
#define C_LTPINK        0xFBBA
#define C_LTPURPLE    0xD3BF
#define C_DKGREY        0x4A49
#define C_DKBLUE        0x0812
#define C_DKRED         0x9000
#define C_DKGREEN       0x04A3
#define C_DKCYAN        0x0372
#define C_DKMAGENTA     0x900B
#define C_DKYELLOW      0x94A0
#define C_DKTEAL    0x0452
#define C_DKORANGE    0x92E0
#define C_DKPINK        0x9009
#define C_DKPURPLE      0x8012
#define C_MDGREY        0x7BCF
#define C_MDBLUE    0x1016
#define C_MDRED       0xB000
#define C_MDGREEN     0x0584
#define C_MDCYAN      0x04B6
#define C_MDMAGENTA   0xB010
#define C_MDYELLOW      0xAD80
#define C_MDTEAL    0x0594
#define C_MDORANGE      0xB340
#define C_MDPINK        0xB00E
#define C_MDPURPLE    0x8816

byte NWSTemp[14];
const char *NWSForeCast[14];
const char *NWSTime[14];
byte TimeOffset = 0;

String JSONData;
String WeatherServerPath;
String DayForecastPath;

bool found = false;

int Row;
char buf[32];

int httpResponseCode = 0;
int x;
int daylightOffset_sec = 3600;
int gridX = 0, gridY = 0;
int i;
uint16_t TextColor;
const char *gridId;
byte Start = 0;


unsigned long  gmtOffset_sec = GMT_OFFSET * 3600;
byte Hour, Minute, Second;

IPAddress Actual_IP;
IPAddress PR_IP(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress ip;

WebServer server(80);

DynamicJsonDocument doc(24000);
DeserializationError error;

struct tm timeinfo;

elapsedSeconds CurrentTime;
elapsedSeconds DisplayDataTime;
elapsedMillis UpdateTime;
elapsedSeconds GetTimeData;
elapsedSeconds GetWeatherData;

Adafruit_ILI9341 Display = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

FlickerFreePrint<Adafruit_ILI9341> Timeff(&Display, C_WHITE, C_BLACK);

void setup() {

  Serial.begin(230400);

  Serial.println("Starting");

  Display.begin();
  Display.setTextWrap(false);
  Display.setRotation(3);
  Display.fillScreen(C_BLACK);

  disableCore0WDT();
  disableCore1WDT();

  Serial.println("starting server");

  WiFi.begin(HOME_SSID, PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Actual_IP = WiFi.localIP();

  Serial.println("CONNECTED");
  printWifiStatus();
  delay(100);

  // config internet time
  configTime(gmtOffset_sec, daylightOffset_sec, NPT_TIME_SERVER);

  // now get the time
  GetInternetTime();

  // optional to print time to serial monitor
  printLocalTime();

  // get the national weather service grid and ID for your desired location
  GetGrid();

  // get the national weather service 7-day forecase with the grid and ID
  GetDayForcast();

  // display stuff on your ILI9341
  DisplayHeader();
  DisplayData();

  // restart the timers
  UpdateTime = 0;
  GetTimeData = 0;
  GetWeatherData = 0;
  DisplayDataTime = 0;

}

void loop() {

  // update The time display every 1 second
  if (UpdateTime > 1000) {
    UpdateTime = 0;
    if (!found) {
      GetDayForcast();
    }
    DisplayHeader();
  }

  // udate the time every day, once we have internet time we'll use millis() to keep it updated
  if (GetTimeData > 86400) { // 86400 reset the time every day
    GetTimeData = 0;
    GetInternetTime();
  }

  if (DisplayDataTime > 5) {
    DisplayDataTime = 0;
    DisplayData();
  }

  // update the weather forecase every hour
  if (GetWeatherData > 3600) { // 3600 check every hour
    GetDayForcast();
    GetWeatherData = 0;
    // blank out all previous data
    Display.fillRect(0, 49, 320, 188, C_BLACK);
    DisplayData();
  }

}

void GetInternetTime() {
  getLocalTime(&timeinfo);
  // here's where we convert internet time to an unsigned long, then use the ellapsed counter to keep it up to date
  // if the ESP restarts, we'll start with the the internet time
  // note CurrentTime is managed by the ellapsed second counter
  CurrentTime = (timeinfo.tm_hour * 3600) + (timeinfo.tm_min * 60) + timeinfo.tm_sec;
}

void DisplayHeader() {

  // tear apart the CurrentTime (unsigned long into hh:mm:ss)
  Second = CurrentTime % 60;
  Minute = ((CurrentTime - Second) / 60) % 60;
  Hour = CurrentTime / 3600;
  TimeOffset = 0;

  // if just past midnight, blank out date and redraw
  if ((Hour == 0) && (Minute < 2)) {
    Display.fillRect(0, 0, 320, 50, C_BLACK);
  }

  // backing out 12 to get to US designation
  if (Hour > 12) {
    TimeOffset = 12;
  }

  // specail case if hour is 0 we show 12:22 as opposed (non-military time)
  if (Hour == 0) {
    Hour  = 24;
  }

  //sprintf(buf, "%d:%02d", Hour - TimeOffset, Minute);

  // if you want to display seconds
  sprintf(buf, "%d:%02d:%02d", Hour - TimeOffset, Minute, Second);

  // set font, use flicker free lib to draw the text
  Display.setFont(&FONT_TIME);
  Display.setCursor(5, 30);
  Timeff.setTextColor(C_WHITE, C_BLACK);
  Timeff.print(buf);

  // draw the date
  Display.setFont(&FONT_TITLE);
  Display.setTextColor(C_ORANGE);
  Display.setCursor(130, 25);
  Display.print(&timeinfo, "%A");
  Display.print(", ");
  Display.print(timeinfo.tm_mon + 1);
  Display.print("/");
  Display.print(timeinfo.tm_mday);

}

void DisplayData() {

  Display.setFont(&FONT_TEXT);
  Display.setTextColor(C_WHITE, C_BLACK);

  Row = 50;

  // display can't show all 14 entries, just drawing first 5
  // here's where you add you desired look and feel
  // my production version paints and icon depending on forecast
  if (Start > 8) {
    Start = 0;
  }
  Display.fillRect(0, 35, 320, 205, C_BLACK);
  for (i = Start; i < Start + 5; i++) {
    //Serial.print("i "); Serial.print(i);
    //Serial.print(", Day "); Serial.print(NWSTime[i]);
    //Serial.print(", Forecast: "); Serial.print(NWSForeCast[i]);
    //Serial.print(", Temp: "); Serial.println(NWSTemp[i]);

    // draw for even only
    if (i % 2 == 0) {
      Display.drawLine(0, Row - 15, 320, Row - 15, C_DKGREY);
    }

    Row += 2;

    Display.setFont(&FONT_TEXT);
    Display.setTextColor(C_WHITE);
    Display.setCursor(10, Row);
    Display.print(NWSTime[i]);

    Display.setTextColor(C_WHITE);
    Display.setCursor(10, Row += 16);
    Display.print(NWSForeCast[i]);

    Display.setFont(&FONT_TIME);
    // may be a bit over the top for an example but cold temps will be drawn in blue, hot in red
    TextColor = GetColor((NWSTemp[i] - 32) / 1.8);
    Display.setTextColor(TextColor);
    Display.setCursor(270, Row);
    Display.print(NWSTemp[i]);

    Row += 21;

  }
  Start++;

}

void printLocalTime() {

  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }

  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");

  Serial.print("Day of week: ");
  Serial.println(&timeinfo, "%A");

  Serial.print("Month: ");
  Serial.println(&timeinfo, "%B");

  Serial.print("Day of Month: ");
  Serial.println(&timeinfo, "%d");

  Serial.print("Year: ");
  Serial.println(&timeinfo, "%Y");
  Serial.print("Hour: ");
  Serial.println(&timeinfo, "%H");
  Serial.print("Hour (12 hour format): ");
  Serial.println(&timeinfo, "%I");
  Serial.print("Minute: ");
  Serial.println(&timeinfo, "%M");
  Serial.print("Second: ");
  Serial.println(&timeinfo, "%S");

  Serial.println("Time variables");
  char timeHour[3];
  strftime(timeHour, 3, "%H", &timeinfo);
  Serial.println(timeHour);
  char timeWeekDay[10];
  strftime(timeWeekDay, 10, "%A", &timeinfo);
  Serial.println(timeWeekDay);
  Serial.println();

}

void printWifiStatus() {

  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
  // print where to go in a browser:
  Serial.print("Open http://");
  Serial.println(ip);

}

void GetDayForcast() {

  HTTPClient http;
  http.setTimeout(6000);

  found = false;

  for (i = 0; i < 14; i++) {
    NWSTime[i] = "No service";
    NWSTemp[i] = 0;
    NWSForeCast[i] = "No service";
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WL_CONNECTED ");

    http.begin(DayForecastPath.c_str());
    httpResponseCode = http.GET();


    Serial.print("HTTP to get day data: "); Serial.println(DayForecastPath);

    if (httpResponseCode > 200) {
      Serial.println("Service unavailable.");
      return;
    }
    if (httpResponseCode > 0) {
      found = true;
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      JSONData = http.getString();
      Serial.println(JSONData);
      error = deserializeJson(doc, JSONData);
      Serial.print(F("deserializeJson() return code: "));
      Serial.println(error.f_str());

      for (i = 0; i < 14; i++) {
        NWSTime[i] =  doc["properties"]["periods"][ i ]["name"];
        NWSTemp[i] = doc["properties"]["periods"][ i ]["temperature"];
        NWSForeCast[i] = doc["properties"]["periods"][ i]["shortForecast"];
      }

      http.end();

    }
  }
  else {
    Serial.println("no wifi connection");
  }

}


void GetGrid() {

  Serial.println("Getting gridId... ");

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WL_CONNECTED ");

    HTTPClient http;
    http.setTimeout(6000);
    WeatherServerPath = NWS_SITE_POINTS;
    WeatherServerPath = WeatherServerPath + LAT;
    WeatherServerPath = WeatherServerPath + ",";
    WeatherServerPath = WeatherServerPath + LON;

    Serial.print("HTTP to get gridId: "); Serial.println(WeatherServerPath);

    http.begin(WeatherServerPath.c_str());
    httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      JSONData = http.getString();
      Serial.println(JSONData);
      error = deserializeJson(doc, JSONData);
      Serial.print(F("deserializeJson() return code: "));
      Serial.println(error.f_str());
      gridId = doc["properties"]["gridId"];
      gridX = doc["properties"]["gridX"];
      gridY = doc["properties"]["gridY"];

      DayForecastPath = NWS_SITE_DAYFORECAST;
      DayForecastPath = DayForecastPath + String(gridId);
      DayForecastPath = DayForecastPath + "/";
      DayForecastPath = DayForecastPath + gridX;
      DayForecastPath = DayForecastPath + ",";
      DayForecastPath = DayForecastPath + gridY;
      DayForecastPath = DayForecastPath + "/forecast";

      http.end();
      delay(100);
    }
  }
  else {
    Serial.println("no wifi connection");
  }
  Serial.println("Done getting GridID.");

}

unsigned int GetColor(float TempC) {
  byte red, green, blue;

  float  b = (MAXTEMP + MINTEMP) / 2.0;
  float  a = b - (MAXTEMP - MINTEMP) * .15 ;  // how fast color changes from cold to mid
  float   c = b + (MAXTEMP - MINTEMP) * .15 ; // how fast color changes from mid to high

  red = constrain(255.0 / (c - b) * TempC - ((b * 255.0) / (c - b)), 0, 255);
  if (TempC < a) {
    green = constrain(255.0 / (a - MINTEMP) * TempC - (255.0 * MINTEMP) / (a - MINTEMP), 0, 255);
  }
  else if (TempC > c) {
    green = constrain(255.0 / (c - MAXTEMP) * TempC - (MAXTEMP * 255.0) / (c - MAXTEMP), 0, 255);
  }
  else {
    green = 255;
  }
  if (TempC > (MAXTEMP )) {
    blue = constrain((TempC - MAXTEMP) * 55.0, 0, 255);
  }
  else {
    blue = constrain(255.0 / (a - b) * TempC - ((a * 255.0) / (a - b)), 0, 255);
  }

  return Display.color565(red, green, blue);
}


// end of code
