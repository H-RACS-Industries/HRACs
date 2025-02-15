#include <string>
#include <WiFi.h>
#include "time.h"
#include <cmath> 
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>

// Debounced version of digitalRead()
bool digitalReadDebounced(int pin) {
    static unsigned long lastDebounceTime = 0;  // Last time the button state was checked
    static bool lastButtonState = LOW;           // Last stable state of the button
    bool currentButtonState = digitalRead(pin);  // Current state of the button

    const unsigned long debounceDelay = 50;      // 50 milliseconds debounce time

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

// class for wifi connection and NTP time access
class WIFI {
  public :
      const char* ssid;
      const char* password;
      const char* ntpServer;
      long gmtOffset_sec;
      int daylightOffset_sec;

      WIFI(const char* ssid, const char* password) {
        this->ssid = ssid;
        this->password = password;
      };

      void init_wifi_connection() {
          Serial.begin(115200);
          delay(1000);

          WiFi.mode(WIFI_STA); //Optional
          WiFi.begin(ssid, password);
          Serial.println("\nConnecting");

          while(WiFi.status() != WL_CONNECTED){
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

      unsigned long get_time() {
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        time_t now;
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo)) {
          Serial.println("failed to obtain item");
          return(0);
        }
        time(&now);
        return now;
      };
};

#define BMP_SCK 18
#define BMP_MISO 19
#define BMP_MOSI 23
#define BMP_CS 5


void test_bmp(){
  Adafruit_BMP280 bmp; // setup BMP
  Serial.begin(9600);
  while ( !Serial ) delay(100);   // wait for native usb
  Serial.println(F("BMP280 test"));
  unsigned status;
  //status = bmp.begin(BMP280_ADDRESS_ALT, BMP280_CHIPID);
  status = bmp.begin(0x76);
  if (!status) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring or "
                      "try a different address!"));
    Serial.print("SensorID was: 0x"); Serial.println(bmp.sensorID(),16);
    Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
    Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
    Serial.print("        ID of 0x60 represents a BME 280.\n");
    Serial.print("        ID of 0x61 represents a BME 680.\n");
    while (1) delay(10);
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
    STEPPER_MOTOR(int step_per_rev, float gear_ratio){
      this-> step_per_rev = step_per_rev;
      this-> gear_ratio = gear_ratio;
      step_per_deg = step_per_rev * gear_ratio / 360;
    };

    void move(float deg) {
      int step = round(deg * step_per_deg);
      digitalWrite(enable_pin, LOW);

      bool direction = (deg < 0) ? 1 : 0;
      Serial.println(direction);

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
  // setup wifi and time

  my_wifi.init_wifi_connection();
  my_wifi.init_ntp_time(ntpServer = ntpServer, gmtOffset_sec = gmtOffset_sec, daylightOffset_sec = daylightOffset_sec);
  epochTime = my_wifi.get_time();
  Serial.print(epochTime);

  // test BMP280 connection
  test_bmp();

  // setup stepper motor controls
  stepper_motor.calibrate_stepper_motor();
};
void loop(){
  stepper_motor.manual_control_check();
};
