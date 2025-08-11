// File: IIOTDEVKIT4G.h

#ifndef IIOTDEVKIT4G_H
#define IIOTDEVKIT4G_H

#include <Arduino.h>

// ← CHANGED: embed CA certificate for test.mosquitto.org (PEM format)
static const char mosq_ca_cert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIEAzCCAuugAwIBAgIUBY1hlCGvdj4NhBXkZ/uLUZNILAwwDQYJKoZIhvcNAQEL
BQAwgZAxCzAJBgNVBAYTAkdCMRcwFQYDVQQIDA5Vbml0ZWQgS2luZ2RvbTEOMAwG
A1UEBwwFRGVyYnkxEjAQBgNVBAoMCU1vc3F1aXR0bzELMAkGA1UECwwCQ0ExFjAU
BgNVBAMMDW1vc3F1aXR0by5vcmcxHzAdBgkqhkiG9w0BCQEWEHJvZ2VyQGF0Y2hv
by5vcmcwHhcNMjAwNjA5MTEwNjM5WhcNMzAwNjA3MTEwNjM5WjCBkDELMAkGA1UE
BhMCR0IxFzAVBgNVBAgMDlVuaXRlZCBLaW5nZG9tMQ4wDAYDVQQHDAVEZXJieTES
MBAGA1UECgwJTW9zcXVpdHRvMQswCQYDVQQLDAJDQTEWMBQGA1UEAwwNbW9zcXVp
dHRvLm9yZzEfMB0GCSqGSIb3DQEJARYQcm9nZXJAYXRjaG9vLm9yZzCCASIwDQYJ
KoZIhvcNAQEBBQADggEPADCCAQoCggEBAME0HKmIzfTOwkKLT3THHe+ObdizamPg
UZmD64Tf3zJdNeYGYn4CEXbyP6fy3tWc8S2boW6dzrH8SdFf9uo320GJA9B7U1FW
Te3xda/Lm3JFfaHjkWw7jBwcauQZjpGINHapHRlpiCZsquAthOgxW9SgDgYlGzEA
s06pkEFiMw+qDfLo/sxFKB6vQlFekMeCymjLCbNwPJyqyhFmPWwio/PDMruBTzPH
3cioBnrJWKXc3OjXdLGFJOfj7pP0j/dr2LH72eSvv3PQQFl90CZPFhrCUcRHSSxo
E6yjGOdnz7f6PveLIB574kQORwt8ePn0yidrTC1ictikED3nHYhMUOUCAwEAAaNT
MFEwHQYDVR0OBBYEFPVV6xBUFPiGKDyo5V3+Hbh4N9YSMB8GA1UdIwQYMBaAFPVV
6xBUFPiGKDyo5V3+Hbh4N9YSMA8GA1UdEwEB/wQFMAMBAf8wDQYJKoZIhvcNAQEL
BQADggEBAGa9kS21N70ThM6/Hj9D7mbVxKLBjVWe2TPsGfbl3rEDfZ+OKRZ2j6AC
6r7jb4TZO3dzF2p6dgbrlU71Y/4K0TdzIjRj3cQ3KSm41JvUQ0hZ/c04iGDg/xWf
+pp58nfPAYwuerruPNWmlStWAXf0UTqRtg4hQDWBuUFDJTuWuuBvEXudz74eh/wK
sMwfu1HFvjy5Z0iMDU8PUDepjVolOCue9ashlS4EB5IECdSR2TItnAIiIwimx839
LdUdRudafMu5T5Xma182OC0/u/xRlEm+tvKGGmfFcN0piqVl8OrSPBgIlb+1IKJE
m/XriWr/Cq4h/JfB7NTsezVslgkBaoU=
-----END CERTIFICATE-----
)EOF";

struct Broker {
  String addr;
  String port      = "1883";
  uint8_t version;
  uint8_t clientId;
  String username  = "0";
  String password  = "0";
  uint8_t mqttId   = 0;
  uint  keepalive_time = 3000;
  bool clean_session  = 0;
  bool Server_type    = 0;  // 0-> TCP, 1-> SSL/TLS
};

struct NetStat {
  String IP;
  String APN;
  String oper_long;
  String oper_short;
  String oper_num;
  String AcT;
};

#define PWR_PIN 12
#define RST_PIN 4

class IIOTDEVKIT4G {
public:
  bool Init(unsigned long buad_rate);
  bool AT_TEST();
  bool PWRDOWN();
  bool SET_APN(String CID, String PDP_type, String APNNAME);

  bool CSQ(String *response);
  bool IS_ATTACH();
  bool IS_PACKET_DOMAIN_ATTACH();
  bool GET_IP();

  // MQTT over TLS functions
  bool MQTT_SETUP(Broker *broker, String server, String port);
  bool MQTT_STOP();

  bool MQTT_CONNECT(Broker *broker, String clientid);
  bool MQTT_CONNECT(Broker *broker, String clientid, String Username);
  bool MQTT_CONNECT(Broker *broker, String clientid, String Username, String password);

  bool MQTT_RELESECLIENT(Broker *broker);
  bool MQTT_DISCONNECT(Broker *broker);
  bool MQTT_DISCONNECT(Broker *broker, uint timeout);

  bool MQTT_SETTOPIC(Broker *broker, String topic);
  bool MQTT_PAYLOAD(Broker *broker, String msg);

  bool MQTT_PUB(Broker *broker, uint8_t qos, uint pub_timeout, bool retained, bool dup);
  bool MQTT_PUB(Broker *broker, String topic, String msg);
  bool MQTT_PUB(Broker *broker, String topic, String msg, uint8_t qos, uint pub_timeout, bool retained, bool dup);

  uint8_t MQTTSUB(Broker *broker, String topic, uint8_t qos);
  uint8_t MQTTUNSUB(Broker *broker, String topic);

  bool SEND_AT_CMD_RAW(const char *at_command, unsigned int timeout, String* response);
  bool waitForResponse(const char* ok, const char* error, unsigned int timeout);
  bool waitForOK(unsigned int timeout);
  bool waitForURC(const char* urc, unsigned int timeout);


private:
  uint8_t SENDATCMD(const char* at_command, unsigned int timeout, const char* expected_answer1, const char* expected_answer2);
  NetStat _NetStat;
};

#endif
