////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include <Attribute_Request.h>
#include <Arduino_MQTT_Client.h>
#include <DHT20.h>
#include <Espressif_Updater.h>
#include <HardwareSerial.h>
#include <OTA_Firmware_Update.h>
#include <Shared_Attribute_Update.h>
#include <ThingsBoard.h>
#include <WiFi.h>

////////////////////////////////////////////////////////////////////////////////////////////////////

constexpr uint32_t SERIAL_DEBUG_BAUD = 9600UL;

constexpr char WIFI_SSID[] = "Oreki";
constexpr char WIFI_PASSWORD[] = "hardware";

constexpr char TOKEN[] = "zhNGUEL74XzDLd40IQKZ";
constexpr char THINGSBOARD_SERVER[] = "app.coreiot.io";
constexpr uint16_t THINGSBOARD_PORT = 1883U;
constexpr uint16_t MAX_MESSAGE_SEND_SIZE = 512U;
constexpr uint16_t MAX_MESSAGE_RECEIVE_SIZE = 512U;

constexpr char CURRENT_FIRMWARE_TITLE[] = "DHT";
constexpr char CURRENT_FIRMWARE_VERSION[] = "1.0";
constexpr uint8_t FIRMWARE_FAILURE_RETRIES = 12U;
constexpr uint16_t FIRMWARE_PACKET_SIZE = 4096U;
constexpr uint8_t MAX_ATTRIBUTES = 1U;

Shared_Attribute_Update<1U, MAX_ATTRIBUTES> shared_update;
Attribute_Request<2U, MAX_ATTRIBUTES> attr_request;
OTA_Firmware_Update<> ota;
Espressif_Updater<> updater;
const std::array<IAPI_Implementation*, 3U> apis = {
    &shared_update,
    &attr_request,
    &ota
};

WiFiClient wifiClient;
Arduino_MQTT_Client mqttClient(wifiClient);
ThingsBoard thingsboard(mqttClient, MAX_MESSAGE_RECEIVE_SIZE, MAX_MESSAGE_SEND_SIZE, Default_Max_Stack_Size, apis);
DHT20 dht20;

bool sharedUpdateSubscribed = false;
bool currentFWSent = false;
bool updateRequestSent = false;
bool requestedShared = false;

volatile double temperature = 0.0;
volatile double humidity = 0.0;

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void InitWiFi() {
  Serial.print("Connecting to WiFi ...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(); Serial.println("Connected to WiFi!");
}

void InitThingsBoard() {
  Serial.print("Connecting to ThingsBoard ...");
  thingsboard.connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT);
  while (!thingsboard.connected()) {
    delay(500);
    Serial.print(".");
    thingsboard.connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT);
  }
  Serial.println(); Serial.println("Connected to ThingsBoard!");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void TaskHelloWorld(void *pvParameters) {
  while (1) {
    Serial.println("Hello World!");
    vTaskDelay(1000);
  }
}

void TaskWiFi(void *pvParameters) {
  while(1) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi disconnected, attempting to reconnect ...");
      InitWiFi();
    }
    vTaskDelay(5000);
  }
}

void TaskThingsBoard(void *pvParameters) {
  while(1) {
    if (!thingsboard.connected()) {
      Serial.println("ThingsBoard disconnected, attempting to reconnect ...");
      InitThingsBoard();
    }
    vTaskDelay(5000);
  }
}

void TaskDHT20(void *pvParameters) {
  while(1) {
    dht20.read();
    temperature = dht20.getTemperature();
    humidity = dht20.getHumidity();
    Serial.print("Temperature: "); Serial.print(temperature); Serial.println(" *C");
    Serial.print("Humidity: "); Serial.print(humidity); Serial.println(" %");
    thingsboard.sendTelemetryData("temperature", temperature);
    thingsboard.sendTelemetryData("humidity", humidity);
    vTaskDelay(5000);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  Serial.begin(SERIAL_DEBUG_BAUD);
  Serial.println();
  dht20.begin();
  Wire.begin(GPIO_NUM_21, GPIO_NUM_22);
  pinMode(GPIO_NUM_2, OUTPUT);
  InitWiFi();
  InitThingsBoard();
  // xTaskCreate(TaskHelloWorld, "HelloWorld", 2048, NULL, 2, NULL);
  xTaskCreate(TaskWiFi, "WiFi", 2048, NULL, 2, NULL);
  xTaskCreate(TaskThingsBoard, "ThingsBoard", 2048, NULL, 2, NULL);
  xTaskCreate(TaskDHT20, "DHT20", 2048, NULL, 2, NULL);
}

void loop() {}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////