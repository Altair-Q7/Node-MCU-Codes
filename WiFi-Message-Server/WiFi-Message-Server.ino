// ═══════════════════════════════════════════════════════════
//  WiFi Message Server — NodeMCU 12E & ESP32 DevKit V1
//  Auto-detects board via preprocessor macros
// ═══════════════════════════════════════════════════════════

// ── Board detection ─────────────────────────────────────────
#if defined(ESP32)
  #include <WiFi.h>
  #include <WebServer.h>
  #define LED_PIN       2       // GPIO2 = built-in LED on ESP32 DevKit V1
  #define LED_ON        HIGH
  #define LED_OFF       LOW
  #define BOARD_NAME    "ESP32 DevKit V1"
  WebServer server(80);

#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
  #define LED_PIN       2       // GPIO2 = built-in LED on NodeMCU 12E
  #define LED_ON        LOW     // NodeMCU LED is ACTIVE LOW
  #define LED_OFF       HIGH
  #define BOARD_NAME    "NodeMCU 12E (ESP8266)"
  ESP8266WebServer server(80);

#else
  #error "Unsupported board. Please use ESP8266 or ESP32."
#endif

// ─────────────────────────────────────────────
//  Configuration — edit these
// ─────────────────────────────────────────────
const char* SSID        = "WiFi_Server";
const char* PASSWORD    = "12345678";     // Min 8 chars for WPA2
const int   MAX_HISTORY = 10;             // Max messages to keep in memory

// ─────────────────────────────────────────────
//  Message History
// ─────────────────────────────────────────────
struct Message {
  String text;
  String timestamp;   // uptime-based since we have no RTC
  int    index;
};

Message history[MAX_HISTORY];
int     historyCount  = 0;
int     totalReceived = 0;

// Store a new message, rolling oldest out when full
void storeMessage(const String& text) {
  // Shift history down when full
  if (historyCount == MAX_HISTORY) {
    for (int i = 0; i < MAX_HISTORY - 1; i++) {
      history[i] = history[i + 1];
    }
    historyCount = MAX_HISTORY - 1;
  }

  unsigned long s = millis() / 1000;
  char ts[16];
  snprintf(ts, sizeof(ts), "%02lu:%02lu:%02lu",
    s / 3600, (s % 3600) / 60, s % 60);

  history[historyCount].text      = text;
  history[historyCount].timestamp = String(ts);
  history[historyCount].index     = ++totalReceived;
  historyCount++;
}

// ─────────────────────────────────────────────
//  LED Helpers
// ─────────────────────────────────────────────
void ledBlink(int times = 1, int onMs = 80, int offMs = 100) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, LED_ON);
    delay(onMs);
    digitalWrite(LED_PIN, LED_OFF);
    if (i < times - 1) delay(offMs);
  }
}

