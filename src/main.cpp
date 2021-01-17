#include <WiFi.h>
#include <ArduinoOTA.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

AsyncWebServer webServer(80);
AsyncWebSocket webSocketServer("/ws");

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

Preferences preferences;

const char *wifiSSID = "";
const char *wifiPassword = "";

unsigned long BOOT_TIME;

//Debugging LED
const int BLUE_LED = 21;
const int RED_LED = 23;
const int WHITE_LED = 15;
//

const unsigned int IN1 = 26; //GPIO13 - IN1;
const unsigned int IN2 = 25; //GPIO14 - IN2;
const unsigned int IN3 = 33; //GPIO15 - IN3;
const unsigned int IN4 = 32; //GPIO4  - IN4

const int LEFT_MOTOR = 1;
const int RIGHT_MOTOR = 2;
const int BOTH_MOTORS = 3;

long STOP_AFTER_RIGHT = 0;
long STOP_AFTER_MILLIS_RIGHT;

long STOP_AFTER_LEFT = 0;
long STOP_AFTER_MILLIS_LEFT;

bool TIMER_MINUTE_ACTIVE = false;

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

void rebootESP()
{
  ESP.restart();
}

StaticJsonDocument<384> initialInfoMessage()
{
  StaticJsonDocument<384> doc;

  JsonObject data = doc.createNestedObject("data");
  data["type"] = "initial";

  JsonObject data_attributes = data.createNestedObject("attributes");
  data_attributes["boottime"] = BOOT_TIME;
  data_attributes["rebootreason"] = "Hi";
  data_attributes["rebootreasontext"] = "Text";
  data_attributes["rebootsource"] = "Source";
  data_attributes["freeheap"] = esp_get_free_heap_size();
  data_attributes["rightlastfeedtime"] = "";
  data_attributes["rightlastfeedduration"] = "";
  data_attributes["leftlastfeedtime"] = "";
  data_attributes["leftlastfeedduration"] = "";

  JsonObject data_attributes_timer1 = data_attributes.createNestedObject("timer1");
  data_attributes_timer1["active"] = preferences.getBool("timerActive");
  data_attributes_timer1["timestamp"] = preferences.getInt("timerTimestamp");
  data_attributes_timer1["seconds"] = preferences.getInt("timerSeconds");

  return doc;
}

void saveTimer(boolean active, int timestamp, int forSeconds)
{
  preferences.putInt("timerTimestamp", timestamp);

  preferences.putInt("timerSeconds", forSeconds);
  preferences.putBool("timerActive", active);
}

void stopMotor(int motor)
{
  if (motor == RIGHT_MOTOR or motor == BOTH_MOTORS)
  {
    digitalWrite(RED_LED, LOW);

    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);

    STOP_AFTER_RIGHT = 0;
  }

  if (motor == LEFT_MOTOR or motor == BOTH_MOTORS)
  {
    digitalWrite(BLUE_LED, LOW);

    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);

    STOP_AFTER_LEFT = 0;
  }

  digitalWrite(WHITE_LED, LOW);
}

void motorForwards(int motor, int seconds)
{
  if (motor == RIGHT_MOTOR)
  {
    digitalWrite(RED_LED, HIGH);

    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);

    if (seconds > 0)
    {
      STOP_AFTER_MILLIS_RIGHT = millis();
      STOP_AFTER_RIGHT = seconds;
    }
    else
    {
      STOP_AFTER_RIGHT = 0;
    }
  }
  else if (motor == LEFT_MOTOR)
  {
    digitalWrite(BLUE_LED, HIGH);

    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);

    if (seconds > 0)
    {
      STOP_AFTER_MILLIS_LEFT = millis();
      STOP_AFTER_LEFT = seconds;
    }
    else
    {
      STOP_AFTER_LEFT = 0;
    }
  }
}

void startMotor(int motor, int seconds)
{
  if (motor == 1 or motor == 3)
  {
    motorForwards(LEFT_MOTOR, seconds);
  }
  if (motor == 2 or motor == 3)
  {
    motorForwards(RIGHT_MOTOR, seconds);
  }
}

