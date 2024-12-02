#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "esp_camera.h"

// BLE UUIDs
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// Camera pins for AI Thinker ESP32-CAM
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool streamActive = false;

class ServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      streamActive = true;
      Serial.println("Connected");
    }

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      streamActive = false;
      Serial.println("Disconnected");
      pServer->startAdvertising();
    }
};

void initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;    // 320x240
  config.jpeg_quality = 6;              // 0-63 (lower number means higher quality)
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  s->set_brightness(s, 0);
  s->set_contrast(s, 0);
  s->set_saturation(s, 0);
  Serial.println("Camera initialized");
}

void setup() {
  Serial.begin(115200);
  
  // Initialize Camera
  initCamera();
  
  // Initialize BLE
  BLEDevice::init("ESP32-CAM");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());
  
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                     CHARACTERISTIC_UUID,
                     BLECharacteristic::PROPERTY_READ |
                     BLECharacteristic::PROPERTY_NOTIFY
                   );
  pCharacteristic->addDescriptor(new BLE2902());
  
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  BLEDevice::startAdvertising();
  
  Serial.println("BLE Server Started");
}

void sendFrame(uint8_t* frame_buffer, size_t length) {
  const size_t maxChunkSize = 512; // Maximum BLE packet size
  size_t remaining = length;
  size_t offset = 0;

  while (remaining > 0 && deviceConnected) {
    size_t chunkSize = (remaining < maxChunkSize) ? remaining : maxChunkSize;
    
    // Add start/end markers to identify frame chunks
    uint8_t* chunk = new uint8_t[chunkSize + 2];
    chunk[0] = (offset == 0) ? 0xFF : 0x00;  // Start marker
    memcpy(chunk + 1, frame_buffer + offset, chunkSize);
    chunk[chunkSize + 1] = (remaining <= maxChunkSize) ? 0xFE : 0x00;  // End marker
    
    pCharacteristic->setValue(chunk, chunkSize + 2);
    pCharacteristic->notify();
    
    delete[] chunk;
    
    remaining -= chunkSize;
    offset += chunkSize;
    delay(20); // Small delay to prevent flooding
  }
}

void loop() {
  Serial.print("deviceConnected: ");
  Serial.println(deviceConnected);
  Serial.print("streamActive: ");
  Serial.println(streamActive);
  if (deviceConnected && streamActive) {
    
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      delay(1000);
      return;
    }

    sendFrame(fb->buf, fb->len);
    esp_camera_fb_return(fb);
    delay(100); // Control frame rate
  } else {
    delay(100); // Idle delay
  }
}