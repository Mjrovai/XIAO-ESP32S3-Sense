/*
 * ICM20600 Accelerometer Test
 * 
 * NOTE: It is important to update the files I2Cdev.cpp and I2Cdev.h
 * Download both files from: 
 * https://github.com/jrowberg/i2cdevlib/tree/master/Arduino/I2Cdev
 * and replace the ones available on:
 * https://github.com/Seeed-Studio/Seeed_ICM20600_AK09918
 * 
 * Connect the Grove Moodule to the XIAO ESP32S3. 
 * If you do have a Expansion or base Shield, connect the module as below:
 * XIAO ESP32S3     Grove - IMU 9DOF
     VCC (3.3V/5V)         Red
     GND                   Black
     SDA (D4)              White
     SCL (D5)              Yellow

 * by MJRovai @110523  
 */

#include "ICM20600.h"
#include <Wire.h>

int acc_x, acc_y, acc_z;

ICM20600 icm20600(true);

void setup() {
    Serial.begin(115200);
    while (!Serial);
    Serial.println("ICM20600 Accelerometer Test");
  
    Wire.begin();
    icm20600.initialize(); //RANGE_16G 
  
}

void loop() {
    // get acceleration
    acc_x = icm20600.getAccelerationX();
    acc_y = icm20600.getAccelerationY();
    acc_z = icm20600.getAccelerationZ();

    Serial.print("A:  ");
    Serial.print(acc_x);
    Serial.print(",  ");
    Serial.print(acc_y);
    Serial.print(",  ");
    Serial.print(acc_z);
    Serial.println(" mg");

    delay(100);
}
