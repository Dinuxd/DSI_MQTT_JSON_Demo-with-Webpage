# DSI MQTT JSON Demo with Web Dashboard

This project demonstrates an IIoT telemetry flow using an Arduino-style C++ sketch, a 4G modem/dev kit, MQTT over TLS, JSON payloads, and a browser dashboard.

The device publishes simulated sensor values to a public MQTT broker. The webpage subscribes over secure WebSocket MQTT and displays the latest telemetry values live.

## Why This Repo Exists

This is the polished version of the earlier `DSI_MQTT_JSON_Demo` repository. The firmware is the same core demo, and this repo also includes the live web dashboard, so this should be the main public version.

## Features

- MQTT over TLS on port `8883`
- JSON telemetry payloads
- Retained online/offline status topic
- Automatic reconnect attempt after publish failure
- WebSocket MQTT dashboard using MQTT.js
- Demo values for K-type temperature, IR temperature, and two current sensors

## Hardware and Tools

- Arduino-compatible board with `Serial2`
- IIoT/4G modem dev kit controlled by AT commands
- SIM card with APN configured
- Arduino IDE or compatible build environment
- Browser for `index.html`

## MQTT Configuration

Default demo values:

```text
Broker: test.mosquitto.org
MQTT TLS port: 8883
MQTT WebSocket URL: wss://test.mosquitto.org:8081/mqtt
Data topic: Test/DSI
Status topic: test/topicmqtts
```

Before flashing, update these constants in `DSI_MQTT_JSON_Demo.ino`:

```cpp
static const char* APN_NAME = "your-apn";
static const char* CLIENT_ID = "DSI-Node-01";
static const char* DATA_TOPIC = "Test/DSI";
static const char* STATUS_TOPIC = "test/topicmqtts";
```

## Payload Format

The firmware publishes JSON like this:

```json
{
  "k_type_c": 26.5,
  "ir_c": 31.2,
  "ct1_a": 8.4,
  "ct2_a": 4.1
}
```

## Run the Dashboard

Open `index.html` in a browser. The dashboard connects to the public Mosquitto WebSocket endpoint and subscribes to the configured topics.

## Notes

- The CA certificate is embedded in `IIOTDEVKIT4G.h` for the Mosquitto test broker.
- The current sketch publishes demo random values. Replace `buildJsonPayload()` values with real sensor readings for production use.
- Public test MQTT brokers are for demos only. Use your own broker and unique topics for real deployments.

