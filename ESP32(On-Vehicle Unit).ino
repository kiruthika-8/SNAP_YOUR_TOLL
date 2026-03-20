#include <WiFi.h>
#include <TinyGPSPlus.h>
#include <Firebase_ESP_Client.h>
#include <math.h>

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define WIFI_SSID "wifi"
#define WIFI_PASSWORD "password"

#define API_KEY "Api key"
#define DATABASE_URL "https://boothlesstoll-default-rtdb.asia-southeast1.firebasedatabase.app/"


FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;


TinyGPSPlus gps;
#define RXD2 16
#define TXD2 17
HardwareSerial neogps(1);

unsigned long sendDataPrevMillis = 0;


float prevLat = 0, prevLng = 0;
float totalDist = 0;


float fareRate = 1.0;   // ₹1 per meter
float wallet = 100;     // Initial balance


double haversine(float lat1, float lon1, float lat2, float lon2) {
  double R = 6371000.0;
  double dLat = radians(lat2 - lat1);
  double dLon = radians(lon2 - lon1);

  lat1 = radians(lat1);
  lat2 = radians(lat2);

  double a = sin(dLat / 2) * sin(dLat / 2) +
             sin(dLon / 2) * sin(dLon / 2) * cos(lat1) * cos(lat2);
  double c = 2 * atan2(sqrt(a), sqrt(1 - a));
  return R * c;
}

void setup() {
  Serial.begin(115200);
  neogps.begin(9600, SERIAL_8N1, RXD2, TXD2);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  Firebase.signUp(&config, &auth, "", "");
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  while (neogps.available()) {
    gps.encode(neogps.read());
  }

  if (Firebase.ready() && millis() - sendDataPrevMillis > 5000) {
    sendDataPrevMillis = millis();

    if (!gps.location.isValid()) {
      Serial.println("Waiting for valid GPS fix...");
      return;
    }

    float lat = gps.location.lat();
    float lng = gps.location.lng();

    float deltaDist = 0;

    if (prevLat != 0 && prevLng != 0) {
      deltaDist = haversine(prevLat, prevLng, lat, lng);
      totalDist += deltaDist;
    }

    prevLat = lat;
    prevLng = lng;

    float deltaFare = deltaDist * fareRate;

    if (wallet >= deltaFare) {
      wallet -= deltaFare;
    } else {
      Serial.println("Insufficient wallet balance!");
      return;
    }

    // Firebase update
    Firebase.RTDB.setFloat(&fbdo, "vehicle/latitude", lat);
    Firebase.RTDB.setFloat(&fbdo, "vehicle/longitude", lng);
    Firebase.RTDB.setFloat(&fbdo, "vehicle/total_distance_m", totalDist);
    Firebase.RTDB.setFloat(&fbdo, "vehicle/wallet_balance", wallet);

    Serial.printf("Lat: %.6f Lng: %.6f\n", lat, lng);
    Serial.printf("Distance +%.2f m | Total: %.2f m\n", deltaDist, totalDist);
    Serial.printf("Fare: ₹%.2f | Wallet: ₹%.2f\n\n", deltaFare, wallet);
  }
}
