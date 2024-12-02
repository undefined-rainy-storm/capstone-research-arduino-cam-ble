#include <Wire.h>
#include <SD.h>

// Pin definitions for OV7670
const int vsyncPin = 3;
const int hrefPin = 2;
const int pclkPin = 4;
const int dataPins[] = {22, 23, 24, 25, 26, 27, 28, 29}; // D0-D7 connected to pins 22-29

// SD card
const int chipSelect = 10;
File imgFile;

void setup() {
  Serial.begin(115200);
  Wire.begin(); // Initialize I2C
  initCamera(); // Initialize the camera
  generateXCLK(); // Start generating XCLK
  
  // Initialize SD card
  if (!SD.begin(chipSelect)) {
    Serial.println("SD card initialization failed!");
    return;
  }
  imgFile = SD.open("image.raw", FILE_WRITE);
  if (!imgFile) {
    Serial.println("Error opening file!");
    return;
  }
  
  // Setup pins
  pinMode(vsyncPin, INPUT);
  pinMode(hrefPin, INPUT);
  pinMode(pclkPin, INPUT);
  for (int i = 0; i < 8; i++) {
    pinMode(dataPins[i], INPUT);
  }

  Serial.println("Starting image capture...");
  captureImage(); // Capture image from the camera
}

void loop() {
  // Nothing to do here
}

void initCamera() {
  Wire.beginTransmission(0x21); // OV7670 I2C address
  Wire.write(0x12); // COM7 register address
  Wire.write(0x80); // Reset camera
  Wire.endTransmission();

  delay(100); // Give some time for the camera to reset
  
  // Configure more registers as needed for your application
  // This is just a basic example. Check the OV7670 datasheet for more details.
}

void generateXCLK() {
  // Setup Timer1 to generate an 8 MHz clock on pin 11 (XCLK)
  pinMode(11, OUTPUT);
  TCCR1A = 0x00; // Setup Timer1
  TCCR1B = _BV(WGM12) | _BV(CS10); // CTC mode, no prescaler
  OCR1A = 7; // This will generate an 8 MHz clock signal on pin 11
  TIMSK1 = _BV(OCIE1A); // Enable timer interrupt
}

ISR(TIMER1_COMPA_vect) {
  digitalWrite(11, !digitalRead(11)); // Toggle XCLK on pin 11
}

void captureImage() {
  // Wait for the frame to start (VSYNC high)
  while (digitalRead(vsyncPin) == LOW);
  while (digitalRead(vsyncPin) == HIGH);
  
  // Now read each line and pixel data
  while (digitalRead(vsyncPin) == LOW) { // Frame is active
    if (digitalRead(hrefPin) == HIGH) { // Line is active
      while (digitalRead(hrefPin) == HIGH) { // Continue reading line
        while (digitalRead(pclkPin) == LOW); // Wait for PCLK rising edge
        byte pixelData = readPixelData(); // Read pixel data
        imgFile.write(pixelData); // Save pixel data to SD card
      }
    }
  }
  
  imgFile.close();
  Serial.println("Image capture complete.");
}

