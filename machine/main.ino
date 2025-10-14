// connect wifi()✅
// get current time()✅
// setup()
  // setup stepper
  // ✅setup check button
  // ✅ setup slider
  // ✅setup screen
// reset stepper()
// tune pid()
// web requestions()✅
  // get temp✅
  // post temp✅
  // set time✅
  // get current tuning paramters
// pid() -> int
#include <HTTPClient.h>
// check_sleep()
#include <WiFi.h>
int gmtOffset_sec = 32400;
int daylightOffset_sec = 0;
String ntpServer = "pool.ntp.org";
int get_ntp_time(
  String ntpServer,
  int gmtOffset_sec,
  int daylightOffset_sec
) {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
      Serial.println("Failed to obtain the time");
  }
  time(&now);
  return now;
}


String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;
  http.begin(client, serverName);

  int code = http.GET();
  String payload = "{}";

  if (code > 0) {
    Serial.printf("GET [%s] => %d\n", serverName, code);
    payload = http.getString();

    // --- Quick JSON-to-number extractor ---
    // If payload looks like {"temp":24.7} or {"value":22.3}
    int colon = payload.indexOf(':');
    int endBrace = payload.indexOf('}');
    if (colon != -1 && endBrace != -1 && payload.startsWith("{")) {
      String num = payload.substring(colon + 1, endBrace);
      num.trim();
      return num;   // return just the number as string
    }
  } else {
    Serial.printf("GET failed, code=%d\n", code);
  }

  http.end();
  return payload;
}
String httpPOSTRequest(const char* serverName, String postData) {
  WiFiClient client;
  HTTPClient http;

  // Begin connection
  http.begin(client, serverName);
  http.addHeader("Content-Type", "application/json"); // or "application/x-www-form-urlencoded"

  // Send HTTP POST request
  int httpResponseCode = http.POST(postData);

  String payload = "{}";

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }

  // Free resources
  http.end();
  return payload;
}


// Define stepper motor connections:
#define dirPin 32
#define stepPin 33
#define enaPin 14
// Pin for check button
#define checkButtonPin 25

void stepper_setup() {
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  pinMode(enaPin, OUTPUT);
}

void button_setup() {
  pinMode(checkButtonPin, INPUT_PULLUP);
}

void move_motor(bool dir, int steps) {
  // Set direction
  digitalWrite(dirPin, dir ? HIGH : LOW);

  for (int i = 0; i < steps; i++) {
      digitalWrite(stepPin, HIGH);
      delayMicroseconds(500);
      digitalWrite(stepPin, LOW);
      delayMicroseconds(500);
  }
}


bool calibrate_stepper_motor() {
    while (digitalRead(checkButtonPin) == HIGH) {
        move_motor(0, 1);
    }
    return true;
}

#define BASE_URL "http://192.168.6.115:8000"

float post_ideal_temp(int room_id, float ideal_temp) {
  Serial.print("Posting ideal temperature: ");
  Serial.println(ideal_temp);

  String ideal_temp_formatted = String(ideal_temp, 2);
  ideal_temp_formatted.replace('.', '-');         

  String url = String(BASE_URL) + "/api/heat_report/" + String(room_id) + "/" + ideal_temp_formatted;

  String resp = httpGETRequest(url.c_str());
  
  // code commented by Hamroz, because a single GET response is sufficient
  // // JSON payload with both room_id and ideal_temp
  // String jsonData = "{\"room_id\":" + String(room_id) + // scuffed way to deal with cpp annoying ass string concatonataiton
  //                   ",\"ideal_temp\":" + String(ideal_temp, 2) + "}";

  // String response = httpPOSTRequest(url.c_str(), jsonData);

  // float server_reply = response.toFloat();  // parse reply (if numeric)

  // Serial.print("Server replied: ");
  // Serial.println(server_reply);

  return server_reply;
}

#include <ArduinoJson.h>

