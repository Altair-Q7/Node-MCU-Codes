/*****************************************************************
 WiFi Network Scanner
 NodeMCU 12E (ESP8266) & ESP32 DevKit V1 — dual compatible
 ─────────────────────────────────────────────────────────────────
 Scans for nearby WiFi networks and prints details to Serial.
 Built-in LED gives visual feedback without needing Serial open.
*****************************************************************/

// ═══════════════════════════════════════════════════════════════
//  Board Detection
// ═══════════════════════════════════════════════════════════════
#if defined(ESP32)
  #include <WiFi.h>
  #define LED_PIN           2       // GPIO2 = built-in LED, active HIGH
  #define LED_ON            HIGH
  #define LED_OFF           LOW
  #define BOARD_NAME        "ESP32 DevKit V1"
  #define CHIP_MODEL        ESP.getChipModel()
  #define HAS_CHIP_MODEL    1       // ESP32 API supports getChipModel()

#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
  #define LED_PIN           2       // GPIO2 = built-in LED, ACTIVE LOW
  #define LED_ON            LOW
  #define LED_OFF           HIGH
  #define BOARD_NAME        "NodeMCU 12E (ESP8266)"
  #define CHIP_MODEL        "ESP8266"
  #define HAS_CHIP_MODEL    0       // ESP8266 has no getChipModel()

#else
  #error "Unsupported board. Use ESP8266 or ESP32."
#endif

// ═══════════════════════════════════════════════════════════════
//  Configuration
// ═══════════════════════════════════════════════════════════════
#define SCAN_INTERVAL_MS  5000    // Delay between scans (ms)
#define SERIAL_BAUD_RATE  115200

// ═══════════════════════════════════════════════════════════════
//  LED Helpers
// ═══════════════════════════════════════════════════════════════

void ledBlink(int times = 1, int onMs = 80, int offMs = 100) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, LED_ON);
    delay(onMs);
    digitalWrite(LED_PIN, LED_OFF);
    if (i < times - 1) delay(offMs);
  }
}

// Long steady pulse — scan in progress
void ledScanPulse() {
  digitalWrite(LED_PIN, LED_ON);
  delay(300);
  digitalWrite(LED_PIN, LED_OFF);
}

// SOS — scan error
void ledError() {
  ledBlink(3, 80, 80);   // · · ·
  delay(200);
  ledBlink(3, 300, 100); // — — —
  delay(200);
  ledBlink(3, 80, 80);   // · · ·
}

// ═══════════════════════════════════════════════════════════════
//  Signal & Encryption Helpers
// ═══════════════════════════════════════════════════════════════

String getSignalQuality(int rssi) {
  if (rssi >= -50) return "Excellent  [####]";
  if (rssi >= -60) return "Good       [### ]";
  if (rssi >= -70) return "Fair       [##  ]";
  if (rssi >= -80) return "Weak       [#   ]";
  return             "Very Weak  [    ]";
}

// ESP32 and ESP8266 use different encryption enums —
// we wrap both into one function using board-specific types
String getEncryptionType(int enc) {
#if defined(ESP32)
  // ESP32 uses wifi_auth_mode_t with WPA3 support
  switch ((wifi_auth_mode_t)enc) {
    case WIFI_AUTH_OPEN:            return "Open (None)";
    case WIFI_AUTH_WEP:             return "WEP";
    case WIFI_AUTH_WPA_PSK:         return "WPA-PSK";
    case WIFI_AUTH_WPA2_PSK:        return "WPA2-PSK";
    case WIFI_AUTH_WPA_WPA2_PSK:    return "WPA/WPA2-PSK";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-Enterprise";
    case WIFI_AUTH_WPA3_PSK:        return "WPA3-PSK";
    case WIFI_AUTH_WPA2_WPA3_PSK:   return "WPA2/WPA3-PSK";
    default:                        return "Unknown";
  }
#elif defined(ESP8266)
  // ESP8266 uses older ENC_TYPE_* constants
  switch (enc) {
    case ENC_TYPE_NONE: return "Open (None)";
    case ENC_TYPE_WEP:  return "WEP";
    case ENC_TYPE_TKIP: return "WPA-PSK";
    case ENC_TYPE_CCMP: return "WPA2-PSK";
    case ENC_TYPE_AUTO: return "WPA/WPA2 (Auto)";
    default:            return "Unknown";
  }
#endif
}

// Format a raw BSSID byte array as XX:XX:XX:XX:XX:XX
String formatBSSID(uint8_t* bssid) {
  char buf[18];
  snprintf(buf, sizeof(buf),
    "%02X:%02X:%02X:%02X:%02X:%02X",
    bssid[0], bssid[1], bssid[2],
    bssid[3], bssid[4], bssid[5]
  );
  return String(buf);
}

// ═══════════════════════════════════════════════════════════════
//  Print Utilities
// ═══════════════════════════════════════════════════════════════

void printDivider(char c, int len) {
  for (int i = 0; i < len; i++) Serial.print(c);
  Serial.println();
}

void printRow(const char* label, String value) {
  Serial.print("  ");
  Serial.print(label);
  Serial.println(value);
}