// ─────────────────────────────────────────────
//  HTML Page Builder
// ─────────────────────────────────────────────
String buildPage(const String& alertHtml = "") {

  // Build history rows
  String rows = "";
  if (historyCount == 0) {
    rows = "<tr><td colspan='3' class='empty'>No messages yet</td></tr>";
  } else {
    // Show newest first
    for (int i = historyCount - 1; i >= 0; i--) {
      rows += "<tr>";
      rows += "<td class='idx'>#" + String(history[i].index) + "</td>";
      rows += "<td class='ts'>"  + history[i].timestamp      + "</td>";
      rows += "<td class='msg'>" + history[i].text            + "</td>";
      rows += "</tr>";
    }
  }

  // Connected clients count
  #if defined(ESP32)
    int clients = WiFi.softAPgetStationNum();
  #else
    int clients = WiFi.softAPgetStationNum();
  #endif

  // Uptime string
  unsigned long s = millis() / 1000;
  char uptime[16];
  snprintf(uptime, sizeof(uptime), "%02lu:%02lu:%02lu",
    s / 3600, (s % 3600) / 60, s % 60);

  String html = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>ESP Message Server</title>
<style>
  @import url('https://fonts.googleapis.com/css2?family=Share+Tech+Mono&family=Exo+2:wght@300;600;800&display=swap');

  :root {
    --bg:       #0a0e17;
    --panel:    #0f1623;
    --border:   #1e2d45;
    --accent:   #00e5ff;
    --accent2:  #ff6b35;
    --green:    #39ff14;
    --text:     #c8d8e8;
    --muted:    #4a6080;
    --radius:   8px;
  }

  * { box-sizing: border-box; margin: 0; padding: 0; }

  body {
    background: var(--bg);
    color: var(--text);
    font-family: 'Exo 2', sans-serif;
    font-weight: 300;
    min-height: 100vh;
    padding: 24px 16px;
    background-image:
      radial-gradient(ellipse at 20% 10%, rgba(0,229,255,0.04) 0%, transparent 60%),
      radial-gradient(ellipse at 80% 90%, rgba(255,107,53,0.04) 0%, transparent 60%);
  }

  header {
    text-align: center;
    margin-bottom: 28px;
  }

  header h1 {
    font-size: clamp(1.4rem, 5vw, 2rem);
    font-weight: 800;
    letter-spacing: 0.08em;
    text-transform: uppercase;
    color: var(--accent);
    text-shadow: 0 0 20px rgba(0,229,255,0.4);
  }

  header .board-tag {
    display: inline-block;
    margin-top: 6px;
    font-family: 'Share Tech Mono', monospace;
    font-size: 0.75rem;
    color: var(--muted);
    letter-spacing: 0.12em;
    border: 1px solid var(--border);
    padding: 3px 10px;
    border-radius: 20px;
  }

  /* ── Status bar ── */
  .status-bar {
    display: flex;
    gap: 12px;
    flex-wrap: wrap;
    justify-content: center;
    margin-bottom: 24px;
  }

  .stat {
    background: var(--panel);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    padding: 10px 18px;
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 2px;
    min-width: 90px;
  }

  .stat .val {
    font-family: 'Share Tech Mono', monospace;
    font-size: 1.1rem;
    color: var(--accent);
  }

  .stat .lbl {
    font-size: 0.65rem;
    letter-spacing: 0.1em;
    text-transform: uppercase;
    color: var(--muted);
  }

  .dot {
    display: inline-block;
    width: 8px; height: 8px;
    border-radius: 50%;
    background: var(--green);
    box-shadow: 0 0 6px var(--green);
    animation: pulse 2s ease-in-out infinite;
    margin-right: 5px;
  }

  @keyframes pulse {
    0%,100% { opacity: 1; }
    50%      { opacity: 0.3; }
  }

  /* ── Form card ── */
  .card {
    background: var(--panel);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    padding: 22px;
    margin-bottom: 20px;
  }

  .card h2 {
    font-size: 0.8rem;
    font-weight: 600;
    letter-spacing: 0.15em;
    text-transform: uppercase;
    color: var(--muted);
    margin-bottom: 14px;
  }

  .input-row {
    display: flex;
    gap: 10px;
  }

  input[type="text"] {
    flex: 1;
    background: var(--bg);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    color: var(--text);
    font-family: 'Share Tech Mono', monospace;
    font-size: 0.95rem;
    padding: 10px 14px;
    outline: none;
    transition: border-color 0.2s;
  }

  input[type="text"]:focus {
    border-color: var(--accent);
    box-shadow: 0 0 0 3px rgba(0,229,255,0.08);
  }

  button {
    background: var(--accent);
    color: #000;
    border: none;
    border-radius: var(--radius);
    padding: 10px 22px;
    font-family: 'Exo 2', sans-serif;
    font-weight: 700;
    font-size: 0.85rem;
    letter-spacing: 0.08em;
    text-transform: uppercase;
    cursor: pointer;
    transition: background 0.15s, transform 0.1s;
    white-space: nowrap;
  }

  button:hover  { background: #33ecff; }
  button:active { transform: scale(0.97); }

  .btn-clear {
    background: transparent;
    color: var(--accent2);
    border: 1px solid var(--accent2);
    margin-top: 10px;
    width: 100%;
  }

  .btn-clear:hover { background: rgba(255,107,53,0.1); }

  /* ── Alert ── */
  .alert {
    background: rgba(57,255,20,0.08);
    border: 1px solid rgba(57,255,20,0.3);
    border-left: 3px solid var(--green);
    border-radius: var(--radius);
    padding: 10px 14px;
    margin-bottom: 8px;
    font-family: 'Share Tech Mono', monospace;
    font-size: 0.85rem;
    color: var(--green);
  }

  /* ── History table ── */
  .table-wrap { overflow-x: auto; }

  table {
    width: 100%;
    border-collapse: collapse;
    font-size: 0.88rem;
  }

  th {
    text-align: left;
    padding: 8px 12px;
    font-size: 0.65rem;
    letter-spacing: 0.12em;
    text-transform: uppercase;
    color: var(--muted);
    border-bottom: 1px solid var(--border);
  }

  td {
    padding: 10px 12px;
    border-bottom: 1px solid rgba(30,45,69,0.6);
    vertical-align: top;
  }

  tr:last-child td { border-bottom: none; }

  tr:hover td { background: rgba(0,229,255,0.03); }

  td.idx { font-family: 'Share Tech Mono', monospace; color: var(--muted); width: 50px; }
  td.ts  { font-family: 'Share Tech Mono', monospace; color: var(--accent2); white-space: nowrap; width: 90px; }
  td.msg { word-break: break-word; }
  td.empty { text-align: center; color: var(--muted); padding: 24px; font-style: italic; }

  footer {
    text-align: center;
    margin-top: 28px;
    font-size: 0.7rem;
    color: var(--muted);
    letter-spacing: 0.08em;
    font-family: 'Share Tech Mono', monospace;
  }

  /* Auto-refresh indicator */
  .refresh-bar {
    height: 2px;
    background: var(--border);
    border-radius: 1px;
    margin-top: 12px;
    overflow: hidden;
  }
  .refresh-bar-inner {
    height: 100%;
    background: var(--accent);
    width: 0%;
    animation: refill 10s linear infinite;
  }
  @keyframes refill { from { width:0% } to { width:100% } }
</style>
</head>
<body>

<header>
  <h1><span class="dot"></span>ESP Message Server</h1>
  <div class="board-tag">)rawhtml";

  html += BOARD_NAME;

  html += R"rawhtml( &nbsp;|&nbsp; )rawhtml";
  html += WiFi.softAPIP().toString();

  html += R"rawhtml(</div>
</header>

<div class="status-bar">
  <div class="stat">
    <span class="val">)rawhtml";
  html += String(clients);
  html += R"rawhtml(</span><span class="lbl">Clients</span>
  </div>
  <div class="stat">
    <span class="val">)rawhtml";
  html += String(totalReceived);
  html += R"rawhtml(</span><span class="lbl">Received</span>
  </div>
  <div class="stat">
    <span class="val">)rawhtml";
  html += String(uptime);
  html += R"rawhtml(</span><span class="lbl">Uptime</span>
  </div>
  <div class="stat">
    <span class="val">)rawhtml";
  html += String(ESP.getFreeHeap() / 1024);
  html += R"rawhtml( KB</span><span class="lbl">Free Heap</span>
  </div>
