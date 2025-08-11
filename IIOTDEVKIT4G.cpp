#ifndef IIOTDEVKIT4G_H
#include "IIOTDEVKIT4G.h"
#endif

bool IIOTDEVKIT4G::Init(unsigned long buad_rate)
{
  // port config and buad rate set 
  pinMode(PWR_PIN, OUTPUT);
  Serial2.begin(buad_rate);
  // PWR on pin signal
  digitalWrite(PWR_PIN, HIGH);
  delay(600);
  digitalWrite(PWR_PIN, LOW);
  delay(2500);
  // disable echo mode.
  if(SENDATCMD("ATE0\r\n", 2000, "OK", "ERROR")!=1){
    return false;
  }
  delay(100);
  return AT_TEST();
}

bool IIOTDEVKIT4G::PWRDOWN()
{
  
  uint8_t answer = SENDATCMD("AT+CPOF\r\n", 2000, "OK", "error");

  if (answer == 1)
  {
    digitalWrite(PWR_PIN, LOW);   
    return true;
  }
  return false;
  
}
bool IIOTDEVKIT4G::CSQ(String *response)
{
  
  uint8_t answer=SEND_AT_CMD_RAW("AT+CSQ\r\n",2000,response);
  return answer;

}


bool IIOTDEVKIT4G::SET_APN(String CID,String PDP_type,String APNNAME)
{
  uint8_t answer = SENDATCMD("AT+CFUN=0\r\n", 2000, "OK", "ERROR");
  if (answer == 1)
  {
    String atCommand = "AT+CGDCONT="+CID+",\""+PDP_type+"\",\""+ APNNAME + "\",\"\",0,0,,,,\r\n";
    //Serial.print(atCommand);
    char charArray[atCommand.length()];
    atCommand.toCharArray(charArray, atCommand.length());
    answer = SENDATCMD(charArray, 4000, "OK", "ERROR");
    if (answer == 1)
    {
      uint8_t answer = SENDATCMD("AT+CFUN=1\r\n", 2000, "OK", "ERROR");
      if (answer == 1)
      {
        return true;
      }
      else
      {
        return false;
      }
    }
    else
    {
      return false;
    }
  }
  else
  {
    // Serial.println("AP  set  Error");
    return false;
  }
}

bool IIOTDEVKIT4G ::IS_ATTACH()
{
  String response;
  uint8_t answer=SEND_AT_CMD_RAW("AT+CREG?\r\n",2000,&response);
  if (answer)
  {
    //Serial.println(response[8]);
    if((response[7]=='0') && (response[9]=='1')) return true;
  }
  return false;
}

bool IIOTDEVKIT4G ::IS_PACKET_DOMAIN_ATTACH()
{
  String response;
  uint8_t answer=SEND_AT_CMD_RAW("AT+CGATT?\r\n",2000,&response);
  if (answer)
  {
    //Serial.println(response);
    if(response[8]=='1') return true;
  }
  return false;
}

bool IIOTDEVKIT4G ::GET_IP()
{
  String response;
  uint8_t answer = SEND_AT_CMD_RAW("AT+CGCONTRDP\r\n",2000,&response);

  //if (response =="ERROR")
  //response = +CGCONTRDP: 1,5,"nbiot","10.106.221.48.255.255.255.0"
  //_NetStat.IP = 
  // uint8_t answer = SENDATCMD("AT+CGCONTRDP?\r\n", 2000, "OK", "ERROR");
  Serial.println(response);
  if (answer == 1)
  {
    return true;
  }
  else 
  {
    return false;
  }
}

