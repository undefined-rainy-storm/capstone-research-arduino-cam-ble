// config.h
#ifndef CONFIG_H
#define CONFIG_H

// WiFi credentials if needed
#define WIFI_SSID "your_ssid"
#define WIFI_PASSWORD "your_password"

// Camera settings
#define FRAME_SIZE FRAMESIZE_VGA    // Resolution: 640x480
#define JPEG_QUALITY 12             // 0-63 (lower means better quality)
#define FRAME_RATE 15               // Frames per second

// BLE settings
#define BLE_DEVICE_NAME "ESP32-CAM"
#define BLE_MAX_MTU 512
#define STREAMING_INTERVAL 50       // Milliseconds between frames

// Power management
#define BATTERY_CHECK_INTERVAL 60000 // Check battery every minute
#define LOW_BATTERY_THRESHOLD 3.3    // Volts
#define CPU_FREQUENCY 160           // MHz

// Debug settings
#define SERIAL_BAUD_RATE 115200
#define DEBUG_MODE true

#endif