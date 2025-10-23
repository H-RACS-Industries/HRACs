#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>
#include <arduino_secrets.h>   
#include <mbedtls/sha256.h>
#include <Preferences.h>

// ----------- CONFIG -------------
#define WIFI_SSID  "ISAK-S"
#define WIFI_PASS  "heK7bTGW"
#define BASE_URL   "http://192.168.4.15:8000/"   
#define ID         11

const long SECONDS_PER_DAY = 86400;
int gmtOffset_sec = 9 * 3600;  // JST
int daylightOffset_sec = 0;
String ntpServer = "pool.ntp.org";

int previous_time = 0;
int current_time = 0;

// ---------- Helpers: SHA256 + hex + nonce + time ----------
String to_hex(const uint8_t* buf, size_t len) {
  static const char* hex = "0123456789abcdef";
  String out;
  out.reserve(len * 2);
  for (size_t i = 0; i < len; i++) {
    out += hex[(buf[i] >> 4) & 0xF];
    out += hex[ buf[i]       & 0xF];
  }
  return out; // lowercase
}

String sha256_hex(const String& data) {
  uint8_t out[32];
  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts(&ctx, 0);
  mbedtls_sha256_update(&ctx, (const unsigned char*)data.c_str(), data.length());
  mbedtls_sha256_finish(&ctx, out);
  mbedtls_sha256_free(&ctx);
  return to_hex(out, 32);
}

// base = SECRET || METHOD|PATH|TS|NONCE|BODY
String make_signature(const String& method, const String& path, const String& ts,
                      const String& nonce, const String& body) {
  String base;
  base.reserve(strlen(SECRET_KEY) + method.length() + path.length() + ts.length() + nonce.length() + body.length() + 10);
  base += SECRET_KEY;
  base += method; base += "|"; base += path;  base += "|";
  base += ts;     base += "|"; base += nonce; base += "|";
  base += body;

  return sha256_hex(base); // lowercase hex
}

String rand_nonce() {
  uint32_t r = esp_random();
  char buf[9]; snprintf(buf, sizeof(buf), "%08x", r);
  return String(buf);
}

bool ensure_time_ready(unsigned long timeout_ms = 10000) {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer.c_str());
  unsigned long start = millis();
  time_t now;
  do {
    delay(200);
    time(&now);
    if (now > 1700000000) return true; // ~2023+
  } while (millis() - start < timeout_ms);
  Serial.println("WARN: NTP time not ready; signatures may be rejected");
  return false;
}

// ---------- WiFi ----------
void setup_wifi(const char* ssid, const char* password) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("\nConnecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) { Serial.print("."); delay(500); }
  Serial.println("\nConnected!");
  Serial.print("IP: "); Serial.println(WiFi.localIP());
}

// ---------- NTP wrapper ----------
int get_ntp_time(String ntpServer, int gmtOffset_sec, int daylightOffset_sec) {
  // assume ensure_time_ready() was already called in setup
  time_t now; time(&now);
  return now;
}

// ---------- Signature headers (call this before GET/POST) ----------
void add_auth_headers(HTTPClient& http, const String& method, const String& fullUrl,
                      const String& pathForSig, const String& body /*may be ""*/) {
  // TS & nonce
  time_t now_t; time(&now_t);
  String ts = String((unsigned long)now_t);
  String nonce = rand_nonce();

  // Build signature on PATH (not the full URL)
  String sig = make_signature(method, pathForSig, ts, nonce, body);

  // Add headers
  http.addHeader("X-Device-Id", String(ID));   // send as string
  http.addHeader("X-Timestamp", ts);
  http.addHeader("X-Nonce", nonce);
  http.addHeader("X-Signature", sig);
}

// ---------- API wrappers ----------
void post_ideal_temp(int room_id, float ideal_temp) {
  Serial.print("Posting ideal temperature: ");
  Serial.println(ideal_temp);

  String ideal_temp_formatted = String(ideal_temp, 2);
  ideal_temp_formatted.replace('.', '-');

  String path = "api/post_ideal_temp/" + String(room_id) + "/" + ideal_temp_formatted + "/";  
  String url  = String(BASE_URL) + path;

  WiFiClient client;
  HTTPClient http;
  http.begin(client, url);

  // GET with signature (body empty)
  add_auth_headers(http, "GET", url, "/" + path, "");   // sign "/api/..."? If your verifier uses request.path, include leading "/" and the exact path portion it sees (e.g. "/api/post_ideal_temp/.../").
  int code = http.GET();

  if (code > 0) {
    Serial.printf("Response code: %d\n", code);
    Serial.println(http.getString());
  } else {
    Serial.printf("HTTP error: %d\n", code);
  }

  http.end();
}

