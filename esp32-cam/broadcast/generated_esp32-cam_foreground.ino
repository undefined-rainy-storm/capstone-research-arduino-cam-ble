#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "esp_camera.h"

// BLE service and characteristic UUIDs
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// Camera pins for ESP32-CAM
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

// BLE server variables
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
const int MTU_SIZE = 512; // Maximum BLE packet size
const int FRAME_BUFFER_SIZE = 8192; // Adjust based on your needs

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("Device connected");
    }

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Device disconnected");
      // Restart advertising to allow new connections
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

  // Frame size and quality
  config.frame_size = FRAMESIZE_QVGA;   // 320x240
  config.jpeg_quality = 6;             // 0-63, lower means higher quality
  config.fb_count = 1;

  // Initialize camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // Adjust camera settings
  sensor_t * s = esp_camera_sensor_get();
  s->set_brightness(s, 0);     // -2 to 2
  s->set_contrast(s, 0);       // -2 to 2
  s->set_saturation(s, 0);     // -2 to 2
  s->set_special_effect(s, 0); // 0 to 6 (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
}

void initBLE() {
  // Initialize BLE device
  BLEDevice::init("ESP32-CAM");
  
  // Create BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );

  // Create a BLE Descriptor
  pCharacteristic->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // functions that help with iPhone connections issue
  BLEDevice::startAdvertising();

  // Set MTU size
  BLEDevice::setMTU(MTU_SIZE);
  
  Serial.println("BLE device ready");
}

void sendFrame(uint8_t* frame_buffer, size_t frame_length) {
  if (!deviceConnected) return;

  // Send frame size first (4 bytes)
  uint8_t size_buffer[4];
  size_buffer[0] = (frame_length >> 24) & 0xFF;
  size_buffer[1] = (frame_length >> 16) & 0xFF;
  size_buffer[2] = (frame_length >> 8) & 0xFF;
  size_buffer[3] = frame_length & 0xFF;
  pCharacteristic->setValue(size_buffer, 4);
  pCharacteristic->notify();
  delay(10); // Small delay to ensure proper transmission

  // Send frame data in chunks
  size_t chunk_size = MTU_SIZE - 3; // Account for BLE overhead
  size_t sent = 0;
  
  while (sent < frame_length) {
    size_t remaining = frame_length - sent;
    size_t current_chunk_size = (remaining < chunk_size) ? remaining : chunk_size;
    
    pCharacteristic->setValue(&frame_buffer[sent], current_chunk_size);
    pCharacteristic->notify();
    
    sent += current_chunk_size;
    delay(10); // Small delay between chunks
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting ESP32-CAM BLE Server");
  
  initCamera();
  initBLE();
}

void loop() {
  if (deviceConnected) {
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      return;
    }

    // Send the frame
    sendFrame(fb->buf, fb->len);

    // Return the frame buffer to be reused
    esp_camera_fb_return(fb);
    
    // Add delay to control frame rate
    delay(100); // Adjust this value to change frame rate
  }
}