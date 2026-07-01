#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const int TRIG_PIN = 5;
const int ECHO_PIN = 18;
const int RED_LED_PIN = 26;
const int GREEN_LED_PIN = 27;
const int BUZZER_PIN = 25;

const float OCCUPIED_THRESHOLD_CM = 50.0;
const float AVAILABLE_THRESHOLD_CM = 55.0;
const unsigned long SENSOR_TIMEOUT_US = 30000;
const unsigned long DASHBOARD_INTERVAL_MS = 500;

const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 64;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

bool occupied = false;
bool displayReady = false;
unsigned long lastDashboardUpdate = 0;

float readDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration = pulseIn(ECHO_PIN, HIGH, SENSOR_TIMEOUT_US);
  if (duration == 0) {
    return 400.0;
  }

  return duration / 58.0;
}

bool decideParkingStatus(float distanceCm) {
  if (occupied) {
    return distanceCm < AVAILABLE_THRESHOLD_CM;
  }

  return distanceCm < OCCUPIED_THRESHOLD_CM;
}

void updateIndicators(bool isOccupied) {
  digitalWrite(RED_LED_PIN, isOccupied ? HIGH : LOW);
  digitalWrite(GREEN_LED_PIN, isOccupied ? LOW : HIGH);
}

void updateDisplay(float distanceCm, bool isOccupied) {
  if (!displayReady) {
    return;
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Smart Parking Bay");
  display.drawLine(0, 12, 127, 12, SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 20);
  display.println(isOccupied ? "OCCUPIED" : "AVAILABLE");

  display.setTextSize(1);
  display.setCursor(0, 48);
  display.print("Distance: ");
  display.print(distanceCm, 1);
  display.println(" cm");
  display.display();
}

void printDashboard(float distanceCm, bool isOccupied) {
  Serial.print("Parking Bay 1 | Distance: ");
  Serial.print(distanceCm, 1);
  Serial.print(" cm | Status: ");
  Serial.println(isOccupied ? "OCCUPIED" : "AVAILABLE");
}

void beepOnNewOccupation(bool previousStatus, bool currentStatus) {
  if (!previousStatus && currentStatus) {
    tone(BUZZER_PIN, 1200, 180);
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  Wire.begin(21, 22);
  displayReady = display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  updateIndicators(occupied);
  updateDisplay(400.0, occupied);

  Serial.println("IoT-Based Smart Parking Space Detector");
  Serial.println("Dashboard: HC-SR04 distance, LED state, OLED status, buzzer alert");
}

void loop() {
  float distanceCm = readDistanceCm();
  bool previousStatus = occupied;
  occupied = decideParkingStatus(distanceCm);

  updateIndicators(occupied);
  beepOnNewOccupation(previousStatus, occupied);

  unsigned long now = millis();
  if (now - lastDashboardUpdate >= DASHBOARD_INTERVAL_MS) {
    updateDisplay(distanceCm, occupied);
    printDashboard(distanceCm, occupied);
    lastDashboardUpdate = now;
  }

  delay(50);
}
