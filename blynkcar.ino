// BLYNK AUTH
#define BLYNK_TEMPLATE_ID "TMPL4NzoU4s43"
#define BLYNK_TEMPLATE_NAME "ESP32"
#define BLYNK_AUTH_TOKEN "3hW-dEq28Qg57_uU71O1HZcI8uYFTCmC"

// Libraries
#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_MotorShield.h>
#include <Wire.h>
#include <BlynkSimpleEsp32.h>
#include <NewPing.h> 


// Define variables
#define FORWARD 1
#define BACKWARD 2
#define LEFT 3
#define RIGHT 4
#define STOP 5


// Put your Internet credentials here
const char* ssid = "";
const char* password = "";

char auth[] = "3hW-dEq28Qg57_uU71O1HZcI8uYFTCmC";
char server[] = "blynk-cloud.com";
#define BLYNK_PRINT Serial

Adafruit_MotorShield AFMS = Adafruit_MotorShield();
Adafruit_DCMotor* myMotor1;
Adafruit_DCMotor* myMotor2;

int speed = 255; // Default speed

#define FRONT_TRIGGER_PIN 32 // Trigger pin for the front ultrasonic sensor
#define FRONT_ECHO_PIN 14   // Echo pin for the front ultrasonic sensor
#define BACK_TRIGGER_PIN 33 // Trigger pin for the back ultrasonic sensor
#define BACK_ECHO_PIN 15    // Echo pin for the back ultrasonic sensor
#define MAX_DISTANCE 200    // Maximum distance to measure (in cm)

NewPing frontSonar(FRONT_TRIGGER_PIN, FRONT_ECHO_PIN, MAX_DISTANCE); // Initialize front ultrasonic sensor
NewPing backSonar(BACK_TRIGGER_PIN, BACK_ECHO_PIN, MAX_DISTANCE);   // Initialize back ultrasonic sensor

unsigned long previousMillis = 0;
const long interval = 30000; // Interval at which to read temperature, in milliseconds
const long ultrasonicInterval = 100; // Interval at which to check distance, in milliseconds
unsigned long previousUltrasonicMillis = 0;

void setup() {
    Serial.begin(9600);
    Blynk.begin(auth, ssid, password);

    Serial.println("Setting up Motor Shield");
    AFMS.begin();
    myMotor1 = AFMS.getMotor(3);
    myMotor2 = AFMS.getMotor(4);
    myMotor1->setSpeed(speed);
    myMotor2->setSpeed(speed);
}

void loop() {
    Blynk.run();

    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
    }
    
    if (currentMillis - previousUltrasonicMillis >= ultrasonicInterval) {
        previousUltrasonicMillis = currentMillis;

        unsigned int frontDistance = frontSonar.ping_cm();
        unsigned int backDistance = backSonar.ping_cm();

        if (frontDistance > 0 && frontDistance < 10) { // Obstacle is closer than 10 cm in front
            moveBackwardShort();
        } else if (backDistance > 0 && backDistance < 10) { // Obstacle is closer than 10 cm in the back
            moveForwardShort();
        }
    }
}

BLYNK_WRITE(V0) { // Slider to control speed
    speed = param.asInt();
    myMotor1->setSpeed(speed);
    myMotor2->setSpeed(speed);
}

BLYNK_WRITE(V1) {  // Forward Button
    int pinValue = param.asInt();
    if (pinValue == 1) {
        Serial.println("Moving forward");
        myMotor1->run(FORWARD);
        myMotor2->run(FORWARD);
    } else {
        stopMotors();
    }
}

BLYNK_WRITE(V2) {  // Backward Button
    int pinValue = param.asInt();
    if (pinValue == 1) {
        Serial.println("Moving backward");
        myMotor1->run(BACKWARD);
        myMotor2->run(BACKWARD);
    } else {
        stopMotors();
    }
}

BLYNK_WRITE(V3) {  // Left Button
    int pinValue = param.asInt();
    if (pinValue == 1) {
        Serial.println("Turning left");
        myMotor1->run(BACKWARD);
        myMotor2->run(FORWARD);
    } else {
        stopMotors();
    }
}

BLYNK_WRITE(V4) {  // Right Button
    int pinValue = param.asInt();
    if (pinValue == 1) {
        Serial.println("Turning right");
        myMotor1->run(FORWARD);
        myMotor2->run(BACKWARD);
    } else {
        stopMotors();
    }
}

void stopMotors() {
    Serial.println("Stopping");
    myMotor1->run(RELEASE);
    myMotor2->run(RELEASE);
}

void moveBackwardShort() {
    Serial.println("Obstacle detected in front! Moving backward briefly.");
    myMotor1->run(BACKWARD);
    myMotor2->run(BACKWARD);
    delay(200); // Move backward for 500 milliseconds (adjust as needed)
    stopMotors();
}

void moveForwardShort() {
    Serial.println("Obstacle detected in the back! Moving forward briefly.");
    myMotor1->run(FORWARD);
    myMotor2->run(FORWARD);
    delay(200); // Move forward for 500 milliseconds (adjust as needed)
    stopMotors();
}