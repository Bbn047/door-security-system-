#include <ESP32Servo.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define servo_pin 18
#define touch_pin 34
#define trig_pin 12
#define echo_pin 14
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 55
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
#define led_pin1 2  // Green LED
#define led_pin2 4  // Red LED

Servo myservo;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

bool accessGranted = false;
bool personDetected = false;
bool doorOpen = false;

unsigned long accessStartTime = 0;
const unsigned long accessDuration = 5000;  // 5 seconds
const int door_width = 20;                  // cm

void setup() {
  Serial.begin(115200);
  myservo.attach(servo_pin);
  pinMode(led_pin1, OUTPUT);
  pinMode(led_pin2, OUTPUT);
  pinMode(touch_pin, INPUT);
  pinMode(trig_pin, OUTPUT);
  pinMode(echo_pin, INPUT);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 initialization failed"));
    while (true);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  closeDoor();
  displayMessage("System Ready");
}

void loop() {
  unsigned long currentTime = millis();
  int distance = readUltrasonic();
  bool touchDetected = digitalRead(touch_pin);

  // TOUCH STARTS ACCESS
  if (touchDetected == HIGH && !accessGranted) {
    accessGranted = true;
    accessStartTime = currentTime;
    personDetected = false;
    openDoor();
    displayMessage("Access Granted");
    return;  // Avoid running more logic this cycle
  }

  // DURING ACCESS WINDOW
  if (accessGranted) {
    if (distance < door_width && !personDetected) {
      personDetected = true;
      displayMessage("Person Entered");
      closeDoor();
      accessGranted = false;
      return;
    }

    if (!personDetected && currentTime - accessStartTime >= accessDuration) {
      displayMessage("No Entry Detected");
      closeDoor();
      accessGranted = false;
      return;
    }

    return;  // Wait out the rest of the access window
  }

  // UNAUTHORIZED ENTRY
  if (!accessGranted && distance < door_width) {
    displayMessage("Security Alert");
    digitalWrite(led_pin1, LOW);   // Green OFF
    digitalWrite(led_pin2, HIGH);  // Red ON
    closeDoor();
    return;
  }

  // IDLE STATE
  displayMessage("Locked");
  digitalWrite(led_pin1, LOW);   // Green OFF
  digitalWrite(led_pin2, HIGH);  // Red ON
}

int readUltrasonic() {
  digitalWrite(trig_pin, LOW);
  delayMicroseconds(2);
  digitalWrite(trig_pin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig_pin, LOW);

  long duration = pulseIn(echo_pin, HIGH);
  int distance = duration * 0.034 / 2;  // cm
  return distance;
}

void displayMessage(String msg) {
  static String lastMsg = "";
  if (msg != lastMsg) {
    display.clearDisplay();
    display.setCursor(0, 1);
    display.println(msg);
    display.display();
    lastMsg = msg;
  }
}

void openDoor() {
  myservo.write(90);
  doorOpen = true;
  digitalWrite(led_pin1, HIGH);  // Green ON
  digitalWrite(led_pin2, LOW);   // Red OFF
}

void closeDoor() {
  myservo.write(0);
  doorOpen = false;
  digitalWrite(led_pin1, LOW);   // Green OFF
  digitalWrite(led_pin2, HIGH);  // Red ON
}