bool IIOTDEVKIT4G::MQTT_SETUP(Broker *broker, String server, String port)
{
  broker->addr = server;
  broker->port = port;

  // 1) Delete old CA if present
  SENDATCMD("AT+CCERTDELE=\"mosquitto.org.pem\"\r\n",
            20000, "OK", "ERROR");

  // 2) Tell MQTT to use SSL context 0
  SENDATCMD("AT+CMQTTSSLCFG=0,0\r\n",
            2000, "OK", "ERROR");

  // 3) Download new CA into slot 0
  {
    char buf[64];
    unsigned len = strlen_P(mosq_ca_cert);
    snprintf(buf, sizeof(buf),
             "AT+CCERTDOWN=\"mosquitto.org.pem\",%u\r\n", len);

    // issue command and wait for '>' prompt
    if (SENDATCMD(buf, 120000, ">", "ERROR") != 1) return false;

    // stream PEM bytes
    const char *p = mosq_ca_cert;
    while (*p) { Serial2.write(*p++); }
    Serial2.write("\r\n", 2);

    // wait for OK (or ERROR on failure)
    if (!waitForOK(120000)) return false;
  }

  // 4) Start MQTT service
  Serial2.println("AT+CMQTTSTART");
  // wait for the success URC; if it never comes, see if we at least got OK
  if (!waitForURC("+CMQTTSTART: 0", 30000)) {
    // OK without URC means “already started”—ignore it
    waitForOK(1000);
  }
  return true;
}


bool IIOTDEVKIT4G::MQTT_STOP()
{
  // Stop MQTT service
  uint8_t ans = SENDATCMD("AT+CMQTTSTOP\r\n",
                          30000,
                          "+CMQTTSTOP: 0",
                          "ERROR");
  return (ans == 1 || ans == 2);
}


bool IIOTDEVKIT4G ::MQTT_DISCONNECT(Broker *broker)
{
  return MQTT_DISCONNECT(broker, 0);
}

bool IIOTDEVKIT4G::MQTT_DISCONNECT(Broker *broker, uint timeout)
{
  // 1) Build the command string
  String atCommand = "AT+CMQTTDISC=" + String(broker->mqttId);
  if (timeout != 0) {
    atCommand += "," + String(timeout);
  }
  atCommand += "\r\n";

  // 2) The module emits "+CMQTTDISC:0,0" (no space after ':')
  String expected = "+CMQTTDISC:" + String(broker->mqttId) + ",0";

  // 3) Send and wait for that URC (or ERROR)
  uint8_t answer = SENDATCMD(atCommand.c_str(),
                             30000,
                             expected.c_str(),
                             "ERROR");

  if (answer == 1) {
    Serial.println("CMQTTDISC ok");
    // 4) Release the client handle
    return MQTT_RELESECLIENT(broker);
  }

  // any other result (including ans==0 or ==2) is a failure
  return false;
}



bool IIOTDEVKIT4G::MQTT_CONNECT(Broker *broker,
                                String clientid,
                                String Username,
                                String password)
{
  Serial.println(">>> MQTT_CONNECT: start");
  broker->username = Username;
  broker->password = password;

  // 1) Acquire client (ignore OK vs ERROR)
  String cmd1 = "AT+CMQTTACCQ=" + String(broker->mqttId)
                + ",\"" + clientid + "\",1";
  Serial.print  (">>> TX: "); Serial.println(cmd1);
  Serial2.println(cmd1);
  // wait up to 40s for any OK or ERROR
  unsigned long t0 = millis();
  while (millis() - t0 < 40000) {
    if (!Serial2.available()) continue;
    String line = Serial2.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;
    Serial.print("<<< "); Serial.println(line);
    if (line.indexOf("OK") >= 0 || line.indexOf("ERROR") >= 0)
      break;
  }

  String willTopic = "test/topicmqtts";
  String cmdWillTopic = "AT+CMQTTWILLTOPIC=" 
                        + String(broker->mqttId)
                        + "," + String(willTopic.length());
  Serial.print  (">>> TX: "); Serial.println(cmdWillTopic);
  Serial2.println(cmdWillTopic);
  // wait for the “>” prompt before sending the actual topic
  if (Serial2.find(">")) {
    Serial2.print(willTopic);
    Serial2.write('\r');
  }
  // consume the OK
  while (!Serial2.find("OK")) { /* spin */ }


  String willMsg = "DSI_OFFLINE(LWT)";
  String cmdWillMsg = "AT+CMQTTWILLMSG=" 
                      + String(broker->mqttId)
                      + "," + String(willMsg.length())
                      + ",2";              
  Serial.print  (">>> TX: "); Serial.println(cmdWillMsg);
  Serial2.println(cmdWillMsg);
  // wait for the “>” prompt
  if (Serial2.find(">")) {
    Serial2.print(willMsg);
    Serial2.write('\r');
  }
  // consume the OK
  while (!Serial2.find("OK")) { /* spin */ }

  // 2) Issue CONNECT
  String cmd2 = "AT+CMQTTCONNECT=" + String(broker->mqttId)
                + ",\"tcp://" + broker->addr + ":8883\","
                + String(broker->keepalive_time) + ","
                + String(broker->clean_session ? 1 : 0);
  Serial.print  (">>> TX: "); Serial.println(cmd2);
  Serial2.println(cmd2);

  // 3) Read and print all lines until we see success URC or ERROR or timeout
  String successURC = "+CMQTTCONNECT: " 
                      + String(broker->mqttId) + ",0";
  bool gotOK  = false;
  bool gotURC = false;
  t0 = millis();
  while (millis() - t0 < 40000) {
    if (!Serial2.available()) continue;
    String line = Serial2.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;
    Serial.print("<<< "); Serial.println(line);

    if (line.indexOf("OK") >= 0) {
      gotOK = true;
      continue;
    }
    if (line.indexOf(successURC) >= 0) {
      Serial.println(">>> Received success URC");
      gotURC = true;
      break;
    }
    if (line.indexOf("ERROR") >= 0) {
      Serial.println(">>> Received ERROR");
      break;
    }
  }

  if (!gotURC) {
    Serial.print(">>> Connect failed ");
    if (!gotOK)  Serial.print("(no OK seen) ");
    if (!gotURC) Serial.print("(no URC seen) ");
    Serial.println();
    Serial.println(">>> Releasing client");
    MQTT_RELESECLIENT(broker);
    return false;
  }

  Serial.println(">>> MQTT_CONNECT: success!");
  return true;
}


