/* Adding TFT Support Libraries */
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>

/* Adding Edge Impulse Inferencing Library */
#include <Add_Your_inferencing_here.h> // Remember to change this line!
#include "edge-impulse-sdk/dsp/image/image.hpp"

/* Adding ESP32S3 Sense Camera Support */
#include "esp_camera.h"

/*  The bounding boxes were coming out small and the coordinates were out of place. 
    I understood that, although the camera configuration points to 240x240, 
    the images of the bounding boxes in the TFT were not in the same proportion. 
    I decided to divide the maximum resolutions (1600x1200) by 240 and arrived at values 6.67 and 5.
 */

#define MAX_WIDTH 1600
#define MAX_HEIGHT 1200

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

#define TOUCH_INT D7

/* Original EDGE IMPULSE example parameters - Change Camera Capture to display same size as TFT */
#define EI_CAMERA_RAW_FRAME_BUFFER_COLS           240
#define EI_CAMERA_RAW_FRAME_BUFFER_ROWS           240
#define EI_CAMERA_FRAME_BYTE_SIZE                 3

/* Private variables ------------------------------------------------------- */
static bool debug_nn = false; // Set this to true to see e.g. features generated from the raw signal
static bool is_initialised = false;
uint8_t *snapshot_buf; //points to the output of the capture

/* Camera Config Parameters */
static camera_config_t camera_config = {
  .pin_pwdn = PWDN_GPIO_NUM,
  .pin_reset = RESET_GPIO_NUM,
  .pin_xclk = XCLK_GPIO_NUM,
  .pin_sscb_sda = SIOD_GPIO_NUM,
  .pin_sscb_scl = SIOC_GPIO_NUM,

  .pin_d7 = Y9_GPIO_NUM,
  .pin_d6 = Y8_GPIO_NUM,
  .pin_d5 = Y7_GPIO_NUM,
  .pin_d4 = Y6_GPIO_NUM,
  .pin_d3 = Y5_GPIO_NUM,
  .pin_d2 = Y4_GPIO_NUM,
  .pin_d1 = Y3_GPIO_NUM,
  .pin_d0 = Y2_GPIO_NUM,
  .pin_vsync = VSYNC_GPIO_NUM,
  .pin_href = HREF_GPIO_NUM,
  .pin_pclk = PCLK_GPIO_NUM,

  //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
  .xclk_freq_hz = 10000000,
  .ledc_timer = LEDC_TIMER_0,
  .ledc_channel = LEDC_CHANNEL_0,

  /* This config pixel format needed to be the same in function ei_camera_capture ()
                                                        HERE
     bool converted = fmt2rgb888(fb->buf, fb->len, PIXFORMAT_RGB565, snapshot_buf);

  */
  .pixel_format = PIXFORMAT_RGB565, //YUV422,GRAYSCALE,RGB565,JPEG
  .frame_size = FRAMESIZE_240X240,    //QQVGA-UXGA Do not use sizes above QVGA when not JPEG

  .jpeg_quality = 12, //0-63 lower number means higher quality
  .fb_count = 2,       //if more than one, i2s runs in continuous mode. Use only with JPEG
  .fb_location = CAMERA_FB_IN_PSRAM, // Frame Buffer in PSRAM - YES!
  .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};

/* Function definitions ------------------------------------------------------- */
bool ei_camera_init(void);
void ei_camera_deinit(void);
bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf) ;

/* Create TFT_SPI object */
TFT_eSPI tft = TFT_eSPI();

/* Bounding Boxes Coordinates need to be uint32_t in TFTeSPI Library */
uint32_t x;
uint32_t y;
uint32_t w;
uint32_t h;

/**
  @brief      Arduino setup function
*/
void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  //comment out the below line to start inference immediately after upload
  // while (!Serial);

  /* Init TFT */
  tft.init();

  /*   Need to set Rotation to 3 and change Camera Sensor 
      Config to Sync coordinates of bounding boxes*/
  
  tft.setRotation(3);
  tft.fillScreen(TFT_WHITE);

  if (ei_camera_init() == false) {
    ei_printf("Failed to initialize Camera!\r\n");
  }
  else {
    ei_printf("Camera initialized\r\n");
  }

  ei_printf("\nStarting continious inference in 2 seconds...\n");
  ei_sleep(2000);
}

