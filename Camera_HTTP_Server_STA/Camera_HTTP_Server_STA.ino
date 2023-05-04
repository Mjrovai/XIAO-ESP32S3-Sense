/******************************************************************************************
*  XIAO-ESP32S3-Sense - Image Capture & Save via WebPage Server
* 
*   This program is a demo of how to use the XIAO-ESP32S3-Sense camera.
*   It can take a photo and send it to the Web for verification or saving it at a SD Card.
* 
*   The demo sketch will do the following tasks:
*    1. Set the camera to JPEG output mode.
*    2. Create a web page (for example ==> http://192.168.4.119//) ==> See the correct IP on 
*       Serial Monitor. It can be changed.
*    3. if server.on ("/capture", HTTP_GET, serverCapture) ==> take photo and send to the Web.
*    4. It is possible to rotate the image at webPage, using the button [ROTATE]
*    5. [CAPTURE] will previw the image on the web, showing its size on Serial Monitor
*    6. [SAVE] will save an image on the SD Card, showing the image on the web. Saved images 
*       will follow a sequential naming (image1.jpg, image2.jpg...
*    
*  Developed by Marcelo Rovai on 03May23
*  Visit my blog: https://MJRoBot.org 
*  
*  Based on code and tutorials: 
*  Rui Santos:  https://RandomNerdTutorials.com/esp32-cam-take-photo-display-web-server/
*  Working with WebServer:https://lastminuteengineers.com/creating-esp32-web-server-arduino-ide/
*  ESP8266 SPIFF image code:https://www.mauroalfieri.it/elettronica/esp8266-spiff-image-code.html
*********************************************************************************************/

#include <WiFi.h>
#include <WebServer.h>
#include "webpage.h"
#include "esp_camera.h"
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "soc/soc.h"           // Disable brownour problems
#include "soc/rtc_cntl_reg.h"  // Disable brownour problems
#include "driver/rtc_io.h"
#include <SPIFFS.h>
#include <FS.h>
#include "SD.h"
#include "SPI.h"


/*Put your SSID & Password*/
const char* ssid = "Your credentials here";  // Enter SSID here
const char* password = "Your credentials here";  //Enter Password here

WebServer server(80);

boolean takeNewPhoto = false;
boolean savePhotoSD = false;
bool sd_sign = false;              // Check sd status
int imageCount = 1;                // File Counter

// Photo File Name to save in SPIFFS
#define FILE_PHOTO "/photo.jpg"

#define CAMERA_MODEL_XIAO_ESP32S3 // Has PSRAM
#include "camera_pins.h"

bool camera_sign = false;          // Check camera status

//Stores the camera configuration parameters
camera_config_t config;

void setup() {
  Serial.begin(115200);
  delay(100);
 
  // Mounting SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    ESP.restart();
  }
  else {
    delay(500);
    Serial.println("SPIFFS mounted successfully");
  }

  //Initialize the camera  
  Serial.print("Initializing the camera module...");
  configInitCamera();
  Serial.println("Ok!");

  // Initialize SD card
  if(!SD.begin(21)){
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  // Determine if the type of SD card is available
  if(cardType == CARD_NONE){
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if(cardType == CARD_MMC){
    Serial.println("MMC");
  } else if(cardType == CARD_SD){
    Serial.println("SDSC");
  } else if(cardType == CARD_SDHC){
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  sd_sign = true; // sd initialization check passes


  // Connect to local wi-fi network
  Serial.println("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  //check wi-fi is connected to wi-fi network
  while (WiFi.status() != WL_CONNECTED) {
  delay(1000);
  Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected..!");
  Serial.print("Got IP: ");  
  Serial.println(WiFi.localIP());

  // Turn-off the 'brownout detector'
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  server.on("/", handle_OnConnect);
  server.on("/capture", handle_capture);
  server.on("/save", handle_save);
  server.on("/saved_photo", []() {getSpiffImg(FILE_PHOTO, "image/jpg"); } );
  server.onNotFound(handle_NotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  
  if (takeNewPhoto) {
    capturePhotoSaveSpiffs();
    takeNewPhoto = false;
  }
  if (savePhotoSD) {
    char filename[32];
    sprintf(filename, "/image%d.jpg", imageCount);
    photo_save(filename);
    Serial.printf("Saved picture：%s\n", filename);
    Serial.println("");
    imageCount++;  
    savePhotoSD = false;
  }

  delay(1);
}

void handle_OnConnect() {
  server.send(200, "text/html", index_html); 
}

void handle_capture() {
  takeNewPhoto = true;
  Serial.println("Capturing Image for view only");
  server.send(200, "text/plain", "Taking Photo"); 
}

void handle_save() {
  savePhotoSD = true;
  Serial.println("Saving Image to SD Card");
  server.send(200, "text/plain", "Saving Image to SD Card"); 
}


void handle_savedPhoto() {
  Serial.println("Saving Image");
  server.send(200, "image/jpg", FILE_PHOTO); 
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

/******************************************************
  Functions
*******************************************************/

// Check if photo capture was successful
bool checkPhoto( fs::FS &fs ) {
  File f_pic = fs.open( FILE_PHOTO );
  unsigned int pic_sz = f_pic.size();
  return ( pic_sz > 100 );
}

// Capture Photo and Save it to SPIFFS
void capturePhotoSaveSpiffs( void ) {
  camera_fb_t * fb = NULL; // pointer
  bool ok = 0; // Boolean indicating if the picture has been taken correctly
  int fileSize = 0;

  do {
    // Take a photo with the camera
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      return;
    }

    // Photo for view
    File file = SPIFFS.open(FILE_PHOTO, FILE_WRITE);

    // Insert the data in the photo file
    if (!file) {
      Serial.println("Failed to open file in writing mode");
    }
    else {
      file.write(fb->buf, fb->len); // payload (image), payload length
    }
    fileSize = file.size();
    // Close the file
    file.close();
    esp_camera_fb_return(fb);

    // check if file has been correctly saved in SPIFFS
    ok = checkPhoto(SPIFFS);
  } while ( !ok );
    Serial.print("The picture has a ");
    Serial.print("size of ");
    Serial.print(fileSize);
    Serial.println(" bytes");
}


void getSpiffImg(String path, String TyPe) { 
 if(SPIFFS.exists(path)){ 
    File file = SPIFFS.open(path, "r");
    server.streamFile(file, TyPe);
    file.close();
  }
}

// Save pictures to SD card
void photo_save(const char * fileName) {
  camera_fb_t * fb = NULL; // pointer
  bool ok = 0; // Boolean indicating if the picture has been taken correctly

  do {
    // Take a photo
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Failed to get camera frame buffer");
      return;
    }
    // Save photo to file
    writeFile(SD, fileName, fb->buf, fb->len);
  
    // Insert the data in the photo file 
    File file = SPIFFS.open(FILE_PHOTO, FILE_WRITE);
    file.write(fb->buf, fb->len); // payload (image), payload length
    file.close();

    // Release image buffer
    esp_camera_fb_return(fb);
      
    // check if file has been correctly saved in SPIFFS
    ok = checkPhoto(SPIFFS);
  } while ( !ok );
  Serial.println("Photo saved to file");
}


// SD card write file
void writeFile(fs::FS &fs, const char * path, uint8_t * data, size_t len){
    //Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.write(data, len) != len){
        Serial.println("Write failed");
    }
    file.close();
}

void configInitCamera(){
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
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG; // for streaming
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  
  // Initialize the Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
    //ESP.restart();
  }
  camera_sign = true; // Camera initialization check passes

}