bool IIOTDEVKIT4G::MQTT_RELESECLIENT(Broker *broker)
{
  // Release client index
  String cmd = "AT+CMQTTREL=" + String(broker->mqttId) + "\r\n";
  return (SENDATCMD(cmd.c_str(), 20000, "OK", "ERROR") == 1);
}

bool IIOTDEVKIT4G::MQTT_CONNECT(Broker *broker, String clientid) {
  return MQTT_CONNECT(broker, clientid, "", "");
}
bool IIOTDEVKIT4G::MQTT_CONNECT(Broker *broker, String clientid, String Username) {
  return MQTT_CONNECT(broker, clientid, Username, "");
}

bool IIOTDEVKIT4G::MQTT_SETTOPIC(Broker *broker, String topic)
{
  uint topic_len = topic.length();
 // Serial.print("topic length - ");
 // Serial.println(topic_len);

  if((topic_len<1)||(topic_len>1023)) return false;

  
  String atCommand = "AT+CMQTTTOPIC="+String(broker->mqttId)+","+String(topic_len)+"\r\n";
  char charArray[atCommand.length()];
  atCommand.toCharArray(charArray, atCommand.length());

  while(Serial2.available()){
    Serial2.read();
  }

  Serial2.write(charArray); //send array to module

  bool OK=false;
  uint long previous = millis();
  do{
    if(Serial2.available()){
      char val = Serial2.read();
      if(val == '>'){
          OK = true;
      }
    }
  }while((OK == false) && ((millis()-previous)<3000));
  if(OK==false){
    return false;
  }

  //Serial.println("topic responce c");
  char topicarray[topic.length()];
  topic.toCharArray(topicarray, topic.length()+1);

  String response2;
  bool answer = SEND_AT_CMD_RAW(topicarray, 30000, &response2);

  if(answer && (response2[0]=='O') && (response2[1]=='K')){
    return true;
  }
  else{
    return false;
  }
}