/**
  @brief      Get data and run inferencing

  @param[in]  debug  Get debug info if true
*/
void loop()
{

  // instead of wait_ms, we'll wait on the signal, this allows threads to cancel us...
  if (ei_sleep(5) != EI_IMPULSE_OK) {
    return;
  }

  snapshot_buf = (uint8_t*)malloc(EI_CAMERA_RAW_FRAME_BUFFER_COLS * EI_CAMERA_RAW_FRAME_BUFFER_ROWS * EI_CAMERA_FRAME_BYTE_SIZE);

  // check if allocation was successful
  if (snapshot_buf == nullptr) {
    ei_printf("ERR: Failed to allocate snapshot buffer!\n");
    return;
  }

  /* The signal that will be inferred is this one */
  ei::signal_t signal;
  signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
  signal.get_data = &ei_camera_get_data;

  /*  Makes the call to the ei_camera_capture function,
      which captures the image to the framebuffer,
      converts the image to the correct format and resizes it.
  */
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

  // Print the predictions to Serial Debug
  ei_printf("TFT Example -  Predictions (DSP: %d ms., Classification: %d ms., Anomaly: %d ms.): \n",
            result.timing.dsp, result.timing.classification, result.timing.anomaly);

#if EI_CLASSIFIER_OBJECT_DETECTION == 1
  bool bb_found = result.bounding_boxes[0].value > 0;


  for (size_t ix = 0; ix < result.bounding_boxes_count; ix++) {
    auto bb = result.bounding_boxes[ix];
    if (bb.value == 0) {
      continue;
    }
    /*
      commenting this line, just to compare dimensions above
      ei_printf("   %s (%f) [ x: %u, y: %u, width: %u, height: %u ]\n", bb.label, bb.value, bb.x, bb.y, bb.width, bb.height);

    */

    /* Create Coordinates and Size for Bounding Boxes */
    x = bb.x;
    y = bb.y;

    /* This operation is needed in order correct the dimensions of camera sensor and TFT display resolution */
    w = bb.width * 6; // 1600 / 240 = 6.67
    h = bb.height * 5; // 1200 MAX / 240 = 5

    /* Checking Sizes */
    ei_printf("BB Coord [ x: %u, y: %u, width: %u, height: %u ]\n",  bb.x, bb.y, bb.width, bb.height);
    ei_printf("Percentage %u",  result.classification[0].value);

    /* Draw Bounding Boxes in Display */
    tft.drawRect (x, y, w, h, TFT_GREEN);
    tft.setCursor(x, y);
    tft.setTextColor(TFT_GREEN);
    tft.setTextFont(4);
    tft.println(bb.label);
    tft.endWrite();
  }
  if (!bb_found) {
    ei_printf("TFT Example -     No objects found\n");
    tft.endWrite();
  }
#else
  for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
    ei_printf("    %s: %.5f\n", result.classification[ix].label,
              result.classification[ix].value);
  }
#endif

#if EI_CLASSIFIER_HAS_ANOMALY == 1
  ei_printf("TFT Example -     anomaly score: %.3f\n", result.anomaly);
#endif


  free(snapshot_buf);

}

/**
   @brief   Setup image sensor & start streaming

   @retval  false if initialisation failed
*/
bool ei_camera_init(void) {

  if (is_initialised) return true;

  //initialize the camera
  esp_err_t err = esp_camera_init(&camera_config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return false;
  }

  sensor_t * s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated

  /*  You need to change these settings, otherwise
      the x and y coordinates of the TFT and the camera will be
      different from each other. 
      It appears that the TFT is rotated 90 counterclockwise in relation to the camera...
  */
  s->set_vflip(s, 1); // flip vertical
  s->set_hmirror(s, 1); // mirror horizontal
  
  is_initialised = true;
  return true;
}

/**
   @brief      Stop streaming of sensor data
*/
void ei_camera_deinit(void) {

  //deinitialize the camera
  esp_err_t err = esp_camera_deinit();

  if (err != ESP_OK)
  {
    ei_printf("Camera deinit failed\n");
    return;
  }

  is_initialised = false;
  return;
}


/**
   @brief      Capture, rescale and crop image

   @param[in]  img_width     width of output image
   @param[in]  img_height    height of output image
   @param[in]  out_buf       pointer to store output image, NULL may be used
                             if ei_camera_frame_buffer is to be used for capture and resize/cropping.

   @retval     false if not initialised, image captured, rescaled or cropped failed

*/
bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf) {
  bool do_resize = false;

  if (!is_initialised) {
    ei_printf("ERR: Camera is not initialized\r\n");
    return false;
  }

  camera_fb_t *fb = esp_camera_fb_get();

  if (!fb) {
    ei_printf("Camera capture failed\n");
    return false;
  }
  /*  Write camera captured framebuffer into TFT Display */
  tft.startWrite();
  tft.setAddrWindow(0, 0, EI_CAMERA_RAW_FRAME_BUFFER_COLS, EI_CAMERA_RAW_FRAME_BUFFER_ROWS);
  tft.pushColors(fb->buf, fb->len);

  /*  Pay attention to this PIXFORMAT_RGB565 parameter in camera config. Need to be equal or convertion will fail  */
  bool converted = fmt2rgb888(fb->buf, fb->len, PIXFORMAT_RGB565, snapshot_buf);

  esp_camera_fb_return(fb);

  if (!converted) {
    ei_printf("Conversion failed\n");
    return false;
  }

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

#if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_CAMERA
#error "Invalid model for current sensor"
#endif
