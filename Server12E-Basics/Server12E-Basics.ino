#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* ssid = "NodeMCU_Server";
const char* password = "12345678";

ESP8266WebServer server(80);

void handleRoot() {

  String page = "<html><body>";
  page += "<h2>Send Message to NodeMCU</h2>";
  page += "<form action='/send' method='GET'>";
  page += "<input type='text' name='msg'>";
  page += "<input type='submit' value='Send'>";
  page += "</form>";
  page += "</body></html>";

  server.send(200, "text/html", page);
}

void handleMessage() {

  if (server.hasArg("msg")) {

    String message = server.arg("msg");

    Serial.print("Message received: ");
    Serial.println(message);

  }

  server.send(200, "text/html",
  "<h3>Message received!</h3><a href='/'>Back</a>");
}

void setup() {

  Serial.begin(115200);

  WiFi.softAP(ssid, password);

  Serial.println();
  Serial.println("Access Point Started");

  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/send", handleMessage);

  server.begin();

}

void loop() {

  server.handleClient();

}