</div>

<div class="card">
  <h2>Send Message</h2>
  )rawhtml";

  html += alertHtml;

  html += R"rawhtml(
  <form action="/send" method="GET">
    <div class="input-row">
      <input type="text" name="msg" placeholder="Type a message..." maxlength="200" autofocus autocomplete="off">
      <button type="submit">Send</button>
    </div>
  </form>
  <form action="/clear" method="GET">
    <button type="submit" class="btn-clear">Clear History</button>
  </form>
</div>

<div class="card">
  <h2>Message History (newest first)</h2>
  <div class="table-wrap">
    <table>
      <thead>
        <tr>
          <th>#</th>
          <th>Uptime</th>
          <th>Message</th>
        </tr>
      </thead>
      <tbody>
        )rawhtml";

  html += rows;

  html += R"rawhtml(
      </tbody>
    </table>
  </div>
  <div class="refresh-bar"><div class="refresh-bar-inner"></div></div>
</div>

<footer>Auto-refreshes every 10s &nbsp;·&nbsp; )rawhtml";
  html += BOARD_NAME;
  html += R"rawhtml(</footer>

<script>
  // Auto-refresh every 10 seconds to update stats & history
  setTimeout(() => location.reload(), 10000);
</script>

</body>
</html>
)rawhtml";

  return html;
}// ═══════════════════════════════════════════════════════════
//  WiFi Message Server — NodeMCU 12E & ESP32 DevKit V1
//  Auto-detects board via preprocessor macros
// ═══════════════════════════════════════════════════════════

