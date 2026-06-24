#include "IIOTDEVKIT4G.h"

IIOTDEVKIT4G IIOT_Dev_kit;
Broker       TB_Broker0;

static const char* APN_CID     = "1";
static const char* APN_TYPE    = "IP";
static const char* APN_NAME    = "your-apn";

// MQTT over TLS to the public Mosquitto test broker.
static const char* MQTT_HOST   = "test.mosquitto.org";
static const char* MQTT_PORT   = "8883";         
static const char* CLIENT_ID   = "DSI-Node-01";  

static const char* DATA_TOPIC   = "Test/DSI";
static const char* STATUS_TOPIC = "test/topicmqtts"; 

static const uint32_t PUB_PERIOD_MS = 5000;

uint32_t lastPub = 0;

static bool mqttStartAndConnect() {
  
  TB_Broker0.addr           = MQTT_HOST;
  TB_Broker0.port           = MQTT_PORT;
  TB_Broker0.mqttId         = 0;
  TB_Broker0.keepalive_time = 300;     
  TB_Broker0.clean_session  = true;
  TB_Broker0.Server_type    = 1;        

  if (!IIOT_Dev_kit.MQTT_SETUP(&TB_Broker0, TB_Broker0.addr, TB_Broker0.port)) {
    Serial.println(F("[ERR] MQTT_SETUP failed"));
    return false;
  }


  if (!IIOT_Dev_kit.MQTT_CONNECT(&TB_Broker0, CLIENT_ID)) {
    Serial.println(F("[ERR] MQTT_CONNECT failed"));
    return false;
  }

  if (!IIOT_Dev_kit.MQTT_PUB(&TB_Broker0, STATUS_TOPIC, "DSI_ONLINE", 2, 60, true, false)) {
    Serial.println(F("[WARN] Birth (ONLINE) publish failed"));
  } else {
    Serial.println(F("[OK] Birth (ONLINE) retained"));
  }

  return true;
}

static String buildJsonPayload() {
  // Demo values. Replace these with actual sensor readings.
  float k_type_c = 20.0f + (random(0, 3300) / 10.0f);   
  float ir_c     = 20.0f + (random(0, 1000) / 10.0f);   
  float ct1_a    = (random(0, 500) / 10.0f);            
  float ct2_a    = (random(0, 500) / 10.0f);           

  String json = "{";
  json += "\"k_type_c\":"; json += String(k_type_c, 1); json += ",";
  json += "\"ir_c\":";     json += String(ir_c, 1);     json += ",";
  json += "\"ct1_a\":";    json += String(ct1_a, 1);    json += ",";
  json += "\"ct2_a\":";    json += String(ct2_a, 1);
  json += "}";
  return json;
}

static bool publishData() {
  String payload = buildJsonPayload();

  const bool retain_data = true;   
  bool ok = IIOT_Dev_kit.MQTT_PUB(&TB_Broker0, DATA_TOPIC, payload, 2, 60, retain_data, false);

  Serial.print(F("PUB ")); Serial.print(DATA_TOPIC);
  Serial.print(F(" QoS2 retain=")); Serial.print(retain_data ? "yes" : "no");
  Serial.print(F(" -> ")); Serial.println(ok ? F("OK") : F("FAIL"));

  return ok;
}

void setup() {
  Serial.begin(115200);
  delay(100);

  randomSeed(esp_random()); 

  if (!IIOT_Dev_kit.Init(115200)) {
    Serial.println(F("[ERR] Modem init failed"));
    return;
  }
  Serial.println(F("[OK] Modem initialized"));

  if (!IIOT_Dev_kit.SET_APN(APN_CID, APN_TYPE, APN_NAME)) {
    Serial.println(F("[ERR] APN setup failed"));
   
  } else {
    Serial.println(F("[OK] APN set"));
  }

  String rssi;
  if (IIOT_Dev_kit.CSQ(&rssi)) {
    Serial.print(F("[INFO] CSQ: ")); Serial.println(rssi);
  }

 
  if (IIOT_Dev_kit.IS_ATTACH())                 Serial.println(F("[OK] Registered to network"));
  else                                          Serial.println(F("[WARN] Not registered yet"));

  if (IIOT_Dev_kit.IS_PACKET_DOMAIN_ATTACH())   Serial.println(F("[OK] Packet domain attached"));
  else                                          Serial.println(F("[WARN] Packet domain not attached"));

  if (!mqttStartAndConnect()) {
    Serial.println(F("[FATAL] MQTT not connected"));
  } else {
    Serial.println(F("[OK] MQTT connected (TLS)"));
  }

  lastPub = millis();
}

void loop() {

  if (millis() - lastPub >= PUB_PERIOD_MS) {
    lastPub = millis();


    if (!publishData()) {
      Serial.println(F("[WARN] Publish failed - trying reconnect"));
      IIOT_Dev_kit.MQTT_DISCONNECT(&TB_Broker0);
      delay(500);
      if (mqttStartAndConnect()) {
        publishData(); 
      }
    }
  }


  unsigned long t0 = millis();
  while (millis() - t0 < 10 && Serial2.available()) {
    Serial.write(Serial2.read());
  }
}
