#include <U8g2lib.h>
#include <Wire.h>

// Initialize the OLED display
// SSD1306 controller, 72x40 resolution, I2C interface
U8G2_SSD1306_72X40_ER_1_HW_I2C u8g2(U8G2_R2, U8X8_PIN_NONE);

void setup() {
  Serial.begin(115200);
  
  Serial.println("XIAOML Kit - Hello World");
  Serial.println("==========================");
  
  // Initialize the display
  u8g2.begin();
  
  Serial.println("✓ Display initialized");
  Serial.println("Showing Hello World message...");
  
  // Clear the display
  u8g2.clearDisplay();
}

void loop() {
  // Start drawing sequence
  u8g2.firstPage();
  do {
    // Set font
    u8g2.setFont(u8g2_font_ncenB08_tr);
    
    // Display "Hello World" centered
    u8g2.setCursor(8, 15);
    u8g2.print("Hello");
    
    u8g2.setCursor(12, 30);
    u8g2.print("World!");
    
    // Add a simple decoration - draw a frame around the text
    u8g2.drawFrame(2, 2, 68, 36);
    
  } while (u8g2.nextPage());
  
  // No delay needed - the display will show continuously
}
