/*
 * This code was adapted from the MRovai version for Round Display
 * 
 * Important changes:
 *  - It is not necessary to to cut off J3 on the XIAO ESP32S3 Sense expansion board. 
 *  It is possible to use the XIAO's SD Card Reader. For that you need use SD_CS_PIN as 21.
 * 
 * - The camera buffer data should be captured as RGB565 (raw image) with a 240x240 frame size  
 * to be displayed on the round display. This raw image should be converted to jpeg before save 
 * in the SD card. This can be done with the line: 
 * esp_err_t ret = frame2jpg(fb, 12, &out_buf, &out_len);
 * 
 * - The XCLK_FREQ_HZ should be reduced from 20KHz to 10KHz in order to prevent 
 * the message "no EV-VSYNC-OVF message" that appears on Serial Monitor (probably due the time 
 * added for JPEG conversion. 
 *
 * - This example is mainly to see the Fomo bounding boxes in real  time, using Display.
 *
 * 
 * Adapted by Djair Guilherme, April 2024
 * 
 * 
*/

// Include TFT Libraries. Don't forget to set the correct setup file for TFT_eSPI

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>

// Your FOMO Inferencing header comes here
#include <XIAO-ESP32S3-Sense-Small-Object_Detection_inferencing.h>
#include "edge-impulse-sdk/dsp/image/image.hpp"

// Prepare Camera Pinout
#include "esp_camera.h"
#define CAMERA_MODEL_XIAO_ESP32S3 // Has PSRAM

#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     10
#define SIOD_GPIO_NUM     40
#define SIOC_GPIO_NUM     39

#define Y9_GPIO_NUM       48
#define Y8_GPIO_NUM       11
#define Y7_GPIO_NUM       12
#define Y6_GPIO_NUM       14
#define Y5_GPIO_NUM       16
#define Y4_GPIO_NUM       18
#define Y3_GPIO_NUM       17
#define Y2_GPIO_NUM       15
#define VSYNC_GPIO_NUM    38
#define HREF_GPIO_NUM     47
#define PCLK_GPIO_NUM     13

#define EI_CAMERA_RAW_FRAME_BUFFER_COLS           240
#define EI_CAMERA_RAW_FRAME_BUFFER_ROWS           240
#define EI_CAMERA_FRAME_BYTE_SIZE                 3
#define TOUCH_INT D7

static bool debug_nn = false; // Set this to true to see e.g. features generated from the raw signal
static bool is_initialised = false;
uint8_t *snapshot_buf; //points to the output of the capture

TFT_eSPI tft = TFT_eSPI();

/* This is just a mix for the Edge Impulse Library and MJRovai original example... */
const int camera_width = EI_CAMERA_RAW_FRAME_BUFFER_COLS;
const int camera_height = EI_CAMERA_RAW_FRAME_BUFFER_ROWS;

/* Function definitions , from Edge Impulse default generated library */
void ei_camera_deinit(void);
bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf) ;

void setup() {

  Serial.begin(115200);
  while (!Serial);

  // Preparing Seeed Xiao Round Camera pinout
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
  config.xclk_freq_hz = 10000000; // Reduced XCLK_FREQ_HZ from 20KHz to 10KHz (no EV-VSYNC-OVF message)

  config.frame_size = FRAMESIZE_240X240; // The round display Resolution 

  config.pixel_format = PIXFORMAT_RGB565; // Raw Camera Input without JPEG conversion
  
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  
  if (config.pixel_format == PIXFORMAT_JPEG) {
    if (psramFound()) {
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }
  
  // Camera init - Edge Impulse library has an separated function for this... Not needed.
  
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  Serial.println("Camera ready");
  is_initialised = true; // Camera initialization check passes

  // Initialize TFT Display with White Background
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_WHITE);

  Serial.println("Edge Impulse Inferencing Demo");

  ei_printf("\nStarting continious inference in 2 seconds...\n");
  ei_sleep(2000);
}


