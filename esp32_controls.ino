
#include <string>
#include <WiFi.h>
#include "time.h"
#include <cmath>
#include <Wire.h>
#include <SPI.h>
#include <Arduino_JSON.h>
#include <HTTPClient.h>

// Debounced version of digitalRead()
bool digitalReadDebounced(int pin) {
  static unsigned long lastDebounceTime = 0;   // Last time the button state was checked
  static bool lastButtonState = LOW;           // Last stable state of the button
  bool currentButtonState = digitalRead(pin);  // Current state of the button

  const unsigned long debounceDelay = 50;  // 50 milliseconds debounce time

  // Check if the button state has changed
  if (currentButtonState != lastButtonState) {
    lastDebounceTime = millis();  // Reset the debounce timer
  }

  // If the button state has been stable for more than the debounce delay, register the press
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (currentButtonState != lastButtonState) {
      lastButtonState = currentButtonState;  // Update the last stable state
    }
  }

  return lastButtonState;  // Return the stable state of the button
}

const char* ssid = "ISAK-S";
const char* password = "heK7bTGW";
const char* ntpServer = "pool.ntp.org";
unsigned long epochTime;
long gmtOffset_sec = 32400;
int daylightOffset_sec = 0;

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
// class for wifi connection and NTP time access
class WIFI {
public:
  const char* ssid;
  const char* password;
  const char* ntpServer;
  String server = "http://192.168.6.115:8000";
  long gmtOffset_sec;
  int daylightOffset_sec;

  WIFI(const char* ssid, const char* password) {
    this->ssid = ssid;
    this->password = password;
  };

  void init_wifi_connection() {

    WiFi.mode(WIFI_STA);  //Optional
    WiFi.begin(ssid, password);
    Serial.println("\nConnecting");

    while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(100);
    }

    Serial.println("\nConnected to the WiFi network");
    Serial.print("Local ESP32 IP: ");
    Serial.println(WiFi.localIP());
  };

  void init_ntp_time(const char* ntpServer, long gmtOffset_sec, int daylightOffset_sec) {
    this->ntpServer = ntpServer;
    this->gmtOffset_sec = gmtOffset_sec;
    this->daylightOffset_sec = daylightOffset_sec;
  };

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

  unsigned long get_time() {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    time_t now;
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
      Serial.println("failed to obtain item");
      return (0);
    }
    time(&now);
    return now;
  };
};

//Stepper Motor Setup
#define direction_pin 33
#define step_pin 32
#define enable_pin 34
#define calibrating_button 18
#define up_pin 16
#define down_pin 17
int step_per_rev = 200;
int gear_ratio = 5.536;

class STEPPER_MOTOR {
private:
  int step_per_rev;
  float gear_ratio;
  float step_per_deg;
  float current_degree = 0;

  bool calibrated = false;

public:
  STEPPER_MOTOR(int step_per_rev, float gear_ratio) {
    this->step_per_rev = step_per_rev;
    this->gear_ratio = gear_ratio;
    step_per_deg = step_per_rev * gear_ratio / 360;
  };

  void move(float deg) {
    int step = round(deg * step_per_deg);
    digitalWrite(enable_pin, LOW);

    bool direction = (deg < 0) ? 1 : 0;
    for (int i = 0; i < abs(step); i++) {
      digitalWrite(direction_pin, direction);
      digitalWrite(step_pin, HIGH);
      delayMicroseconds(500);
      digitalWrite(step_pin, LOW);
      delayMicroseconds(500);
    };

    digitalWrite(enable_pin, HIGH);
    current_degree += deg;
  };

  void set_degree(int deg) {
    move(deg - current_degree);
  }

  void calibrate_stepper_motor() {
    if (calibrated == false) {
      while (digitalReadDebounced(calibrating_button) == LOW) {
        move(-1);
      };
      current_degree = 0;
      calibrated = true;
    };
  };

  bool manual_control_check() {
    bool mode;

    if (digitalRead(up_pin) == HIGH) {
      move(10);
      delay(178);
      mode = false;
    } else if (digitalRead(down_pin) == HIGH) {
      move(-10);
      delay(178);
      mode = false;
    };

    return mode;
  };
};

WIFI my_wifi(ssid = ssid, password = password);
STEPPER_MOTOR stepper_motor(step_per_rev = step_per_rev, gear_ratio = gear_ratio);

void setup() {
  Serial.begin(9600);
  // setup wifi and time
  my_wifi.init_wifi_connection();
  Serial.print("started");
  my_wifi.init_ntp_time(ntpServer = ntpServer, gmtOffset_sec = gmtOffset_sec, daylightOffset_sec = daylightOffset_sec);
  epochTime = my_wifi.get_time();
  Serial.print(epochTime);
};

float ideal_temps;
int wake_time;
int sleep_time;
int time_of_day;
void loop() {
  Serial.print("time:");
  time_of_day = my_wifi.get_time()%86400;
  Serial.println(time_of_day);
  if (time_of_day == sleep_time) {
    Serial.println("motor turned off");
  } else if (time_of_day == wake_time) {
    Serial.println("motor turned on");
  }

  ideal_temps = my_wifi.get_temp();
  wake_time = my_wifi.get_wake();
  sleep_time = my_wifi.get_sleep();
  delay(10000);
};
