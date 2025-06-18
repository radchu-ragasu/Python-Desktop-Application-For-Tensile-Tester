#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

// ======================
// CONFIGURE THESE
// ======================
#define WIFI_SSID "Pikachuu"
#define WIFI_PASSWORD "Ragasu1009"

#define FIREBASE_HOST "https://esp-32-setup-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "AIzaSyCSqfW_z1oXwH2_n3iqgspy34oLgp3P8pI"

unsigned long lastSendTime = 0;
const unsigned long interval = 10000; // 10 seconds

int experimentId = -1;     // Will be fetched from Firebase
String experimentKey = ""; // E0001, E0002, etc.

void setupWiFi()
{
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nConnected!");
}

// Pad number with zeros (e.g., 1 → E0001)
String generateExperimentKey(int id)
{
  char buffer[10];
  sprintf(buffer, "E%04d", id);
  return String(buffer);
}

// Get the nextExperimentId from Firebase
int getNextExperimentId()
{
  HTTPClient http;
  String url = FIREBASE_HOST;
  url += "/nextExperimentId.json?auth=" + String(FIREBASE_AUTH);

  http.begin(url);
  int httpCode = http.GET();
  if (httpCode == 200)
  {
    String payload = http.getString();
    http.end();
    return payload.toInt(); // Convert string to int
  }
  else
  {
    Serial.println("Failed to fetch nextExperimentId");
    http.end();
    return -1;
  }
}

// Create experiment structure and upload to Firebase
void createNewExperiment(String key, String name)
{
  HTTPClient http;
  String url = FIREBASE_HOST;
  url += "/experiments/" + key + ".json?auth=" + String(FIREBASE_AUTH);

  unsigned long now = millis();
  String payload = "{";
  payload += "\"name\": \"" + name + "\",";
  payload += "\"createdAt\": " + String(now) + ",";
  payload += "\"data\": {}";
  payload += "}";

  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  int code = http.PUT(payload);
  Serial.println("Creating experiment: " + key);
  Serial.println("HTTP Code: " + String(code));
  http.end();
}

// Increment and update nextExperimentId
void updateNextExperimentId(int nextId)
{
  HTTPClient http;
  String url = FIREBASE_HOST;
  url += "/nextExperimentId.json?auth=" + String(FIREBASE_AUTH);

  String payload = String(nextId);

  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.PUT(payload);
  http.end();
}

// Simulate stress/strain data and upload
void uploadFakeData(int timeSec)
{
  float strain = random(10, 100) / 1000.0; // 0.010 to 0.099
  float stress = random(50, 500) / 10.0;   // 5.0 to 50.0

  String url = FIREBASE_HOST;
  url += "/experiments/" + experimentKey + "/data/" + String(timeSec) + ".json?auth=" + String(FIREBASE_AUTH);

  String payload = "{";
  payload += "\"x\": " + String(strain, 3) + ",";
  payload += "\"y\": " + String(stress, 2);
  payload += "}";

  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  int code = http.PUT(payload);
  Serial.println("Uploaded data at " + String(timeSec) + "s");
  Serial.println("Payload: " + payload);
  Serial.println("HTTP Code: " + String(code));
  http.end();
}

void setup()
{
  Serial.begin(115200);
  setupWiFi();

  experimentId = getNextExperimentId();
  if (experimentId == -1)
  {
    Serial.println("Can't proceed without a valid experiment ID.");
    return;
  }

  experimentKey = generateExperimentKey(experimentId);
  createNewExperiment(experimentKey, "Test Run Without Sensors");
  updateNextExperimentId(experimentId + 1);
  lastSendTime = millis();
}

int timeElapsed = 0;

void loop()
{
  if (WiFi.status() != WL_CONNECTED)
    return;

  unsigned long currentTime = millis();
  if (currentTime - lastSendTime >= interval)
  {
    lastSendTime = currentTime;
    uploadFakeData(timeElapsed);
    timeElapsed += 10;
  }
}
