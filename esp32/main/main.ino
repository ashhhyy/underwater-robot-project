// ESP32-CAM Underwater Drone Sketch
// Features:
// - Camera capture with ESP32-CAM OV2640
// - TensorFlow Lite micro inference placeholder to detect "fish"
// - Save image to microSD if fish detected
// - Obstacle avoidance using ultrasonic sensors (left, right, bottom depth sensor)
// - Gyroscope sensor MPU6050 for orientation stabilization
// - Controls 4 propellers (2 for forward/backward, 2 for up/down)
// - Auto submerge when turned ON and auto float up when turned OFF
// - Web server for on/off control and dashboard display

#include <WiFi.h>
#include <WebServer.h>
#include <esp_camera.h>
#include <SPI.h>
#include <SD.h>
#include "FS.h"
#include <Wire.h>
#include <MPU6050.h>  // Library for MPU6050 gyroscope sensor

// WiFi Credentials
const char* ssid = "your_SSID";
const char* password = "your_PASSWORD";

// Motor Control Pins (example GPIO pins, adjust as needed)
#define MOTOR1_PIN1 12  // Forward/backward motor 1
#define MOTOR1_PIN2 13
#define MOTOR2_PIN1 14  // Forward/backward motor 2
#define MOTOR2_PIN2 27
#define MOTOR3_PIN1 26  // Up/down motor 1
#define MOTOR3_PIN2 25
#define MOTOR4_PIN1 33  // Up/down motor 2
#define MOTOR4_PIN2 32

// Ultrasonic sensor analog pins
#define LEFT_SENSOR_PIN 34   
#define RIGHT_SENSOR_PIN 35  
#define DEPTH_SENSOR_PIN 36  

// I2C pins for MPU6050
#define I2C_SDA 21
#define I2C_SCL 22

MPU6050 mpu;

WebServer server(80);

bool systemOn = false;

// Orientation data
float pitch = 0.0;
float roll = 0.0;

camera_config_t config;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Motor pins
  pinMode(MOTOR1_PIN1, OUTPUT);
  pinMode(MOTOR1_PIN2, OUTPUT);
  pinMode(MOTOR2_PIN1, OUTPUT);
  pinMode(MOTOR2_PIN2, OUTPUT);
  pinMode(MOTOR3_PIN1, OUTPUT);
  pinMode(MOTOR3_PIN2, OUTPUT);
  pinMode(MOTOR4_PIN1, OUTPUT);
  pinMode(MOTOR4_PIN2, OUTPUT);
  stopMotors();

  // Initialize I2C for MPU6050
  Wire.begin(I2C_SDA, I2C_SCL);
  Serial.println("Initializing MPU6050...");
  mpu.initialize();
  if (mpu.testConnection()) {
    Serial.println("MPU6050 connection successful");
  } else {
    Serial.println("MPU6050 connection failed");
  }

  // Connect to WiFi
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  int wifi_attempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifi_attempts < 30) {
    delay(1000);
    Serial.print(".");
    wifi_attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi connection failed!");
  }

  // Camera config
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 5;
  config.pin_d1 = 18;
  config.pin_d2 = 19;
  config.pin_d3 = 21;
  config.pin_d4 = 36;
  config.pin_d5 = 39;
  config.pin_d6 = 34;
  config.pin_d7 = 35;
  config.pin_xclk = 0;
  config.pin_pclk = 22;
  config.pin_vsync = 25;
  config.pin_href = 23;
  config.pin_sscb_sda = 26;
  config.pin_sscb_scl = 27;
  config.pin_pwdn = 32;
  config.pin_reset = -1;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_SVGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return;
  }
  Serial.println("Camera initialized");

  // Initialize SD card
  if(!SD.begin()) {
    Serial.println("SD Card Mount Failed");
  } else {
    Serial.println("SD Card Mount Success");
  }

  // Setup web server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/control", HTTP_POST, handleControl);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();

  if (systemOn) {
    submerge();
    captureAndProcessImage();
    updateOrientation();
    stabilizeOrientation();
    avoidObstacles();
  } else {
    floatUp();
    stopForwardMotors();
  }

  delay(500);
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head><title>ESP32-CAM Underwater Drone</title>";
  html += "<style>";
  html += "body {font-family: Arial; margin: 20px; background: #002B36; color: #839496;}";
  html += "button {padding: 12px 24px; font-size: 18px; margin: 10px; background: #268bd2; border: none; color: white; border-radius: 8px; cursor:pointer;}";
  html += "button:hover {background: #2aa198;}";
  html += ".status {font-weight:bold; margin-top: 20px;}";
  html += "</style></head><body>";
  html += "<h1>ESP32-CAM Underwater Drone Control</h1>";
  html += "<button onclick=\"toggleDrone(true)\">Turn ON</button>";
  html += "<button onclick=\"toggleDrone(false)\">Turn OFF</button>";
  html += "<div class='status'>Status: <span id='status'>" + String(systemOn ? "ON" : "OFF") + "</span></div>";
  html += "<h2>Sensor Data</h2>";
  html += "<ul>";
  html += "<li>Left Distance: <span id='leftDist'>" + String(readDistanceCM(LEFT_SENSOR_PIN)) + "</span> cm</li>";
  html += "<li>Right Distance: <span id='rightDist'>" + String(readDistanceCM(RIGHT_SENSOR_PIN)) + "</span> cm</li>";
  html += "<li>Depth Distance: <span id='depthDist'>" + String(readDistanceCM(DEPTH_SENSOR_PIN)) + "</span> cm</li>";
  html += "<li>Pitch: <span id='pitch'>" + String(pitch, 2) + "</span> °</li>";
  html += "<li>Roll: <span id='roll'>" + String(roll, 2) + "</span> °</li>";
  html += "</ul>";
  html += "<script>";
  html += "function toggleDrone(state) {";
  html += "fetch('/control', {method:'POST', headers: {'Content-Type': 'text/plain'}, body: state ? 'on' : 'off'})";
  html += ".then(response => response.text())";
  html += ".then(data => {document.getElementById('status').innerText = state ? 'ON' : 'OFF';});}";
  html += "setInterval(() => {location.reload();}, 10000);"; // reload every 10s
  html += "</script></body></html>";

  server.send(200, "text/html", html);
}