void loop()
{

  // This remains from Edge Impulse Demo
  if (ei_sleep(5) != EI_IMPULSE_OK) {
    return;
  }

  snapshot_buf = (uint8_t*)malloc(EI_CAMERA_RAW_FRAME_BUFFER_COLS * EI_CAMERA_RAW_FRAME_BUFFER_ROWS * EI_CAMERA_FRAME_BYTE_SIZE);

  // check if allocation was successful
  if (snapshot_buf == nullptr) {
    ei_printf("ERR: Failed to allocate snapshot buffer!\n");
    return;
  }

  ei::signal_t signal;
  signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
  signal.get_data = &ei_camera_get_data;

  if (ei_camera_capture((size_t)EI_CLASSIFIER_INPUT_WIDTH, (size_t)EI_CLASSIFIER_INPUT_HEIGHT, snapshot_buf) == false) {
    ei_printf("Failed to capture image\r\n");
    free(snapshot_buf);
    return;
  }

  // Run the classifier
  ei_impulse_result_t result = { 0 };

  EI_IMPULSE_ERROR err = run_classifier(&signal, &result, debug_nn);
  if (err != EI_IMPULSE_OK) {
    ei_printf("ERR: Failed to run classifier (%d)\n", err);
    return;
  }

  // print the predictions
  ei_printf("Predictions (DSP: %d ms., Classification: %d ms., Anomaly: %d ms.): \n",
            result.timing.dsp, result.timing.classification, result.timing.anomaly);

#if EI_CLASSIFIER_OBJECT_DETECTION == 1
  bool bb_found = result.bounding_boxes[0].value > 0;
  for (size_t ix = 0; ix < result.bounding_boxes_count; ix++) {
    auto bb = result.bounding_boxes[ix];

    // This will be used at Rectangle Draw
    uint32_t x = bb.x;
    uint32_t y = bb.y;
    uint32_t w = bb.width;
    uint32_t h = bb.height;
    
    if (bb.value == 0) {
      continue;
    }
    ei_printf("    %s (%f) [ x: %u, y: %u, width: %u, height: %u ]\n", bb.label, bb.value, bb.x, bb.y, bb.width, bb.height);
    
    // Draw Green Rectangle Bounding Box!
    tft.drawRect (x,y,w,h, TFT_GREEN);
  }
  if (!bb_found) {
    ei_printf("    No objects found\n");
  }
#else
  for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
    ei_printf("    %s: %.5f\n", result.classification[ix].label,
              result.classification[ix].value);
  }
#endif

#if EI_CLASSIFIER_HAS_ANOMALY == 1
  ei_printf("    anomaly score: %.3f\n", result.anomaly);
#endif

  free(snapshot_buf);

}


void ei_camera_deinit(void) {

  //deinitialize the camera - Remains from Edge Impulse Demo
  esp_err_t err = esp_camera_deinit();

  if (err != ESP_OK)
  {
    ei_printf("Camera deinit failed\n");
    return;
  }

  is_initialised = false;
  return;
}


bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf) {
  bool do_resize = false;

  if (!is_initialised) {
    ei_printf("ERR: Camera is not initialized\r\n");
    return false;
  }

  // Capture to Framebuffer
  camera_fb_t *fb = esp_camera_fb_get();

  if (!fb) {
    ei_printf("Camera capture failed\n");
    return false;
  } else {
    // Show image at TFT Display - Yeah
    tft.startWrite();
    tft.setAddrWindow(0, 0, camera_width, camera_height);
    tft.pushColors(fb->buf, fb->len);
    tft.endWrite();
  }

  // Release Camera Framebuffer
  esp_camera_fb_return(fb);


  if ((img_width != EI_CAMERA_RAW_FRAME_BUFFER_COLS)
      || (img_height != EI_CAMERA_RAW_FRAME_BUFFER_ROWS)) {
    do_resize = true;
  }

  if (do_resize) {
    ei::image::processing::crop_and_interpolate_rgb888(
      out_buf,
      EI_CAMERA_RAW_FRAME_BUFFER_COLS,
      EI_CAMERA_RAW_FRAME_BUFFER_ROWS,
      out_buf,
      img_width,
      img_height);
  }

  return true;
}

static int ei_camera_get_data(size_t offset, size_t length, float *out_ptr)
{
  // we already have a RGB888 buffer, so recalculate offset into pixel index
  size_t pixel_ix = offset * 3;
  size_t pixels_left = length;
  size_t out_ptr_ix = 0;

  while (pixels_left != 0) {
    out_ptr[out_ptr_ix] = (snapshot_buf[pixel_ix] << 16) + (snapshot_buf[pixel_ix + 1] << 8) + snapshot_buf[pixel_ix + 2];

    // go to the next pixel
    out_ptr_ix++;
    pixel_ix += 3;
    pixels_left--;
  }
  // and done!
  return 0;
}