// ── Board detection ─────────────────────────────────────────
#if defined(ESP32)
  #include <WiFi.h>
  #include <WebServer.h>
  #define LED_PIN       2       // GPIO2 = built-in LED on ESP32 DevKit V1
…}

// ─────────────────────────────────────────────
//  Route Handlers
// ─────────────────────────────────────────────

// GET /  → main page
void handleRoot() {
  ledBlink(1, 30);
  server.send(200, "text/html", buildPage());
}

// GET /send?msg=...  → receive message
void handleSend() {
  String alert = "";

  if (server.hasArg("msg") && server.arg("msg").length() > 0) {
    String msg = server.arg("msg");

    // Basic sanitisation — strip angle brackets to prevent XSS
    msg.replace("<", "&lt;");
    msg.replace(">", "&gt;");

    storeMessage(msg);

    Serial.print("[MSG #");
    Serial.print(totalReceived);
    Serial.print("] ");
    Serial.println(msg);

    // 2 quick blinks = message received
    ledBlink(2, 60, 60);

    alert = "<div class='alert'>&#10003; Message stored: " + msg + "</div>";

  } else {
    alert = "<div class='alert' style='border-left-color:#ff6b35;color:#ff6b35;'>&#9888; Empty message ignored.</div>";
  }

  server.send(200, "text/html", buildPage(alert));
}

// GET /clear  → wipe history
void handleClear() {
  historyCount  = 0;
  totalReceived = 0;
  Serial.println("[INFO] Message history cleared.");
  ledBlink(3, 40, 40);
  server.send(200, "text/html", buildPage("<div class='alert'>History cleared.</div>"));
}

// GET /status  → plain-text health check (useful for scripting / curl)
void handleStatus() {
  String s = "OK\n";
  s += "board=" + String(BOARD_NAME) + "\n";
  s += "ip="    + WiFi.softAPIP().toString() + "\n";
  s += "total=" + String(totalReceived) + "\n";
  s += "heap="  + String(ESP.getFreeHeap()) + "\n";
  server.send(200, "text/plain", s);
}

// 404 fallback
void handleNotFound() {
  server.send(404, "text/plain", "404 Not Found");
}

// ─────────────────────────────────────────────
//  Setup
// ─────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LED_OFF);

  delay(500);

  // 5 rapid blinks = board booted
  ledBlink(5, 50, 50);

  Serial.println();
  Serial.println("╔══════════════════════════════════════╗");
  Serial.println("║     ESP WiFi Message Server          ║");
  Serial.println("╚══════════════════════════════════════╝");
  Serial.print  ("  Board    : "); Serial.println(BOARD_NAME);

  // Start soft AP
  bool apStarted = WiFi.softAP(SSID, PASSWORD);

  if (!apStarted) {
    Serial.println("! AP failed to start. Halting.");
    // Rapid error blink forever
    while (true) { ledBlink(10, 30, 30); delay(500); }
  }

  Serial.print  ("  SSID     : "); Serial.println(SSID);
  Serial.print  ("  Password : "); Serial.println(PASSWORD);
  Serial.print  ("  IP Addr  : "); Serial.println(WiFi.softAPIP());
  Serial.print  ("  Heap     : "); Serial.print(ESP.getFreeHeap()); Serial.println(" bytes");
  Serial.println();
  Serial.println("  LED Patterns:");
  Serial.println("    5x fast   → Booted OK");
  Serial.println("    1x short  → Page request");
  Serial.println("    2x quick  → Message received");
  Serial.println("    3x quick  → History cleared");
  Serial.println();

  // Register routes
  server.on("/",       handleRoot);
  server.on("/send",   handleSend);
  server.on("/clear",  handleClear);
  server.on("/status", handleStatus);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("  Server started. Connect to WiFi and open http://");
  Serial.println("  " + WiFi.softAPIP().toString());
  Serial.println();

  // 2 slow blinks = server ready
  ledBlink(2, 300, 150);
}

// ─────────────────────────────────────────────
//  Loop
// ─────────────────────────────────────────────
void loop() {
  server.handleClient();

  // Passive heartbeat tick every 3 seconds (non-blocking via millis)
  static unsigned long lastTick = 0;
  if (millis() - lastTick >= 3000) {
    lastTick = millis();
    ledBlink(1, 20);   // Tiny 20ms tick — alive indicator
  }
}