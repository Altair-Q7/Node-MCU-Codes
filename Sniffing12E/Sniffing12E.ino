#include <ESP8266WiFi.h>

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("ESP8266 WiFi Scanner Starting...");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
}

void loop() {

  Serial.println("Scanning...");
  int networks = WiFi.scanNetworks();

  if (networks == 0) {
    Serial.println("No networks found.");
  } 
  else {

    Serial.print(networks);
    Serial.println(" networks detected\n");

    for (int i = 0; i < networks; i++) {

      Serial.print(i + 1);
      Serial.print(": ");

      Serial.print(WiFi.SSID(i));
      Serial.print(" | RSSI: ");
      Serial.print(WiFi.RSSI(i));

      Serial.print(" | Channel: ");
      Serial.print(WiFi.channel(i));

      Serial.print(" | Encryption: ");

      switch (WiFi.encryptionType(i)) {
        case ENC_TYPE_NONE:
          Serial.println("Open");
          break;
        case ENC_TYPE_WEP:
          Serial.println("WEP");
          break;
        case ENC_TYPE_TKIP:
          Serial.println("WPA");
          break;
        case ENC_TYPE_CCMP:
          Serial.println("WPA2");
          break;
        default:
          Serial.println("Unknown");
      }
    }
  }

  Serial.println("\nScan complete.\n");
  delay(5000);
}