void handleControl() {
  if(server.hasArg("plain")) {
    String body = server.arg("plain");
    body.trim();
    if(body == "on"){
      systemOn = true;
      server.send(200, "text/plain", "Drone turned ON");
      Serial.println("Drone ON");
      return;
    } else if(body == "off"){
      systemOn = false;
      server.send(200, "text/plain", "Drone turned OFF");
      Serial.println("Drone OFF");
      return;
    }
  }
  server.send(400, "text/plain", "Invalid request");
}

void handleNotFound(){
  server.send(404, "text/plain", "Not Found");
}

void captureAndProcessImage() {
  camera_fb_t* fb = esp_camera_fb_get();
  if(!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  Serial.println("Image captured");

  bool fishDetected = runInference(fb->buf, fb->len);

  if(fishDetected) {
    Serial.println("Fish detected! Saving image to SD card...");
    String path = "/fish_" + String(millis()) + ".jpg";
    fs::FS &fs = SD;
    File file = fs.open(path.c_str(), FILE_WRITE);
    if(!file){
      Serial.println("Failed to open file on SD card");
    } else {
      file.write(fb->buf, fb->len);
      file.close();
      Serial.println("Saved file to: " + path);
    }
  } else {
    Serial.println("No fish detected");
  }

  esp_camera_fb_return(fb);
}

bool runInference(uint8_t* imageData, size_t length) {
  int r = random(10);
  return (r == 0);
}

void updateOrientation() {
  int16_t ax, ay, az;
  int16_t gx, gy, gz;

  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  float accelX = ax / 16384.0;
  float accelY = ay / 16384.0;
  float accelZ = az / 16384.0;

  pitch = atan2(accelY, sqrt(accelX*accelX + accelZ*accelZ)) * 180.0 / PI;
  roll = atan2(-accelX, accelZ) * 180.0 / PI;

  Serial.printf("Pitch: %.2f, Roll: %.2f\n", pitch, roll);
}

void stabilizeOrientation() {
  const int maxCorrPWM = 30;
  const float kp_pitch = 1.2;
  const float kp_roll = 1.0;

  int pitchCorr = constrain(int(kp_pitch * pitch), -maxCorrPWM, maxCorrPWM);
  int rollCorr = constrain(int(kp_roll * roll), -maxCorrPWM, maxCorrPWM);

  Serial.printf("Pitch Correction: %d, Roll Correction: %d\n", pitchCorr, rollCorr);

  // Adjust pitch control motors
  if(pitchCorr > 0) {
    digitalWrite(MOTOR1_PIN1, LOW);
    digitalWrite(MOTOR1_PIN2, LOW);
    digitalWrite(MOTOR2_PIN1, LOW);
    digitalWrite(MOTOR2_PIN2, LOW);
  } else if(pitchCorr < 0) {
    digitalWrite(MOTOR1_PIN1, HIGH);
    digitalWrite(MOTOR1_PIN2, LOW);
    digitalWrite(MOTOR2_PIN1, HIGH);
    digitalWrite(MOTOR2_PIN2, LOW);
  }

  // Adjust roll control motors
  if(rollCorr > 0) {
    digitalWrite(MOTOR3_PIN1, HIGH);
    digitalWrite(MOTOR3_PIN2, LOW);
    digitalWrite(MOTOR4_PIN1, LOW);
    digitalWrite(MOTOR4_PIN2, LOW);
  } else if(rollCorr < 0) {
    digitalWrite(MOTOR3_PIN1, LOW);
    digitalWrite(MOTOR3_PIN2, LOW);
    digitalWrite(MOTOR4_PIN1, HIGH);
    digitalWrite(MOTOR4_PIN2, LOW);
  } else {
    digitalWrite(MOTOR3_PIN1, LOW);
    digitalWrite(MOTOR3_PIN2, LOW);
    digitalWrite(MOTOR4_PIN1, LOW);
    digitalWrite(MOTOR4_PIN2, LOW);
  }
}

void avoidObstacles() {
  int leftDist = readDistanceCM(LEFT_SENSOR_PIN);
  int rightDist = readDistanceCM(RIGHT_SENSOR_PIN);
  int depthDist = readDistanceCM(DEPTH_SENSOR_PIN);

  Serial.printf("Sensors - Left: %d cm, Right: %d cm, Depth: %d cm\n", leftDist, rightDist, depthDist);

  const int obstacleThresh = 30;

  if(leftDist > 0 && leftDist < obstacleThresh) {
    Serial.println("Obstacle on LEFT, moving right");
    digitalWrite(MOTOR1_PIN1, LOW);
    digitalWrite(MOTOR1_PIN2, LOW);
    digitalWrite(MOTOR2_PIN1, HIGH);
    digitalWrite(MOTOR2_PIN2, LOW);
  } else if(rightDist > 0 && rightDist < obstacleThresh) {
    Serial.println("Obstacle on RIGHT, moving left");
    digitalWrite(MOTOR1_PIN1, HIGH);
    digitalWrite(MOTOR1_PIN2, LOW);
    digitalWrite(MOTOR2_PIN1, LOW);
    digitalWrite(MOTOR2_PIN2, LOW);
  } else if(depthDist > 0 && depthDist < obstacleThresh) {
    Serial.println("Obstacle BELOW, moving up");
    digitalWrite(MOTOR3_PIN1, HIGH);
    digitalWrite(MOTOR3_PIN2, LOW);
    digitalWrite(MOTOR4_PIN1, HIGH);
    digitalWrite(MOTOR4_PIN2, LOW);
  }
  // Stabilization motors handled by stabilizeOrientation()
}

void stopMotors() {
  digitalWrite(MOTOR1_PIN1, LOW);
  digitalWrite(MOTOR1_PIN2, LOW);
  digitalWrite(MOTOR2_PIN1, LOW);
  digitalWrite(MOTOR2_PIN2, LOW);
  digitalWrite(MOTOR3_PIN1, LOW);
  digitalWrite(MOTOR3_PIN2, LOW);
  digitalWrite(MOTOR4_PIN1, LOW);
  digitalWrite(MOTOR4_PIN2, LOW);
}

void stopForwardMotors() {
  digitalWrite(MOTOR1_PIN1, LOW);
  digitalWrite(MOTOR1_PIN2, LOW);
  digitalWrite(MOTOR2_PIN1, LOW);
  digitalWrite(MOTOR2_PIN2, LOW);
}

// Automatically submerge by activating downward thrusters
void submerge() {
  Serial.println("Submerging...");
  digitalWrite(MOTOR3_PIN1, LOW);
  digitalWrite(MOTOR3_PIN2, HIGH);  // Reverse motor 3 for downward thrust
  digitalWrite(MOTOR4_PIN1, LOW);
  digitalWrite(MOTOR4_PIN2, HIGH);  // Reverse motor 4 for downward thrust
}

// Automatically float up by activating upward thrusters
void floatUp() {
  Serial.println("Floating up...");
  digitalWrite(MOTOR3_PIN1, HIGH);
  digitalWrite(MOTOR3_PIN2, LOW);  // Forward motor 3 for upward thrust
  digitalWrite(MOTOR4_PIN1, HIGH);
  digitalWrite(MOTOR4_PIN2, LOW);  // Forward motor 4 for upward thrust
}

int readDistanceCM(int analogPin) {
  int val = analogRead(analogPin);
  int distance = map(val, 4095, 0, 20, 600);
  if(distance < 20) distance = 0;
  if(distance > 600) distance = 600;
  return distance;
}