bool IIOTDEVKIT4G::MQTT_PAYLOAD(Broker *broker, String msg){

  uint msg_len = msg.length();
  //Serial.print("msg length - ");
  //Serial.println(msg_len);

  if((msg_len<1)||(msg_len>10240)) return false;

  String atCommand = "AT+CMQTTPAYLOAD="+String(broker->mqttId)+","+String(msg_len)+"\r\n";
  char charArray[atCommand.length()];
  atCommand.toCharArray(charArray, atCommand.length());

  while(Serial2.available()){
    Serial2.read();
  }

  Serial2.write(charArray); //send array to module

  bool OK=false;
  uint long previous = millis();
  do{
    if(Serial2.available()){
      if(Serial2.read() == '>'){
          OK = true;
      }
    }
  }while((OK == false) && ((millis()-previous)<20000));
  if(OK==false){
    return false;
  }
 
  char msgarray[msg_len];
  msg.toCharArray(msgarray, msg_len+1);

  String response2;
  bool answer = SEND_AT_CMD_RAW(msgarray, 30000, &response2);
  
  if(answer && (response2[0]=='O') && (response2[1]=='K')){
    return true;
  }
  else{
    return false;
  }
}

bool IIOTDEVKIT4G::MQTT_PUB(Broker *broker,uint8_t qos, uint pub_timeout, bool retained, bool dup){

  String atCommand = "AT+CMQTTPUB="+String(broker->mqttId)+"," + String(qos) +","+ String(pub_timeout)+","+String(retained)+","+String(dup)+"\r\n";
  char charArray[atCommand.length()];
  atCommand.toCharArray(charArray, atCommand.length());
 
  String response;
  bool answer = SEND_AT_CMD_RAW(charArray, 30000, &response);

  if(answer && (response[0]=='O') && (response[1]=='K')){
    return true;
  }
  else{
    return false;
  }

}

bool IIOTDEVKIT4G::MQTT_PUB(Broker *broker, String topic, String msg)
{
  return MQTT_PUB(broker, topic, msg, 0, 5, 0, 0);
}

bool IIOTDEVKIT4G::MQTT_PUB(Broker *broker, String topic, String msg, uint8_t qos, uint pub_timeout, bool retained, bool dup)
{
  /*InPut the topic of publish message*/
  
  if(MQTT_SETTOPIC(broker, topic)==false){
    Serial.println("topic fail");
    return false;
  }

  /*Input the publish message*/
  if(MQTT_PAYLOAD(broker, msg)==false){
    Serial.println("msg fail");
    return false;
  }

  if(MQTT_PUB(broker,qos,pub_timeout,retained,dup)==false)
  {
    Serial.println("pub fail");
     return false;
  }

  return true;
}

uint8_t IIOTDEVKIT4G::MQTTSUB(Broker *broker, String topic, uint8_t qos)
{
  /* Input validation */
  if (qos < 0 || qos > 2)
    return 0xe0; /* QoS must be 0, 1, 2 */

  String atCommand = "AT+CMQSUB=" + String(broker->mqttId) + ",\"" + topic + "\"," + qos + "\r\n";
  char charArray[atCommand.length()];
  atCommand.toCharArray(charArray, atCommand.length());
  uint8_t answer = SENDATCMD(charArray, 4000, "OK", "ERROR");
  return answer == 1 ? 0x01 : answer;
}

uint8_t IIOTDEVKIT4G::MQTTUNSUB(Broker *broker, String topic)
{
  String atCommand = "AT+CMQUNSUB=" + String(broker->mqttId) + "," + topic + "\r\n";
  char charArray[atCommand.length()];
  atCommand.toCharArray(charArray, atCommand.length());
  uint8_t answer = SENDATCMD(charArray, 4000, "OK", "ERROR");
  return answer == 1 ? 0x01 : answer;
}

