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

String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;
  // Your Domain name with URL path or IP address with path
  http.begin(client, serverName);
  
  // If you need Node-RED/server authentication, insert user and password below
  //http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD");
  
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();
  return payload;
}

float get_temp() {
Serial.print("Getting temperature");
float ideal_temps = httpGETRequest("http://192.168.6.115:8000/get_ideal_temp/2").toFloat();
Serial.println(ideal_temps);
return ideal_temps;
};

int get_wake() {
Serial.println("Getting wake time");
int wake_time = httpGETRequest("http://192.168.6.115:8000/get_wake/2").toInt();
Serial.println(wake_time);
return wake_time;
};

int get_sleep() {
Serial.println("Getting sleep time");
int sleep_time = httpGETRequest("http://192.168.6.115:8000/get_sleep/2").toInt();
Serial.println(sleep_time);
return sleep_time;
};
