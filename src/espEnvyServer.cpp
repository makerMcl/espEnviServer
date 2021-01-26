#include <Arduino.h>
#include <Streaming.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <WEMOS_SHT3X.h>
#include <MQ2.h>

// maximum value with ESP-12F was 25000 but got some reboots there due to memory overflow
#define LOGBUF_LENGTH 10000 // log buffer size; overwrite default defined by logBuffer.h

#include "universalUi.h"
#include "webUiGenericPlaceHolder.h"
#include "appendBuffer.h"

// global ui instance
UniversalUI ui = UniversalUI("envy");
WiFiUDP ntpUDP;
NTPClient *timeClient = new NTPClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);
SHT3X *sht = new SHT3X(0x44);
MQ2 *mq2 = new MQ2(A0);

unsigned long lpg, methane, smoke, hydrogen;
float temperature, humidity;

AppendBuffer buf = AppendBuffer(2000);
AsyncWebServer *webUiServer = new AsyncWebServer(80);
RefreshState *refreshState = new RefreshState(3);

String placeholderProcessor(const String &var)
{
  if (0 == strcmp_P(var.c_str(), PSTR("APPNAME")))
    return F("envy");
  if (0 == strcmp_P(var.c_str(), PSTR("SHT30_TEMPERATURE")))
    return buf.format(PSTR("%.1f"), temperature);
  if (0 == strcmp_P(var.c_str(), PSTR("SHT30_HUMIDITY")))
    return buf.format(PSTR("%.1f"), humidity);
  if (0 == strcmp_P(var.c_str(), PSTR("MQ2_LPG")))
    return buf.format(PSTR("%lu"), lpg);
  if (0 == strcmp_P(var.c_str(), PSTR("MQ2_METHANE")))
    return buf.format(PSTR("%lu"), methane);
  if (0 == strcmp_P(var.c_str(), PSTR("MQ2_SMOKE")))
    return buf.format(PSTR("%lu"), smoke);
  if (0 == strcmp_P(var.c_str(), PSTR("MQ2_HYDROGEN")))
    return buf.format(PSTR("%lu"), hydrogen);

  if (0 == strcmp_P(var.c_str(), PSTR("REFRESHINDEXTAG")))
    return refreshState->getRefreshTag(buf, "/index.html");
  if (0 == strcmp_P(var.c_str(), PSTR("REFRESHLOGTAG")))
    return refreshState->getRefreshTag(buf, "/log.html");
  if (0 == strcmp_P(var.c_str(), PSTR("REFRESHINDEXLINK")))
    return refreshState->getRefreshLink(buf, "/index.html");
  if (0 == strcmp_P(var.c_str(), PSTR("REFRESHLOGLINK")))
    return refreshState->getRefreshLink(buf, "/log.html");
  return universalUiPlaceholderProcessor(var, buf);
}

void serverSetup()
{
  // Initialize SPIFFS
  // note: it must be SPIFFS, LittleFS is not supported by ESP-FlashTool!
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  if (!SPIFFS.begin())
  {
    ui.logError() << "An Error has occurred while mounting SPIFFS\n";
    return;
  }
  // webUI control page
  webUiServer->on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    refreshState->evaluateRefreshParameters(request);
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/index.html", "text/html", false, placeholderProcessor);
    response->addHeader(F("Cache-Control"), F("no-cache, must-revalidate"));
    response->addHeader(F("Pragma"), F("no-cache"));
    request->send(response);
  });
  webUiServer->on("/log.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    refreshState->evaluateRefreshParameters(request);
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/log.html", "text/html", false, placeholderProcessor);
    response->addHeader(F("Cache-Control"), F("no-cache, must-revalidate"));
    response->addHeader(F("Pragma"), F("no-cache"));
    request->send(response);
  });
  webUiServer->on("/log/memory", HTTP_GET, [](AsyncWebServerRequest *request) {
    ui.logInfo() << F("free heap = ") << ESP.getFreeHeap() << endl;
    request->redirect(F("/log.html"));
  });
  webUiServer->serveStatic("/favicon.ico", SPIFFS, "/favicon.ico").setCacheControl("max-age=86400"); // 1 day = 24*60*60 [sec]
  webUiServer->serveStatic("/style.css", SPIFFS, "/style.css").setCacheControl("max-age=86400");
  webUiServer->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect(F("/index.html"));
  });
  webUiServer->onNotFound([](AsyncWebServerRequest *request) {
    String body = (request->hasParam("body", true)) ? request->getParam("body", true)->value() : String();
    ui.logInfo() << F("unknown uri=") << request->url() << ", method=" << request->method() << ", body=" << body << endl;
  });
#pragma GCC diagnostic pop
  webUiServer->begin();
}

void setup()
{
  Serial.begin(74800);
  ui.setNtpClient(timeClient);
  ui.init(LED_BUILTIN, true, F(__FILE__), F(__TIMESTAMP__));
  ui.setBlink(100, 4900);

  serverSetup();
  mq2->calibrate();
  ui.logInfo() << "initialized MQ2: ratio=" << mq2->getRo() << endl;
  ui.logInfo() << "STARTED\n";
  ui.logInfo() << "temperatur [C]; humidity [%]; lpg [ppm]; co [ppm]; smoke [ppm]\n";
}

void loop()
{
  if (ui.handle() && (millis() % 5000) == 4000)
  {

    lpg = mq2->readLPG();
    methane = mq2->readMethane();
    smoke = mq2->readSmoke();
    hydrogen = mq2->readHydrogen();
    const byte sht2xStatus = sht->get();
    if (0 == sht2xStatus)
    {
      temperature = sht->cTemp;
      humidity = sht->humidity;
      ui.logInfo() << temperature << " ; " << humidity << " ; " << lpg << " ; " << methane << " ; " << smoke << " ; " << hydrogen << " ; " << mq2->readRatio() << endl;
    }
    else
    {
      ui.logError() << "error reading sht30: " << sht2xStatus << endl;
    }
  }
}