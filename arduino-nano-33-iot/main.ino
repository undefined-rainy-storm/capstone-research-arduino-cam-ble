#include "lib/camera.h"
#include "src/config.h"

void setup() {
  #if ENABLE_DEBUG
  Serial.begin(DEBUG_SERIAL_SPEED);
  #endif

  // Initialize Camera Module
  Wire.begin();  // I2C
  initCamera();
  genXCLK();
}

void loop() {}