// ═══════════════════════════════════════════════════════════════
//  Setup
// ═══════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(SERIAL_BAUD_RATE);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LED_OFF);

  delay(500);

  // 5 rapid blinks = board booted and code is running
  // (visible even before Serial Monitor is open)
  ledBlink(5, 50, 50);

  Serial.println();
  printDivider('=', 52);
  Serial.println("          WiFi Scanner");
  printDivider('=', 52);

  // Station mode required for scanning on both chips
  WiFi.mode(WIFI_STA);

#if defined(ESP32)
  WiFi.disconnect(true);    // Also clears saved credentials on ESP32
#else
  WiFi.disconnect();
#endif

  delay(100);

  // ── Chip info ─────────────────────────────────────────────────
  // ESP32 exposes getChipModel(); ESP8266 does not, so we use the
  // define we set at the top as a fallback string.
  Serial.println();
  Serial.print("  Board      : "); Serial.println(BOARD_NAME);
  Serial.print("  Chip       : "); Serial.println(CHIP_MODEL);
  Serial.print("  CPU Freq   : "); Serial.print(ESP.getCpuFreqMHz()); Serial.println(" MHz");
  Serial.print("  Flash Size : "); Serial.print(ESP.getFlashChipSize() / 1024); Serial.println(" KB");
  Serial.print("  Free Heap  : "); Serial.print(ESP.getFreeHeap()); Serial.println(" bytes");
  Serial.print("  LED Pin    : GPIO "); Serial.println(LED_PIN);
  Serial.println();

  // LED legend — printed once at boot for reference
  Serial.println("  LED Patterns:");
  Serial.println("    5x rapid      → Booted OK");
  Serial.println("    1x long pulse → Scan in progress");
  Serial.println("    Nx blink      → N networks found (max 5)");
  Serial.println("    2x slow       → No networks found");
  Serial.println("    SOS           → Scan error");
  Serial.println("    1x tiny/sec   → Idle heartbeat");

  printDivider('=', 52);
  Serial.println();

  ledBlink(1, 200);
  Serial.println("  Ready. Starting first scan...\n");
}

// ═══════════════════════════════════════════════════════════════
//  Main Loop
// ═══════════════════════════════════════════════════════════════
void loop() {

  Serial.println(">> Scanning...");

  // Long LED pulse so you can see the exact moment scanning starts
  ledScanPulse();

  // ESP32 supports show_hidden parameter; ESP8266 does not
#if defined(ESP32)
  int networks = WiFi.scanNetworks(/*async=*/false, /*show_hidden=*/true);
#else
  int networks = WiFi.scanNetworks();
#endif

  printDivider('-', 52);

  if (networks < 0) {
    Serial.println("  ! Scan error. Retrying next cycle.");
    ledError();

  } else if (networks == 0) {
    Serial.println("  No networks found.");
    ledBlink(2, 400, 200);  // 2x slow blink

  } else {
    // Blink once per network found, capped at 5 so it doesn't drag
    ledBlink(min(networks, 5), 60, 60);

    Serial.print("  Found ");
    Serial.print(networks);
    Serial.println(" network(s):\n");

    for (int i = 0; i < networks; i++) {
      String ssid       = WiFi.SSID(i);
      int    rssi       = WiFi.RSSI(i);
      int    channel    = WiFi.channel(i);
      String bssid      = formatBSSID(WiFi.BSSID(i));
      String encryption = getEncryptionType((int)WiFi.encryptionType(i));
      String quality    = getSignalQuality(rssi);

      // Empty SSID = hidden network (not broadcasting its name)
      if (ssid.length() == 0) ssid = "[Hidden Network]";

      Serial.print("  #"); Serial.println(i + 1);
      printRow("SSID       : ", ssid);
      printRow("BSSID      : ", bssid);
      printRow("Signal     : ", String(rssi) + " dBm  " + quality);
      printRow("Channel    : ", String(channel));

      // Band detection: channels 1-13 = 2.4 GHz, 36+ = 5 GHz
      // ESP8266 is 2.4 GHz only, but we show it anyway for consistency
      printRow("Band       : ", (channel > 14) ? "5 GHz" : "2.4 GHz");
      printRow("Security   : ", encryption);
      Serial.println();
    }
  }

  // ── Footer ────────────────────────────────────────────────────
  printDivider('-', 52);
  Serial.print("  Free heap  : ");
  Serial.print(ESP.getFreeHeap());
  Serial.println(" bytes");
  Serial.print("  Next scan in ");
  Serial.print(SCAN_INTERVAL_MS / 1000);
  Serial.println("s...\n");

  // Free scan result memory — important on both chips
  WiFi.scanDelete();

  // ── Idle heartbeat ────────────────────────────────────────────
  // One tiny tick per second during the inter-scan wait so you
  // can see the board is alive without a Serial Monitor open
  int ticks = SCAN_INTERVAL_MS / 1000;
  for (int t = 0; t < ticks; t++) {
    ledBlink(1, 30, 970);   // 30ms ON, 970ms OFF = ~1s per tick
  }
}