bool fetch_room_state(int room_id, float& out_current_temp, float& out_ideal_temp, int& out_wake_time, int& out_sleep_time) {
  Serial.println("Getting room state (single JSON)");
  String url = String(BASE_URL) + "/api/room-update/" + String(room_id); // <— change if needed
  String resp = httpGETRequest(url.c_str());

  if (resp.length() == 0) {
  Serial.println("Empty HTTP response");
  return false;
  }

  // Parse JSON
  StaticJsonDocument<256> doc; // increase if your JSON grows
  DeserializationError err = deserializeJson(doc, resp);
  if (err) {
  Serial.print("JSON parse error: ");
  Serial.println(err.c_str());
  Serial.print("Payload: ");
  Serial.println(resp);
  return false;
  }

  // Extract with safe defaults
  out_current_temp = doc["current_temperature"] | NAN;
  out_ideal_temp   = doc["ideal_temperature"]   | NAN;
  out_wake_time    = doc["wake_time"]         | -1;
  out_sleep_time   = doc["sleep_time"]        | -1;

  Serial.print("current_temperature="); Serial.println(out_current_temp);
  Serial.print("ideal_temperature=");   Serial.println(out_ideal_temp);
  Serial.print("wake_time=");         Serial.println(out_wake_time);
  Serial.print("sleep_time=");        Serial.println(out_sleep_time);

  // Basic sanity check (optional)
  return !isnan(out_current_temp) && !isnan(out_ideal_temp) &&
  out_wake_time >= 0 && out_sleep_time >= 0;
}

float get_current_temp(int room_id) {
  Serial.print("Getting temperature");
  String url = String(BASE_URL) + "/get_current_temp/" + String(room_id);
  float ideal_temps = httpGETRequest(url.c_str()).toFloat();
  Serial.println(ideal_temps);
  return ideal_temps;
}
float get_ideal_temp(int room_id) {
  Serial.print("Getting temperature");
  String url = String(BASE_URL) + "/get_ideal_temp/" + String(room_id);
  float ideal_temps = httpGETRequest(url.c_str()).toFloat();
  Serial.println(ideal_temps);
  return ideal_temps;
}
int get_wake(int room_id) {
  Serial.println("Getting wake time");
  String url = String(BASE_URL) + "/get_wake/" + String(room_id);
  int wake_time = httpGETRequest(url.c_str()).toInt();
  Serial.println(wake_time);
  return wake_time;
}
int get_sleep(int room_id) {
  Serial.println("Getting sleep time");
  String url = String(BASE_URL) + "/get_sleep/" + String(room_id);
  int sleep_time = httpGETRequest(url.c_str()).toInt();
  Serial.println(sleep_time);
  return sleep_time;
}



#include <WiFi.h>

