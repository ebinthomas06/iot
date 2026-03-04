#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>

// ============================================================
//  CHANGE THESE TWO LINES ONLY
// ============================================================
const char* WIFI_SSID     = "Xiaomi 14 Civi";
const char* WIFI_PASSWORD = "12345678";
// ============================================================

const char* MQTT_BROKER = "broker.hivemq.com";
const int   MQTT_PORT   = 1883;
const char* MQTT_TOPIC  = "robothand/fingers";
const char* CLIENT_ID   = "ESP32_RobotHand";

// ── Servos ──────────────────────────────────────────────────
Servo servo[5];
int   pins[5] = {13, 12, 14, 27, 26};

// ── WiFi + MQTT clients ─────────────────────────────────────
WiFiClient   espClient;
PubSubClient mqtt(espClient);

// ── Called automatically when a message arrives ─────────────
void onMessage(char* topic, byte* payload, unsigned int length) {

  // 1. Build a String from the raw bytes
  String data = "";
  for (unsigned int i = 0; i < length; i++) {
    data += (char)payload[i];
  }
  data.trim();

  Serial.print("Received: ");
  Serial.println(data);

  // 2. Parse "143,90,45,120,60"  (same logic as your original Serial code)
  int  angles[5];
  int  idx = 0;

  char buf[data.length() + 1];
  data.toCharArray(buf, sizeof(buf));

  char* token = strtok(buf, ",");
  while (token != NULL && idx < 5) {
    angles[idx++] = constrain(atoi(token), 0, 180);
    token = strtok(NULL, ",");
  }

  // 3. Move servos only if all 5 values arrived
  if (idx == 5) {
    for (int i = 0; i < 5; i++) {
      servo[i].write(angles[i]);
      Serial.print("  Servo "); Serial.print(i);
      Serial.print(" -> ");    Serial.println(angles[i]);
    }
  } else {
    Serial.println("  Bad data — expected 5 comma-separated values");
  }
}

// ── Connect to WiFi ─────────────────────────────────────────
void connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("WiFi connected!  IP: ");
  Serial.println(WiFi.localIP());
}

// ── Connect to MQTT broker ──────────────────────────────────
void connectMQTT() {
  while (!mqtt.connected()) {
    Serial.print("Connecting to MQTT broker...");

    if (mqtt.connect(CLIENT_ID)) {
      Serial.println(" connected!");
      mqtt.subscribe(MQTT_TOPIC);
      Serial.print("Subscribed to topic: ");
      Serial.println(MQTT_TOPIC);
    } else {
      Serial.print(" failed (rc=");
      Serial.print(mqtt.state());
      Serial.println(")  retrying in 3 s...");
      delay(3000);
    }
  }
}

// ── Setup ───────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== RobotHand ESP32 ===");

  // Attach all servos and move to rest position (0°)
  for (int i = 0; i < 5; i++) {
    servo[i].setPeriodHertz(50);          // MG90 standard 50 Hz
    servo[i].attach(pins[i], 500, 2500);  // same pulse range as your original
    servo[i].write(0);
  }
  Serial.println("Servos attached (all at 0°)");

  // WiFi
  connectWiFi();

  // MQTT
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(onMessage);
  connectMQTT();

  Serial.println("Ready — waiting for gestures from website...");
}

// ── Loop ────────────────────────────────────────────────────
void loop() {
  // Reconnect automatically if broker drops
  if (!mqtt.connected()) {
    Serial.println("MQTT disconnected — reconnecting...");
    connectMQTT();
  }

  mqtt.loop();   // keeps the connection alive & fires onMessage()
}
