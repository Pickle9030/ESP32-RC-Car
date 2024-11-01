/* ---Ohjain--- */
#include <esp_now.h>
#include <WiFi.h>

// Vastaanottimen MAC-osoite
uint8_t broadcastAddress[] = {0x7C, 0x9E, 0xBD, 0xD8, 0x2A, 0x38};

// Rakenne viestin lähettämistä varten
typedef struct struct_message {
  int xAxis;
  int yAxis;
  bool buttonPressed;
} struct_message;

struct_message myData; // Muuttuja viestin tiedoille

esp_now_peer_info_t peerInfo; // ESP-NOW:n peer-tiedot

// Joystickin ja painikkeen pinnit
const int joyXPin = 33;
const int joyYPin = 32;
const int buttonPin = 14;

// Funktio joystickin arvojen muuntamiseen ja deadbandin käsittelyyn
int mapAndAdjustJoystickDeadBandValues(int value, bool reverse)
{
  if (value >= 1910)
  {
    value = map(value, 1910, 4095, 127, 255); // Kartoitus yläpuolelle
  }
  else if (value <= 1906)
  {
    value = map(value, 1906, 0, 127, 0); // Kartoitus alapuolelle
  }
  else
  {
    value = 127; // Deadband-alue
  }

  if (reverse)
  {
    value = 254 - value; // Arvon kääntäminen, jos reverse on true
  }
  return value;
}

// Funktio datan lähettämisen tilan tarkistamiseksi
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup() {
  Serial.begin(115200); // Sarjaliikenteen aloitus

  WiFi.mode(WIFI_STA); // WiFi-tilan asetus

  // ESP-NOW:n alustus
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent); // Lähetyksen callback-funktion rekisteröinti

  // Peer-tietojen asettaminen
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
      
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  pinMode(buttonPin, INPUT_PULLUP); // Painikkeen pinni inputiksi ja pullup-tilaan
}

void loop() {
  // Joystickin ja painikkeen tilan lukeminen
  myData.xAxis = mapAndAdjustJoystickDeadBandValues(analogRead(joyXPin), false);
  myData.yAxis = mapAndAdjustJoystickDeadBandValues(analogRead(joyYPin), false);
  myData.buttonPressed = digitalRead(buttonPin) == LOW;

  // Datan lähetys ESP-NOW:n kautta
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
   
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
  delay(500); // Viive 500 ms ennen seuraavaa lähetysyritystä
}
