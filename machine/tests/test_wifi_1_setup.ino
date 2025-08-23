#include <WiFi.h>

void wifi_setup(const char* ssid, const char* password) {
  WiFi.mode(WIFI_STA);              // ensure station mode
  WiFi.begin(ssid, password);
  Serial.println("\nConnecting");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("\nConnected to WiFi");
  Serial.print("IP is: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);             // match Serial Monitor baud
  delay(200);                       // give time for Serial to come up
  Serial.println("starting");

  wifi_setup("ISAK-S", "heK7bTGW"); // call your function
}

void loop() {
  // you can add periodic prints if you want to see activity
  static unsigned long last = 0;
  if (millis() - last > 2000) {
    last = millis();
    Serial.print("WiFi status: ");
    Serial.println(WiFi.status() == WL_CONNECTED ? "OK" : "DISCONNECTED");
  }
}
