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
    wifi_setup("ISAK-S", "heK7bTGW"); // insert the rigth things
    get_ntp_time( "pool.ntp.org", 32400, 0);
}

int get_ntp_time(
    const char* ntpServer,
    long gmtOffset_sec,
    int daylightOffset_sec
) {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    time_t now;
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain the time");
        return (0);
    }
    time(&now);
    return now;
}

void loop() {
  Serial.println(get_ntp_time( "pool.ntp.org", 32400, 0));
  delay(100);
}