bool IIOTDEVKIT4G::SEND_AT_CMD_RAW(const char *at_command,
                                  unsigned int timeout,
                                  String *resp)
{
  uint8_t x = 0;
  char response[100] = {0}; // uint8 responce[100] -
  unsigned long previous;
  bool buffer_start = false;
  bool buffer_end = false;

  while (Serial2.available() > 0)
    Serial2.read(); // Clean the input buffer

  // delay(100);
  //Serial.write(at_command); // for dubug
  Serial2.write(at_command); // Send the AT command

  // delay(00);
  bool IS_CGEV=false;
  do{
    previous = millis();
    x=0;
    IS_CGEV=false;

    do
    {
      if (Serial2.available())
      {
        char tem1 = Serial2.read();
        if (tem1 = '\r')
        {
          char tem2 = Serial2.read();
          if (tem1 = '\n')
          {
            buffer_start = true;
          }
        }
      }
      delay(10);
    } while (!buffer_start && ((millis() - previous) < timeout));
    if (!buffer_start)
    {
      return false;
    }

    do
    {
      if (Serial2.available())
      {
        response[x] = Serial2.read();
        x++;
      }
    
      if (x > 1)
      {
        char tempArray[2] = {0};
        tempArray[0] = response[x-2];
        tempArray[1] = response[x-1];
        if (strstr(tempArray, "\r\n") != NULL)
        {
          buffer_end = true;
        }
      }
    } while (!buffer_end && ((millis() - previous) < timeout));
    if (!buffer_end)
    {
      return false;
    }

    /*+CGEV check*/
    if((response[0]=='+')&&(response[1]=='C')&&(response[2]=='G')&&(response[1]=='E')){
      Serial.println("detect +CGEV CMD");
      IS_CGEV = true;
    }
  }while(IS_CGEV);

  char tempStr[100] = {0};
  uint pointer=0;
  for(uint8_t i = 0; i < x - 2; i++) {
    if(response[i]==NULL || response[i]=='\r' || response[i]=='\n'){
      Serial.println("null, newline detected");
    }else{
      tempStr[pointer]=response[i];
      pointer++;
    }
  }
  *resp = String(tempStr);
  return true;
}


uint8_t IIOTDEVKIT4G::SENDATCMD(const char* at_command, unsigned int timeout, const char *expected_answer1, const char *expected_answer2)
{

  uint8_t x = 0, answer = 0;
  char response[100] = {0};

  unsigned long previous;

  while (Serial2.available() > 0)
    Serial2.read(); // Clean the input buffer

  delay(100);

  //Serial.write(at_command); // for debug
  Serial2.write(at_command); // Send the AT command

  delay(500);

  previous = millis();
  // this loop waits for the answer
  do
  {
    // if there are data in the UART input buffer, reads it and checks for the asnwer
    if (Serial2.available() != 0)
    {
      response[x] = Serial2.read();
      //Serial.print(response[x]);
      if (expected_answer1 != "")
      {
        // check if the desired answer   is in the response of the module
        if (strstr(response, expected_answer1) != NULL)
        {
          answer = 1;
          delay(100);
        }
        else if (expected_answer2 != "")
        {
          if (strstr(response, expected_answer2) != NULL)
          {
            answer = 2;
            delay(100);
          }
        }
      }
      else
      {
        // Serial.print(response);
      }
      x++;
      delay(10);
    }

  } while ((answer == 0) && ((millis() - previous) < timeout));

  // if (answer == 0) {
  //   Serial.println("AT response Time Out");
  // }
  return answer;
}
bool IIOTDEVKIT4G::waitForResponse(const char* ok,
                                   const char* error,
                                   unsigned int timeout) {
  unsigned long t0 = millis();
  String line;
  while (millis() - t0 < timeout) {
    if (Serial2.available()) {
      line = Serial2.readStringUntil('\n');
      if (line.indexOf(ok)    >= 0) return true;
      if (line.indexOf(error) >= 0) return false;
    }
  }
  return false;
}
bool IIOTDEVKIT4G::waitForOK(unsigned int timeout) {
  unsigned long start = millis();
  while (millis() - start < timeout) {
    if (Serial2.available()) {
      String line = Serial2.readStringUntil('\n');
      if (line.indexOf("OK") >= 0) return true;
    }
  }
  return false;
}

bool IIOTDEVKIT4G::waitForURC(const char* urc, unsigned int timeout) {
  unsigned long start = millis();
  while (millis() - start < timeout) {
    if (Serial2.available()) {
      String line = Serial2.readStringUntil('\n');
      if (line.indexOf(urc) >= 0) return true;
    }
  }
  return false;
}


bool IIOTDEVKIT4G::AT_TEST()
{
  uint8_t answer = SENDATCMD("AT\r\n", 1000, "OK", "ERROR");
  if (answer == 1)
  {
    return true;
  }
  else
  {
    return false;
  }
}
