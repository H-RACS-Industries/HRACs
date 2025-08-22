#include <WiFi.h>

void setup() {
    wifi_setup(); // insert the rigth things
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
