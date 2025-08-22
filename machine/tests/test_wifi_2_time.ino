#include <WiFi.h>

void setup() {
    wifi_setup(); // insert the rigth things
    get_ntp_time();
}

void wifi_setup(
    char* ssid, 
    char* password
    ) {
    
    WiFi.begin(ssid, password);
    Serial.println("\nConnecting");

    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(100);
    }

    Serial.println("\nConnected to WiFi");
    Serial.print("\nIP is:");
    Serial.println(WiFi.localIP());
}

void get_ntp_time(
    const char* ntpServer,
    long gmtOffset_sec,
    int daylightOffset_sec
) {
    configTime(gmtOffset_sec, dayligthOffset_sec, ntpServer);
    time_t now;
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain the time");
        return (0);
    }
    time(&now);
    return now;
}
