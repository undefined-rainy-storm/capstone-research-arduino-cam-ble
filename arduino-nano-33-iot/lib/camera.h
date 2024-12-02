#ifndef _CAMERA_MODULE
#define _CAMERA_MODULE

#include <Wire.h>
#include <SD.h>

enum CamreaType {
  OV7670
};

void initCamera();

class CameraManager {
public:
  CameraManager(CameraType cameraType) {
    switch (cameraType) {
      case CamreaType.OV7670 {
        
      }
    }
  }
};

#endif