void setup_wifi(const char* ssid, const char* password) {
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

# include <Wire.h>
# include "SSD1306.h"//ディスプレイ用ライブラリを読み込み

SSD1306  display(0x3c, 21, 22); //SSD1306インスタンスの作成（I2Cアドレス,SDA,SCL）

void setup_screen() {
  display.init();    //ディスプレイを初期化
  display.setFont(ArialMT_Plain_16);    //フォントを設定
  display.drawString(0, 0, "Hello SSD1306!!");    //(0,0)の位置にHello Worldを表示
  display.display();   //指定された情報を描画
}
void update_screen(String message) {
  display.clear();
  display.drawString(0,0, message);
  display.display();
}
#define checkButtonPin 25 // the number of the pushbutton pin
#define tempUpButton 19
#define tempDownButton 18
void setup_buttons() {
  pinMode(checkButtonPin, INPUT_PULLUP);
  pinMode(tempUpButton , INPUT_PULLUP);
  pinMode(tempDownButton, INPUT_PULLUP);
}

int current_time;
int previous_time = 0;

void setup() {
  Serial.begin(115200);
  setup_wifi("ISAK-S", "heK7bTGW"); 
  get_ntp_time(gmtOffset_sec,daylightOffset_sec,ntpServer);

  setup_screen();
  setup_button();

  stepper_setup();
  calibrate_stepper_motor();
  // get current tuning
}

int set_point = 0;
float current_temp;
float ideal_temp;
float ideal_temp_new;
int wake_time;
int sleep_time;

#define ID 69

bool motor_reset_done = false;  // flag
void reset_motor() {
  while (digitalRead(checkButtonPin) == HIGH) {
    move_motor(0, 1);
  }
  digitalWrite(enaPin, HIGH);
}

class SimplePID {
public:
    enum class Mode { AUTO, MANUAL };

    // Time unit: minutes (to mirror your Python code)
    // ki, kd are given in per-minute terms (like your Python display values).
    explicit SimplePID(double kp,
                       double ki,
                       double kd,
                       std::string name,
                       double sample_time_minutes,
                       double output_min,
                       double output_max)
        : name_(std::move(name)),
          dispKp_(kp), dispKi_(ki), dispKd_(kd),
          kp_(kp),
          ki_(ki * sample_time_minutes),        // per-sample conversion
          kd_(kd / sample_time_minutes),        // per-sample conversion
          sampleTime_(sample_time_minutes),
          outputMin_(output_min),
          outputMax_(output_max),
          inAuto_(true) // default AUTO like Python reset()
    {}

    // Compute PID output. Returns the controller output.
    // setpoint, measurement, current_time are in consistent units; current_time in minutes.
    double compute(double setpoint, double measurement, double current_time_min) {
        input_ = measurement;
        setpoint_ = setpoint;

        if (!inAuto_) {
            return output_;
        }

        const double timeChange = current_time_min - lastTime_;
        if (lastTime_ > 0.0 && timeChange < sampleTime_) {
            return output_; // not time yet
        }

        // Error
        const double error = setpoint - measurement;

        // Track errors (bounded to 1000)
        errors_.push_back(error);
        if (errors_.size() > 1000) {
            errors_.erase(errors_.begin());
        }

        // --- Leaky integrator (time-based) ---
        // dt is the actual elapsed time (min); stepScale adjusts the pre-scaled ki_ for missed cycles.
        const double dt = (lastTime_ > 0.0) ? timeChange : sampleTime_;
        const double decay = std::exp(-leakRate_ * dt);            // leakRate_ is per-minute
        const double stepScale = dt / sampleTime_;                 // 1.0 when dt==sampleTime_
        iTerm_ = decay * iTerm_ + (ki_ * stepScale) * error;
        // -------------------------------------

        // Derivative on measurement to avoid derivative kick (per-sample form consistent with kd_)
        const double dInput = measurement - lastInput_;

        // Preliminary output
        double out = kp_ * error + iTerm_ - kd_ * dInput;

        // Clamp + anti-windup (back-calculate integral)
        if (out > outputMax_) {
            iTerm_ -= (out - outputMax_);
            out = outputMax_;
        } else if (out < outputMin_) {
            iTerm_ += (outputMin_ - out);
            out = outputMin_;
        }

        // Keep integrator within limits as an extra guard
        iTerm_ = std::clamp(iTerm_, outputMin_, outputMax_);

        // Store
        output_ = out;
        lastInput_ = measurement;
        lastTime_ = current_time_min;

        return output_;
    }

    // Update tunings (display values); applies sample-time conversion internally
    void setTunings(double kp, double ki, double kd) {
        if (kp < 0 || ki < 0 || kd < 0) return;
        dispKp_ = kp; dispKi_ = ki; dispKd_ = kd;
        kp_ = kp;
        ki_ = ki * sampleTime_;
        kd_ = kd / sampleTime_;
    }

    // Change sample time (minutes) and adjust ki_, kd_ accordingly
    void setSampleTime(double new_sample_time_minutes) {
        if (new_sample_time_minutes <= 0) return;
        const double ratio = new_sample_time_minutes / sampleTime_;
        ki_ *= ratio;
        kd_ /= ratio;
        sampleTime_ = new_sample_time_minutes;
    }

    // Set output limits and clamp current output and integral term
    void setOutputLimits(double min_val, double max_val) {
        if (min_val >= max_val) return;
        outputMin_ = min_val;
        outputMax_ = max_val;

        output_ = std::clamp(output_, outputMin_, outputMax_);
        iTerm_  = std::clamp(iTerm_,  outputMin_, outputMax_);
    }

    // Set controller mode (AUTO / MANUAL), with bumpless init when entering AUTO
    void setMode(Mode mode) {
        const bool newAuto = (mode == Mode::AUTO);
        if (newAuto && !inAuto_) {
            initialize();
        }
        inAuto_ = newAuto;
    }

    // Convenience overload that mirrors Python set_mode("AUTO"/"MANUAL")
    void setMode(const std::string& modeStr) {
        Mode m = Mode::MANUAL;
        if (equalsIgnoreCase(modeStr, "AUTO")) m = Mode::AUTO;
        setMode(m);
    }

    // Initialize for bumpless transfer MANUAL -> AUTO
    void initialize() {
        iTerm_ = output_;
        lastInput_ = input_;
        iTerm_ = std::clamp(iTerm_, outputMin_, outputMax_);
    }

    // Reset like Python: clears state and sets AUTO
    void reset() {
        iTerm_ = 0.0;
        lastInput_ = 0.0;
        lastTime_ = 0.0;
        output_ = 0.0;
        errors_.clear();
        inAuto_ = true;
    }

    // Leak tuning (per minute)
    void setLeakRate(double per_minute) { if (per_minute >= 0.0) leakRate_ = per_minute; }
    double getLeakRate() const { return leakRate_; }

    // Getters
    std::string getName()         const { return name_; }
    Mode        getMode()         const { return inAuto_ ? Mode::AUTO : Mode::MANUAL; }
    const char* getModeString()   const { return inAuto_ ? "AUTO" : "MANUAL"; }
    double      getKp()           const { return dispKp_; }
    double      getKi()           const { return dispKi_; }
    double      getKd()           const { return dispKd_; }
    double      getSetpoint()     const { return setpoint_; }
    double      getInput()        const { return input_; }
    double      getOutput()       const { return output_; }
    double      getSampleTime()   const { return sampleTime_; }
    double      getOutputMin()    const { return outputMin_; }
    double      getOutputMax()    const { return outputMax_; }
    const std::vector<double>& getErrors() const { return errors_; }

private:
    static bool equalsIgnoreCase(const std::string& a, const std::string& b) {
        if (a.size() != b.size()) return false;
        for (size_t i = 0; i < a.size(); ++i) {
            if (std::tolower(static_cast<unsigned char>(a[i])) !=
                std::tolower(static_cast<unsigned char>(b[i]))) {
                return false;
            }
        }
        return true;
    }

    // Identity
    std::string name_;

    // Display/user-entered gains
    double dispKp_{0.0}, dispKi_{0.0}, dispKd_{0.0};

    // Working gains (converted for per-sample)
    double kp_{0.0}, ki_{0.0}, kd_{0.0};

    // Sample time (minutes)
    double sampleTime_{1.0};

    // Limits
    double outputMin_{0.0}, outputMax_{100.0};

    // State
    double input_{0.0};
    double output_{0.0};
    double setpoint_{0.0};
    double iTerm_{0.0}; // anti windup
    double lastInput_{0.0};
    double lastTime_{0.0};
    bool   inAuto_{true};

    // Leak (per minute). 0.05 ≈ 20 min memory (roughly).
    double leakRate_{0.05};

    // Debug/monitoring
    std::vector<double> errors_;
};

void loop() {
  current_time = get_ntp_time();
  if current_time - previous_time >= 300: //300 is 5 minutes, change accordingly
    previous_time = current_time;
    
    // change by Hamroz
    if (!fetch_room_state(ID, current_temp, ideal_temp_new, wake_time, sleep_time)) {
      // Fallback to individual endpoints if the JSON call fails
      current_temp   = get_current_temp(ID);
      ideal_temp_new = get_ideal_temp(ID);
      wake_time      = get_wake(ID);
      sleep_time     = get_sleep(ID);
    }

    if (current_time % 86400 > sleep_time || current_time % 86400 < wake_time) {
      if (motor_reset_done != true) {
        reset_motor();
        motor_reset_done = true;
      }
    }

    if (ideal_temp_new != ideal_temp) {
      set_point = new_set_point;
      pid.idealtemp=setpoint
    }

    pid.update(current temp)
    pid.output() // moves motor

    if digitalRead(up_button) == LOW:
      ideal_temp = ideal_temp - 0.25;
      update_screeen(String(setpoint) + "°C");
      post_ideal_temp(ID, setpoint);
      //send post request to server
    if digitalRead(down_button) == LOW:
      ideal_temp = ideal_temp + 0.25;
      update_screeen(String(setpoint) + "°C");
      post_ideal_temp(ID, setpoint);
}
