/* ---Robo auto--- */
#define BLYNK_TEMPLATE_ID "TMPL4NzoU4s43"
#define BLYNK_TEMPLATE_NAME "ESP32"
#define BLYNK_AUTH_TOKEN "3hW-dEq28Qg57_uU71O1HZcI8uYFTCmC"

#include <WiFi.h>
#include <Adafruit_MotorShield.h>
#include <Wire.h>
#include <BlynkSimpleEsp32.h>
#include <NewPing.h>
#include <esp_now.h>

// Rakenne tietojen vastaanottamiseen
typedef struct struct_message {
  int xAxis;
  int yAxis;
  bool buttonPressed;
} struct_message;

// Muuttuja saapuville tiedoille
struct_message incomingData;

#define FORWARD 1
#define BACKWARD 2
#define LEFT 3
#define RIGHT 4
#define STOP 5

// WiFi-tiedot
const char* ssid = "TVT-WLAN";
const char* password = "salasana";

// Blynk-tunnukset
char auth[] = "3hW-dEq28Qg57_uU71O1HZcI8uYFTCmC";
char server[] = "blynk-cloud.com";
#define BLYNK_PRINT Serial

// Moottorisuoja ja moottorien määrittely
Adafruit_MotorShield AFMS = Adafruit_MotorShield();
Adafruit_DCMotor* myMotor1;
Adafruit_DCMotor* myMotor2;

int speed = 255;

// Ultraäänisensorien pinnit ja maksimietäisyys
#define FRONT_TRIGGER_PIN 32
#define FRONT_ECHO_PIN 14
#define BACK_TRIGGER_PIN 33
#define BACK_ECHO_PIN 15
#define MAX_DISTANCE 200

NewPing frontSonar(FRONT_TRIGGER_PIN, FRONT_ECHO_PIN, MAX_DISTANCE);
NewPing backSonar(BACK_TRIGGER_PIN, BACK_ECHO_PIN, MAX_DISTANCE);

unsigned long previousUltrasonicMillis = 0;
const long ultrasonicInterval = 100;

// Funktio datan vastaanottamiseksi ESP-NOW:n kautta
void OnDataRecv(const esp_now_recv_info *recv_info, const uint8_t *data, int len) {
  memcpy(&incomingData, data, sizeof(incomingData));
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("xAxis: ");
  Serial.println(incomingData.xAxis);
  Serial.print("yAxis: ");
  Serial.println(incomingData.yAxis);
  Serial.print("Button pressed: ");
  Serial.println(incomingData.buttonPressed);
}

void setup() {
    Serial.begin(115200);

    delay(1000);

    // Blynk-yhteyden aloitus
    Blynk.begin(auth, ssid, password);

    // Moottorisuoja-asetukset
    Serial.println("Setting up Motor Shield");
    AFMS.begin();
    myMotor1 = AFMS.getMotor(3);
    myMotor2 = AFMS.getMotor(4);
    myMotor1->setSpeed(speed);
    myMotor2->setSpeed(speed);

    // Moottorien pysäytys aluksi
    stopMotors();

    // WiFi-asetukset
    WiFi.mode(WIFI_STA);

    // ESP-NOW:n alustus
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    // Datan vastaanotto callback-funktion rekisteröinti
    esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
    unsigned long currentMillis = millis();
    
    // Ultraäänisensorin etäisyyden mittaus tietyin väliajoin
    if (currentMillis - previousUltrasonicMillis >= ultrasonicInterval) {
        previousUltrasonicMillis = currentMillis;

        unsigned int frontDistance = frontSonar.ping_cm();
        unsigned int backDistance = backSonar.ping_cm();

        if (frontDistance > 0 && frontDistance < 10) {
            moveBackwardShort();
        } else if (backDistance > 0 && backDistance < 10) {
            moveForwardShort();
        }
    }
    // Käsitellään joystickin tiedot
    processJoystickData();
}

// Blynk-widgetien käsittelyt
BLYNK_WRITE(V0) {
    speed = param.asInt();
    myMotor1->setSpeed(speed);
    myMotor2->setSpeed(speed);
}

BLYNK_WRITE(V1) {
    int pinValue = param.asInt();
    if (pinValue == 1) {
        Serial.println("Moving forward");
        myMotor1->run(FORWARD);
        myMotor2->run(FORWARD);
    } else {
        stopMotors();
    }
}

BLYNK_WRITE(V2) {
    int pinValue = param.asInt();
    if (pinValue == 1) {
        Serial.println("Moving backward");
        myMotor1->run(BACKWARD);
        myMotor2->run(BACKWARD);
    } else {
        stopMotors();
    }
}

BLYNK_WRITE(V3) {
    int pinValue = param.asInt();
    if (pinValue == 1) {
        Serial.println("Turning left");
        myMotor1->run(BACKWARD);
        myMotor2->run(FORWARD);
    } else {
        stopMotors();
    }
}

BLYNK_WRITE(V4) {
    int pinValue = param.asInt();
    if (pinValue == 1) {
        Serial.println("Turning right");
        myMotor1->run(FORWARD);
        myMotor2->run(BACKWARD);
    } else {
        stopMotors();
    }
}

// Moottorien pysäytys
void stopMotors() {
    Serial.println("Stopping");
    myMotor1->run(RELEASE);
    myMotor2->run(RELEASE);
}

// Lyhyt liike taaksepäin esteen havaittaessa edessä
void moveBackwardShort() {
    Serial.println("Obstacle detected in front! Moving backward briefly.");
    myMotor1->run(BACKWARD);
    myMotor2->run(BACKWARD);
    delay(200);
    stopMotors();
}

// Lyhyt liike eteenpäin esteen havaittaessa takana
void moveForwardShort() {
    Serial.println("Obstacle detected in the back! Moving forward briefly.");
    myMotor1->run(FORWARD);
    myMotor2->run(FORWARD);
    delay(200);
    stopMotors();
}

// Joystick-tietojen käsittely
void processJoystickData() {
    int x = incomingData.xAxis;
    int y = incomingData.yAxis;
    bool buttonPressed = incomingData.buttonPressed;

    Serial.print("Joystick x: ");
    Serial.print(x);
    Serial.print(" y: ");
    Serial.print(y);
    Serial.print(" buttonPressed: ");
    Serial.println(buttonPressed);

    // Jos nappi painettu, pysäytä moottorit
    if (buttonPressed) {
        stopMotors();
        return;
    }

    // Liikkuminen joystickin asentojen mukaan
    if (x < 85) {
        Serial.println("Turning left");
        myMotor1->run(BACKWARD);
        myMotor2->run(FORWARD);
    } else if (x > 170) {
        Serial.println("Turning right");
        myMotor1->run(FORWARD);
        myMotor2->run(BACKWARD);
    } else if (y < 85) {
        Serial.println("Moving backward");
        myMotor1->run(BACKWARD);
        myMotor2->run(BACKWARD);
    } else if (y > 170) {
        Serial.println("Moving forward");
        myMotor1->run(FORWARD);
        myMotor2->run(FORWARD);
    } else {
        stopMotors();
    }
}