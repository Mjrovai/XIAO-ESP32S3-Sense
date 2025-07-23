#include <U8g2lib.h>
#include <Wire.h>

// Initialize the OLED display
// SSD1306 controller, 72x40 resolution, I2C interface
U8G2_SSD1306_72X40_ER_1_HW_I2C u8g2(U8G2_R2, U8X8_PIN_NONE);

// Variables for demonstration
int counter = 0;
unsigned long lastUpdate = 0;
const unsigned long UPDATE_INTERVAL = 1000; // Update every 1 second

// Animation variables
int animationStep = 0;
unsigned long lastAnimation = 0;
const unsigned long ANIMATION_INTERVAL = 200; // Animation speed

void setup() {
  Serial.begin(115200);
  
  Serial.println("XIAO ESP32S3 OLED Display Test");
  Serial.println("SSD1306 - 72x40 Monochrome Display");
  Serial.println("==================================");
  
  // Initialize the display
  u8g2.begin();
  
  Serial.println("✓ Display initialized successfully");
  Serial.println("Display specifications:");
  Serial.println("- Resolution: 72x40 pixels");
  Serial.println("- Controller: SSD1306");
  Serial.println("- Interface: I2C (address 0x3C)");
  Serial.println("- Colors: Monochrome (black/white)");
  Serial.println();
  
  // Clear display and show startup message
  u8g2.clearDisplay();
  displayStartupMessage();
  
  delay(3000); // Show startup message for 3 seconds
}

void loop() {
  // Demonstrate different display functions
  static int demoStep = 0;
  static unsigned long lastDemo = 0;
  const unsigned long DEMO_DURATION = 4000; // Each demo runs for 4 seconds
  
  if (millis() - lastDemo >= DEMO_DURATION) {
    demoStep = (demoStep + 1) % 6; // 6 different demos
    lastDemo = millis();
    Serial.println("Demo step: " + String(demoStep + 1) + "/6");
  }
  
  switch (demoStep) {
    case 0:
      displayTextDemo();
      break;
    case 1:
      displayCounterDemo();
      break;
    case 2:
      displayShapesDemo();
      break;
    case 3:
      displayAnimationDemo();
      break;
    case 4:
      displayProgressBarDemo();
      break;
    case 5:
      displaySensorSimulation();
      break;
  }
  
  delay(50); // Small delay for smooth operation
}

void displayStartupMessage() {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.setCursor(8, 12);
    u8g2.print("XIAO ML");
    u8g2.setCursor(15, 25);
    u8g2.print("Kit");
    u8g2.setCursor(5, 38);
    u8g2.print("OLED Test");
  } while (u8g2.nextPage());
}

void displayTextDemo() {
  u8g2.firstPage();
  do {
    // Title
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.setCursor(2, 10);
    u8g2.print("Text Demo");
    
    // Different font sizes
    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.setCursor(2, 20);
    u8g2.print("Small font");
    
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.setCursor(2, 32);
    u8g2.print("Medium");
    
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.setCursor(45, 32);
    u8g2.print("Big");
  } while (u8g2.nextPage());
}

void displayCounterDemo() {
  // Update counter every second
  if (millis() - lastUpdate >= UPDATE_INTERVAL) {
    counter++;
    lastUpdate = millis();
  }
  
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.setCursor(2, 10);
    u8g2.print("Counter:");
    
    u8g2.setFont(u8g2_font_ncenB14_tr);
    u8g2.setCursor(15, 32);
    u8g2.print(counter);
  } while (u8g2.nextPage());
}

void displayShapesDemo() {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.setCursor(2, 8);
    u8g2.print("Shapes Demo");
    
    // Rectangle
    u8g2.drawFrame(5, 12, 15, 10);
    
    // Filled rectangle
    u8g2.drawBox(25, 12, 15, 10);
    
    // Circle
    u8g2.drawCircle(52, 17, 8);
    
    // Line
    u8g2.drawLine(5, 28, 67, 28);
    
    // Pixels
    for (int i = 0; i < 10; i++) {
      u8g2.drawPixel(5 + i * 6, 35);
    }
  } while (u8g2.nextPage());
}

void displayAnimationDemo() {
  // Update animation
  if (millis() - lastAnimation >= ANIMATION_INTERVAL) {
    animationStep = (animationStep + 1) % 20;
    lastAnimation = millis();
  }
  
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.setCursor(2, 8);
    u8g2.print("Animation");
    
    // Moving circle
    int x = 5 + (animationStep * 3);
    u8g2.drawCircle(x, 20, 3);
    
    // Pulsing rectangle
    int size = 5 + (animationStep % 10);
    u8g2.drawFrame(35 - size/2, 20 - size/2, size, size);
    
    // Progress indicator
    u8g2.drawFrame(2, 30, 68, 6);
    int progress = (animationStep * 68) / 20;
    u8g2.drawBox(2, 30, progress, 6);
  } while (u8g2.nextPage());
}

void displayProgressBarDemo() {
  static int progress = 0;
  
  // Update progress
  if (millis() - lastUpdate >= 100) {
    progress = (progress + 2) % 101;
    lastUpdate = millis();
  }
  
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.setCursor(2, 10);
    u8g2.print("Progress");
    
    // Progress bar background
    u8g2.drawFrame(5, 18, 62, 8);
    
    // Progress bar fill
    int fillWidth = (progress * 60) / 100;
    u8g2.drawBox(6, 19, fillWidth, 6);
    
    // Percentage text
    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.setCursor(28, 35);
    u8g2.print(String(progress) + "%");
  } while (u8g2.nextPage());
}

void displaySensorSimulation() {
  // Simulate sensor data
  float temp = 20.0 + sin(millis() / 1000.0) * 5.0;
  int humidity = 50 + sin(millis() / 1200.0) * 20;
  
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.setCursor(2, 8);
    u8g2.print("Sensors");
    
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.setCursor(2, 20);
    u8g2.print("T:");
    u8g2.setCursor(15, 20);
    u8g2.print(String(temp, 1) + "C");
    
    u8g2.setCursor(2, 32);
    u8g2.print("H:");
    u8g2.setCursor(15, 32);
    u8g2.print(String(humidity) + "%");
    
    // Simple bar graphs
    u8g2.drawFrame(45, 14, 25, 4);
    u8g2.drawBox(45, 14, (temp - 15) * 25 / 10, 4);
    
    u8g2.drawFrame(45, 28, 25, 4);
    u8g2.drawBox(45, 28, humidity * 25 / 100, 4);
  } while (u8g2.nextPage());
}
