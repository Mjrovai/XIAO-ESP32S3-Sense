// Motion Classification with LSM6DS3TR-C IMU
#include <XIAOML_Kit_Motion_Class_-_AD_inferencing.h>
#include <LSM6DS3.h>
#include <Wire.h>
#include <U8g2lib.h>

// IMU setup
LSM6DS3 myIMU(I2C_MODE, 0x6A);

// OLED setup
U8G2_SSD1306_72X40_ER_1_HW_I2C u8g2(U8G2_R2, U8X8_PIN_NONE);

// Inference settings
#define CONVERT_G_TO_MS2    9.81f
#define MAX_ACCEPTED_RANGE  2.0f * CONVERT_G_TO_MS2

static bool debug_nn = false;
static float buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE] = { 0 };
static float inference_buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];

// Function declarations
void displayResult(String prediction, float confidence, float anomaly_score);
void ei_printf(const char *format, ...);

// OLED display function
void displayResult(String prediction, float confidence, float anomaly_score) {
    u8g2.firstPage();
    do {
        u8g2.setFont(u8g2_font_6x10_tr);
        
        // Display prediction
        u8g2.setCursor(2, 12);
        u8g2.print(prediction.substring(0, 10)); // Truncate if too long
        
        // Display confidence
        u8g2.setCursor(2, 24);
        u8g2.print((int)(confidence * 100));
        u8g2.print("%");
        
        // Display anomaly indicator
        if (anomaly_score > 0.5) {
            u8g2.setCursor(2, 36);
            u8g2.print("ANOMALY!");
        }
        
        // Add border
        u8g2.drawFrame(0, 0, 72, 40);
        
    } while (u8g2.nextPage());
}

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);
    
    Serial.println("XIAOML Kit - Motion Classification");
    Serial.println("LSM6DS3TR-C IMU Inference");
    
    // Initialize IMU
    if (myIMU.begin() != 0) {
        Serial.println("ERROR: IMU initialization failed!");
        return;
    }
    
    Serial.println("✓ IMU initialized");
    
    // Initialize OLED
    u8g2.begin();
    u8g2.clearDisplay();
    
    // Show startup message
    u8g2.firstPage();
    do {
        u8g2.setFont(u8g2_font_6x10_tr);
        u8g2.setCursor(2, 15);
        u8g2.print("Motion");
        u8g2.setCursor(2, 30);
        u8g2.print("Classify");
    } while (u8g2.nextPage());
    
    Serial.println("✓ OLED initialized");
    
    if (EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME != 3) {
        Serial.println("ERROR: EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME should be 3");
        return;
    }
    
    Serial.println("✓ Model loaded");
    Serial.println("Starting motion classification...");
    delay(2000);
}

void loop() {
    ei_printf("\nStarting inferencing in 2 seconds...\n");
    delay(2000);
    
    ei_printf("Sampling...\n");
    
    // Clear buffer
    for (size_t i = 0; i < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE; i++) {
        buffer[i] = 0.0f;
    }
    
    // Collect accelerometer data
    for (int i = 0; i < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE; i += 3) {
        uint64_t next_tick = micros() + (EI_CLASSIFIER_INTERVAL_MS * 1000);
        
        // Read IMU data
        float x = myIMU.readFloatAccelX();
        float y = myIMU.readFloatAccelY();
        float z = myIMU.readFloatAccelZ();
        
        // Convert to m/s²
        buffer[i + 0] = x * CONVERT_G_TO_MS2;
        buffer[i + 1] = y * CONVERT_G_TO_MS2;
        buffer[i + 2] = z * CONVERT_G_TO_MS2;
        
        // Apply range limiting
        for (int j = 0; j < 3; j++) {
            if (fabs(buffer[i + j]) > MAX_ACCEPTED_RANGE) {
                buffer[i + j] = copysign(MAX_ACCEPTED_RANGE, buffer[i + j]);
            }
        }
        
        delayMicroseconds(next_tick - micros());
    }
    
    // Copy to inference buffer
    for (int i = 0; i < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE; i++) {
        inference_buffer[i] = buffer[i];
    }
    
    // Create signal from buffer
    signal_t signal;
    int err = numpy::signal_from_buffer(inference_buffer, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);
    if (err != 0) {
        ei_printf("ERROR: Failed to create signal from buffer (%d)\n", err);
        return;
    }
    
    // Run the classifier
    ei_impulse_result_t result = { 0 };
    err = run_classifier(&signal, &result, debug_nn);
    if (err != EI_IMPULSE_OK) {
        ei_printf("ERROR: Failed to run classifier (%d)\n", err);
        return;
    }
    
    // Print predictions
    ei_printf("Predictions (DSP: %d ms, Classification: %d ms, Anomaly: %d ms):\n",
        result.timing.dsp, result.timing.classification, result.timing.anomaly);
        
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        ei_printf("    %s: %.5f\n", result.classification[ix].label, result.classification[ix].value);
    }
    
    // Print anomaly score
#if EI_CLASSIFIER_HAS_ANOMALY == 1
    ei_printf("Anomaly score: %.3f\n", result.anomaly);
#endif

    // Determine prediction
    float max_confidence = 0.0;
    String predicted_class = "unknown";
    
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        if (result.classification[ix].value > max_confidence) {
            max_confidence = result.classification[ix].value;
            predicted_class = String(result.classification[ix].label);
        }
    }
    
    // Display result with confidence threshold
    if (max_confidence > 0.6) {
        ei_printf("\n🎯 PREDICTION: %s (%.1f%% confidence)\n", 
                 predicted_class.c_str(), max_confidence * 100);
    } else {
        ei_printf("\n❓ UNCERTAIN: Highest confidence is %s (%.1f%%)\n", 
                 predicted_class.c_str(), max_confidence * 100);
    }
    
    // Check for anomaly
#if EI_CLASSIFIER_HAS_ANOMALY == 1
    if (result.anomaly > 0.5) {
        ei_printf("⚠️  ANOMALY DETECTED! Score: %.3f\n", result.anomaly);
    }
#endif

    // Update OLED display with results
#if EI_CLASSIFIER_HAS_ANOMALY == 1
    displayResult(predicted_class, max_confidence, result.anomaly);
#else
    displayResult(predicted_class, max_confidence, 0.0);
#endif
    
    delay(1000);
}

void ei_printf(const char *format, ...) {
    static char print_buf[1024] = { 0 };
    va_list args;
    va_start(args, format);
    int r = vsnprintf(print_buf, sizeof(print_buf), format, args);
    va_end(args);
    if (r > 0) {
        Serial.write(print_buf);
    }
}