void processWebsocketData(uint8_t *jsonData, AsyncWebSocketClient *client)
{
  StaticJsonDocument<128> doc;
  deserializeJson(doc, jsonData);

  JsonObject data = doc["data"];

  const char *data_type = data["type"];

  if (strcmp(data_type, "timer") == 0)
  {
    //int data_id = data["id"];

    JsonObject data_attributes = data["attributes"];
    int data_attributes_timestamp = data_attributes["timestamp"];
    int data_attributes_seconds = data_attributes["seconds"];
    bool data_attributes_active = data_attributes["active"];

    saveTimer(data_attributes_active, data_attributes_timestamp, data_attributes_seconds);

    client->text("angekommen");
  }
  else if (strcmp(data_type, "startMotor") == 0)
  {
    JsonObject data_attributes = data["attributes"];

    int data_attributes_motor = data_attributes["motor"];
    int data_attributes_seconds = data_attributes["seconds"];
    startMotor(data_attributes_motor, data_attributes_seconds);
  }
  else if (strcmp(data_type, "stopMotor") == 0)
  {
    stopMotor(BOTH_MOTORS);
  }
  else if (strcmp(data_type, "reboot") == 0)
  {
    client->text("reboot");
    rebootESP();
  }
}

void onWebsocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_CONNECT)
  {
    char output[384];
    serializeJson(initialInfoMessage(), output);
    client->text(output);

    //client->printf("%lu", BOOT_TIME);
  }
  else if (type == WS_EVT_DATA)
  {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->opcode == WS_TEXT)
      processWebsocketData(data, client);
  }
}

void reconnectWifi()
{
  int wifi_retry = 0;
  while (WiFi.status() != WL_CONNECTED && wifi_retry < 5)
  {
    wifi_retry++;
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiSSID, wifiPassword);
    delay(100);
  }
  if (wifi_retry >= 8)
  {
    rebootESP();
  }
}

void checkTimerAction()
{
  boolean timerActive = preferences.getBool("timerActive");
  if (timerActive == false)
  {
    return;
  }

  unsigned long minus = timeClient.getEpochTime() - preferences.getInt("timerTimestamp");

  if (minus % 86400 == 0)
  {
    digitalWrite(WHITE_LED, HIGH);
    if (TIMER_MINUTE_ACTIVE == false)
    {
      //start for specified time
      long forSeconds = preferences.getInt("timerSeconds");
      startMotor(BOTH_MOTORS, forSeconds * 1000);

      //set flag to true
      TIMER_MINUTE_ACTIVE = true;
    }
  }
  else
  {
    TIMER_MINUTE_ACTIVE = false;
  }
}

void checkMotorStopTimer()
{
  if (STOP_AFTER_RIGHT > 0)
  {
    if (millis() - STOP_AFTER_MILLIS_RIGHT >= STOP_AFTER_RIGHT)
    {
      stopMotor(RIGHT_MOTOR);
    }
  }

  if (STOP_AFTER_LEFT > 0)
  {
    if (millis() - STOP_AFTER_MILLIS_LEFT >= STOP_AFTER_LEFT)
    {
      stopMotor(LEFT_MOTOR);
    }
  }
}

void setup()
{
  WiFi.begin(wifiSSID, wifiPassword);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }
  ArduinoOTA.begin();

  if (!SPIFFS.begin(true))
  {
    return;
  }

  timeClient.begin();
  timeClient.forceUpdate();

  webServer.serveStatic("/", SPIFFS, "/www/");
  webServer.onNotFound(notFound);
  webServer.begin();

  webSocketServer.onEvent(onWebsocketEvent);
  webServer.addHandler(&webSocketServer);

  preferences.begin("dualcatfeeder", false);

  BOOT_TIME = timeClient.getEpochTime();

  //Debugging LED
  pinMode(BLUE_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(WHITE_LED, OUTPUT);
  digitalWrite(BLUE_LED, LOW);
  digitalWrite(RED_LED, LOW);
  digitalWrite(WHITE_LED, LOW);

  //Motor
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  stopMotor(BOTH_MOTORS);
}

void loop()
{
  reconnectWifi();
  ArduinoOTA.handle();
  timeClient.update();
  webSocketServer.cleanupClients();
  checkTimerAction();
  checkMotorStopTimer();
}