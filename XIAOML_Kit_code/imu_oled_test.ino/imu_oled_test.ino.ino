#include <U8g2lib.h>
#include <Wire.h>
#include <Arduino.h>
#include <LSM6DS3.h>

// Define I2C interface and resolution
LSM6DS3 myIMU(I2C_MODE, 0x6A); // I2C device address 0x6A
U8G2_SSD1306_72X40_ER_1_HW_I2C u8g2(U8G2_R2, U8X8_PIN_NONE);
// Alternative software I2C (if hardware I2C doesn't work):
// U8G2_SSD1306_72X40_ER_1_SW_I2C u8g2(U8G2_R0, SCL, SDA, U8X8_PIN_NONE);

float aX, aY, aZ, gX, gY, gZ;
char buf[10];
uint8_t x = 5, y = 7; // Screen coordinate parameters
uint8_t currentPage = 0; // Current page (0=accelerometer, 1=gyroscope)
unsigned long lastPageChangeTime = 0; // Last page change time
const unsigned long PAGE_CHANGE_INTERVAL = 3000; // Page change interval (milliseconds)

void setup() {
  Serial.begin(115200);
  
  // Initialize IMU
  if (myIMU.begin() != 0) {
    Serial.println("IMU initialization failed!");
    while(1);
  } else {
    Serial.println("IMU initialized successfully");
  }
  
  // OLED initialization
  u8g2.begin();
  u8g2.clearDisplay();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  
  Serial.println("XIAO ESP32S3 IMU and OLED Test");
  Serial.println("Accelerometer and Gyroscope data will alternate on display every 3 seconds");
}

void loop() {
  // Switch pages at regular intervals
  if (millis() - lastPageChangeTime >= PAGE_CHANGE_INTERVAL) {
    currentPage = (currentPage + 1) % 2;
    lastPageChangeTime = millis();
  }
  
  // Read IMU data
  aX = myIMU.readFloatAccelX();
  aY = myIMU.readFloatAccelY();
  aZ = myIMU.readFloatAccelZ();
  gX = myIMU.readFloatGyroX();
  gY = myIMU.readFloatGyroY();
  gZ = myIMU.readFloatGyroZ();
  
  // Print data to Serial Monitor
  Serial.print("Accel X: "); Serial.print(aX, 3);
  Serial.print(" Y: "); Serial.print(aY, 3);
  Serial.print(" Z: "); Serial.print(aZ, 3);
  Serial.print(" | Gyro X: "); Serial.print(gX, 3);
  Serial.print(" Y: "); Serial.print(gY, 3);
  Serial.print(" Z: "); Serial.println(gZ, 3);
  
  // Draw the current page
  u8g2.firstPage();
  do {
    if (currentPage == 0) {
      // Page 0: Accelerometer data
      u8g2.setCursor(x, y);
      sprintf(buf, "ax: %.3f", aX);
      u8g2.print(buf);
      
      u8g2.setCursor(x, 3*y);
      sprintf(buf, "ay: %.3f", aY);
      u8g2.print(buf);
      
      u8g2.setCursor(x, 5*y);
      sprintf(buf, "az: %.3f", aZ);
      u8g2.print(buf);
    }
    else {
      // Page 1: Gyroscope data
      u8g2.setCursor(x, y);
      sprintf(buf, "gx: %.3f", gX);
      u8g2.print(buf);
      
      u8g2.setCursor(x, 3*y);
      sprintf(buf, "gy: %.3f", gY);
      u8g2.print(buf);
      
      u8g2.setCursor(x, 5*y);
      sprintf(buf, "gz: %.3f", gZ);
      u8g2.print(buf);
    }
  } while (u8g2.nextPage());
  
  delay(500);
}