#include <ArduinoJson.h>
bool fetch_room_state(int room_id, float& out_current_temp, float& out_ideal_temp, int& out_wake_time, int& out_sleep_time) {
  Serial.println("Getting room state");

  
  String path = "api/room-update/" + String(room_id) + "/";   // add trailing slash
  String url  = String(BASE_URL) + path;

  WiFiClient client;
  HTTPClient http;
  http.begin(client, url);

  // GET with signature (empty body)
  add_auth_headers(http, "GET", url, "/" + path, "");

  int code = http.GET();
  if (code <= 0) {
    Serial.println("Could not establish connection to server.");
    http.end();
    return false;
  }

  String resp = http.getString();
  http.end();

  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, resp);
  if (err) {
    Serial.print("JSON parse error: "); Serial.println(err.c_str());
    Serial.print("Payload: "); Serial.println(resp);
    return false;
  }

  out_current_temp = doc["current_temperature"] | NAN;
  out_ideal_temp   = doc["ideal_temperature"]   | NAN;
  out_wake_time    = doc["wake_time"]           | -1;
  out_sleep_time   = doc["sleep_time"]          | -1;

  Serial.print("current_temperature="); Serial.println(out_current_temp);
  Serial.print("ideal_temperature=");   Serial.println(out_ideal_temp);
  Serial.print("wake_time=");           Serial.println(out_wake_time);
  Serial.print("sleep_time=");          Serial.println(out_sleep_time);

  return !isnan(out_current_temp) && !isnan(out_ideal_temp) &&
         out_wake_time >= 0 && out_sleep_time >= 0;
}

void ensure_device_exists() {
  Serial.println("Attempting to create the device if it is new...");

  String path = "api/new_device/" + String(ID) + "/";
  String url  = String(BASE_URL) + path;

  WiFiClient client;
  HTTPClient http;
  http.begin(client, url);

  // GET with signature (body empty)
  add_auth_headers(http, "GET", url, "/" + path, "");   // sign "/api/..."? If your verifier uses request.path, include leading "/" and the exact path portion it sees (e.g. "/api/post_ideal_temp/.../").
  int code = http.GET();

  if (code > 0) {
    Serial.println(http.getString());
  } else {
    Serial.printf("HTTP error: %d\n", code);
  }

  http.end();
}

// ---------- Setup / Loop ----------

void setup() {
  Serial.begin(115200);
  delay(1000);

  setup_wifi(WIFI_SSID, WIFI_PASS);
  ensure_time_ready();

  time_t now; time(&now);
  Serial.print("Epoch time: "); Serial.println((unsigned long)now);

  ensure_device_exists();

  Serial.println("\n--- Testing API Requests ---");
  post_ideal_temp(ID, 2048.2);
  Serial.println("--- Test Done, God knows what happened ---"); // haha

  // stepper
  stepper_setup();
  button_setup();

  calibrate_stepper_motor();
}

float current_temp = -1, ideal_temp_new = -1;
int   wake_time_v  = -1, sleep_time_v  = -1;


// - stepper motor stuff-
// stepper motor
#define dirPin 32
#define stepPin 33
#define checkButtonPin 25

void stepper_setup() {
    pinMode(stepPin, OUTPUT);
    pinMode(dirPin, OUTPUT);
}

void button_setup() {
    pinMode(checkButtonPin, INPUT_PULLUP);
}

void move_motor(bool dir, int steps) {
    digitalWrite(dirPin, dir);
    // These four lines result in 1 step:wwww
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

void loop() {
  current_time = get_ntp_time(ntpServer, gmtOffset_sec, daylightOffset_sec);
  if (current_time - previous_time >= 20) {
    previous_time = current_time;
    if (!fetch_room_state(ID, current_temp, ideal_temp_new, wake_time_v, sleep_time_v)) {
      Serial.println("Couldn’t fetch room state");
    }
  }
}
