#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h> // <--- NEW LIBRARY FOR HTTP REQUESTS

// ----------------------------------------------------
// 1. Wi-Fi Credentials - IMPORTANT: Use your hotspot details here
// ----------------------------------------------------
#define WIFI_SSID "Pikachuu"                  // Your hotspot SSID
#define WIFI_PASSWORD "Ragasu1009" // Your hotspot password

// ----------------------------------------------------
// 2. Firebase Credentials (for REST API)
// ----------------------------------------------------
// Your Firebase Realtime Database URL
#define FIREBASE_RTDB_URL "https://esp-32-setup-default-rtdb.firebaseio.com"
// Your Firebase Web API Key (to append to URL for authentication)
#define FIREBASE_API_KEY "AIzaSyCSqfW_z1oXwH2_n3iqgspy34oLgp3P8pI"

// ----------------------------------------------------
// 3. Sensor/Data Simulation (for demonstration)
// ----------------------------------------------------
int sensorValue = 0; // Simulate a sensor reading

// ----------------------------------------------------
// 4. Timing for sending data (non-blocking)
// ----------------------------------------------------
unsigned long lastSendTime = 0;
const long sendInterval = 5000; // Send data every 5 seconds (5000 milliseconds)

void setup()
{
  Serial.begin(115200);
  Serial.println("\n--- ESP32 Firebase REST API Logger ---");

  // Connect to Wi-Fi
  Serial.print("Connecting to Wi-Fi: ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // NO NTP SYNCHRONIZATION NEEDED FOR THIS METHOD (IF WE CAN'T GET IT)
  Serial.println("Attempting to connect to Firebase via REST API without NTP sync...");
}

void loop()
{
  if (WiFi.isConnected())
  {
    unsigned long currentTime = millis();
    if (currentTime - lastSendTime >= sendInterval)
    {
      lastSendTime = currentTime;

      sensorValue++; // Increment sensor value for demonstration

      // Construct the full Firebase URL for pushing data to a "readings" path
      // The "?auth=" part is for authentication using your API key.
      String firebasePath = FIREBASE_RTDB_URL;
      firebasePath += "/readings.json?auth="; // .json is required for REST API
      firebasePath += FIREBASE_API_KEY;

      // Create the JSON payload
      String jsonPayload = "{";
      jsonPayload += "\"value\": " + String(sensorValue);
      jsonPayload += ",\"uptime_ms\": " + String(millis());
      jsonPayload += ",\"random_data\": " + String(random(0, 100));
      // You can add a server-side timestamp like this:
      // jsonPayload += ",\"timestamp\": {\".sv\": \"timestamp\"}";
      jsonPayload += "}";

      Serial.print("Sending data to Firebase (REST)... ");
      Serial.print("Payload: ");
      Serial.println(jsonPayload);

      HTTPClient http;
      http.begin(firebasePath);                           // Specify request destination
      http.addHeader("Content-Type", "application/json"); // Specify content-type header

      // Send the POST request
      int httpResponseCode = http.POST(jsonPayload);

      if (httpResponseCode > 0)
      {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String response = http.getString(); // Get the response payload
        Serial.println(response);
      }
      else
      {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
        Serial.print("Error: ");
        Serial.println(http.errorToString(httpResponseCode));
      }

      http.end(); // Free resources
    }
  }
  else
  {
    Serial.println("WiFi Disconnected. Reconnecting...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    // You might want to add a reconnect loop here, or just let loop() keep trying
  }
}