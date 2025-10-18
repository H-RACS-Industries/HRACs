#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>

// ----------- CONFIG -------------
#define WIFI_SSID     "ISAK-S"
#define WIFI_PASS     "heK7bTGW"
#define BASE_URL      "http://192.168.4.15:8000/api/"
#define ID       1
// --------------------------------

int gmtOffset_sec = 32400;  // +9h JST
int daylightOffset_sec = 0;
String ntpServer = "pool.ntp.org";

int previous_time = 0;
int current_time = 0;

// ---- Connect WiFi ----a
void setup_wifi(const char* ssid, const char* password) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("\nConnecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nConnected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// ---- NTP ----
int get_ntp_time(String ntpServer, int gmtOffset_sec, int daylightOffset_sec) {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer.c_str());
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
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


// ---- HTTP POST ----
String httpPOSTRequest(const char* serverName, String postData) {
  WiFiClient client;
  HTTPClient http;
  http.begin(client, serverName);
  http.addHeader("Content-Type", "application/json");
  int code = http.POST(postData);
  String payload = "{}";
  if (code > 0) {
    Serial.printf("POST [%s] => %d\n", serverName, code);
    payload = http.getString();
  } else {
    Serial.printf("POST failed, code=%d\n", code);
  }
  http.end();
  return payload;
}

// ---- API Wrappers ----
float get_current_temp(int room_id) {
  String url = String(BASE_URL) + "/get_current_temp/" + String(room_id);
  return httpGETRequest(url.c_str()).toFloat();
}

float get_ideal_temp(int room_id) {
  String url = String(BASE_URL) + "/get_ideal_temp/" + String(room_id);
  return httpGETRequest(url.c_str()).toFloat();
}

// float post_ideal_temp(int room_id, float temp) {
//   String url = String(BASE_URL) + "/post_ideal_temp";
//   String json = "{\"room_id\":" + String(room_id) + ",\"ideal_temp\":" + String(temp, 2) + "}";
//   String reply = httpPOSTRequest(url.c_str(), json);
//   return reply.toFloat();
// }

void post_ideal_temp(int room_id, float ideal_temp) {
  Serial.print("Posting ideal temperature: ");
  Serial.println(ideal_temp);

  String ideal_temp_formatted = String(ideal_temp, 2);
  ideal_temp_formatted.replace('.', '-');         

  String url = String(BASE_URL) + "post_ideal_temp/" + String(room_id) + "/" + ideal_temp_formatted;

  httpGETRequest(url.c_str());
}

#include <ArduinoJson.h>
bool fetch_room_state(int room_id, float& out_current_temp, float& out_ideal_temp, int& out_wake_time, int& out_sleep_time) {
  Serial.println("Getting room state (single JSON)");
  String url = String(BASE_URL) + "room-update/" + String(room_id); // <— change if needed

  Serial.println("url: " + url);

  WiFiClient client;    
  HTTPClient http;      
  http.begin(client, url);
  int code = http.GET();

  String resp = http.getString();

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

// ---- Setup ----
void setup() {
  Serial.begin(115200);
  delay(1000);

  setup_wifi(WIFI_SSID, WIFI_PASS);
  int now = get_ntp_time(ntpServer, gmtOffset_sec, daylightOffset_sec);
  Serial.print("Epoch time: ");
  Serial.println(now);

  Serial.println("\n--- Testing API Requests ---");

  post_ideal_temp(ID, 1000.75);

  Serial.println("--- Done ---");
}

float current_temp = -1, ideal_temp_new=-1;
int wake_time=-1, sleep_time=-1;

void loop() {
  current_time = get_ntp_time(ntpServer,gmtOffset_sec,daylightOffset_sec);
  if (current_time - previous_time >= 10) // in seconds
  {
    Serial.println("entered to if statement");
    previous_time = current_time;
    
    if (!fetch_room_state(ID, current_temp, ideal_temp_new, wake_time, sleep_time)) {
      Serial.println("Couldnt fetch room state XD");
    }
  }
}