// LiKiLink WIFI Bridge for LipoKiller2
// By: Ulf Karlsson <ulk@ulwur.net>
// Version: 0.1 2018-03-30
// This software is released under the GPL. 
// Board: Wemos D1 R2 Mini

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <SoftwareSerial.h>
#include <PubSubClient.h>
#include <Ticker.h>

// variables to hold the parsed data
const byte numChars = 128;
char receivedChars[numChars];
char tempChars[numChars];        // temporary array for use when parsing
boolean newData = false;

String MsgTime;
String MsgCel1;
String MsgCel2;
String MsgCel3;
String MsgCel4;
String MsgCurr;
String MsgLoad;

String ValTime;
String ValCel1;
String ValCel2;
String ValCel3;
String ValCel4;
String ValCurr;
String ValLoad;

char ar_ValTime[10];
char ar_ValCel1[10];
char ar_ValCel2[10];
char ar_ValCel3[10];
char ar_ValCel4[10];
char ar_ValCurr[10];
char ar_ValLoad[10];

//for LED status
Ticker ticker;

//SoftwareSerial settings & create
const int rxPin = D1;  //D1 on D1 Mini
const int txPin = D0;  //D0 on D1 Mini
SoftwareSerial swSer(rxPin, txPin, false, 256);  




//mqtt setting & create
const char* mqtt_server = "192.168.0.202";
const char* mqtt_client_name = "LiKiLink";
const char* mqtt_topic = "LiKiLink";
WiFiClient espClient;
PubSubClient client(espClient);

//Telnet server settings
#define MAX_SRV_CLIENTS 1
WiFiServer server(23);
WiFiClient serverClients[MAX_SRV_CLIENTS];


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  swSer.begin(19200);

  Serial.println("\nLiKi (Link to LiPoKiller2) start");


  
  //set led pin as output
  pinMode(BUILTIN_LED, OUTPUT);
  // start ticker with 0.5 because we start in AP mode and try to connect
  ticker.attach(0.6, tick);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset settings - for testing
  //wifiManager.resetSettings();

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect()) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
  ticker.detach();
  //keep LED on
  digitalWrite(BUILTIN_LED, LOW);

  //telnet
  server.begin();
  server.setNoDelay(true);

  //mqtt
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  if (client.connect(mqtt_client_name)) {
    Serial.println("Connected to MQTT broker");
    Serial.print("Topic is: ");
    Serial.println(mqtt_topic);
 
    if (client.publish(mqtt_topic, "hello from ESP8266")) {
      Serial.println("Publish ok");
    }
    else {
      Serial.println("Publish failed");
    }
  }
  else {
    Serial.println("MQTT connect failed");
    Serial.println("Will reset and try again...");
    abort();
  }

  Serial.print("Ready! Use 'telnet ");
  Serial.print(WiFi.localIP());
  Serial.println(" on port 23' to connect");

}

void loop() {
  uint8_t i;
  //check if there are any new clients
  if (server.hasClient()) {
    for (i = 0; i < MAX_SRV_CLIENTS; i++) {
      //find free/disconnected spot
      if (!serverClients[i] || !serverClients[i].connected()) {
        if (serverClients[i]) {
          serverClients[i].stop();
        }
        serverClients[i] = server.available();
        Serial1.print("New client: "); Serial1.print(i);
        break;
      }
    }
    //no free/disconnected spot so reject
    if (i == MAX_SRV_CLIENTS) {
      WiFiClient serverClient = server.available();
      serverClient.stop();
      Serial1.println("Connection rejected ");
    }
  }
  
    recvWithStartEndMarkers();

  if (newData == true) {
 
	for (i = 0; i < MAX_SRV_CLIENTS; i++) {
      if (serverClients[i] && serverClients[i].connected()) {
        serverClients[i].write(receivedChars, numChars);
        serverClients[i].write("\n\r", 2);
        delay(1);
      }
    }
    if(StartsWith(receivedChars, "Dchg")) {
		MsgTime = GetStringPartAtSpecificIndex(receivedChars, ',' , 0);
		MsgCel1 = GetStringPartAtSpecificIndex(receivedChars, ',' , 1);
		MsgCel2 = GetStringPartAtSpecificIndex(receivedChars, ',' , 2);
		MsgCel3 = GetStringPartAtSpecificIndex(receivedChars, ',' , 3);
		MsgCel4 = GetStringPartAtSpecificIndex(receivedChars, ',' , 4);
		MsgCurr = GetStringPartAtSpecificIndex(receivedChars, ',' , 5);
		MsgLoad = GetStringPartAtSpecificIndex(receivedChars, ',' , 6); 
		showParsedData();

		ValTime = GetStringPartAtSpecificIndex(MsgTime, ':' , 1);
		ValCel1 = GetStringPartAtSpecificIndex(MsgCel1, ':' , 1);
		ValCel2 = GetStringPartAtSpecificIndex(MsgCel2, ':' , 1);
		ValCel3 = GetStringPartAtSpecificIndex(MsgCel3, ':' , 1);
		ValCel4 = GetStringPartAtSpecificIndex(MsgCel4, ':' , 1);
		ValCurr = GetStringPartAtSpecificIndex(MsgCurr, ':' , 1);
		ValLoad = GetStringPartAtSpecificIndex(MsgLoad, ':' , 1); 
		showParsedData2();
		
		//mqtta
    ValTime.toCharArray( ar_ValTime ,10);
    ValCel1.toCharArray( ar_ValCel1 ,10);
    ValCel2.toCharArray( ar_ValCel2 ,10);
    ValCel3.toCharArray( ar_ValCel3 ,10);
    ValCel4.toCharArray( ar_ValCel4 ,10);
    ValCurr.toCharArray( ar_ValCurr ,10);
    ValLoad.toCharArray( ar_ValLoad ,10);
 
    client.publish("LiKiLink/time", ar_ValTime);
    client.publish("LiKiLink/cell1", ar_ValCel1);
    client.publish("LiKiLink/cell2", ar_ValCel2);
    client.publish("LiKiLink/cell3", ar_ValCel3);
    client.publish("LiKiLink/cell4", ar_ValCel4);
    client.publish("LiKiLink/curr", ar_ValCurr);
    client.publish("LiKiLink/load", ar_ValLoad);

    }
  newData = false;
  }

  //Yelda  
  yield;
}

void tick()
{
  //toggle state
  int state = digitalRead(BUILTIN_LED);  // get the current state of GPIO1 pin
  digitalWrite(BUILTIN_LED, !state);     // set pin to the opposite state
}

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
}

// handles message arrived on subscribed topic(s)
void callback(char* topic, byte* payload, unsigned int length) {

  Serial.println("Message arrived:  topic: " + String(topic));
  Serial.println("Length: " + String(length,DEC)); 
 
}

//Splits strrings
String GetStringPartAtSpecificIndex(String StringToSplit, char SplitChar, int StringPartIndex)
{
  int cnt = 0;
  int pos = 0;
  for (int il = 0; il < StringToSplit.length(); il++)
  {
    if (cnt == StringPartIndex)
    {
      int bpos = pos;
      while (pos < StringToSplit.length() && StringToSplit[pos] != SplitChar) pos++;
      return StringToSplit.substring(bpos, pos);
    }
    if (StringToSplit[il] == SplitChar)
    {
      pos = il + 1;
      cnt++;
    }
  }
  return "";
}


//Reads Serial port for lines
void recvWithStartEndMarkers() {
    static boolean recvInProgress = false;
    static byte ndx = 0;
    char startMarker = ' ';
    char endMarker = '\r';
    char rc;

    while (swSer.available() > 0 && newData == false) {  //Serial eller swSer
        rc = swSer.read();

        if (recvInProgress == true) {
            if (rc != endMarker) {
                receivedChars[ndx] = rc;
                ndx++;
                if (ndx >= numChars) {
                    ndx = numChars - 1;
                }
            }
            else {
                receivedChars[ndx] = '\0'; // terminate the string
                recvInProgress = false;
                ndx = 0;
                newData = true;
            }
        }

        else if (rc == startMarker) {
            recvInProgress = true;
        }
    }
}

void showParsedData() {
    Serial.println("Message ");
    Serial.println(MsgTime);
    Serial.println(MsgCel1);
    Serial.println(MsgCel2);
    Serial.println(MsgCel3);
    Serial.println(MsgCel4);
    Serial.println(MsgCurr);
    Serial.println(MsgLoad);
}

void showParsedData2() {
    Serial.println("Values ");  
    Serial.println(ValTime);
    Serial.println(ValCel1);
    Serial.println(ValCel2);
    Serial.println(ValCel3);
    Serial.println(ValCel4);
    Serial.println(ValCurr);
    Serial.println(ValLoad);
    Serial.println();
}

int StartsWith(const char *a, const char *b)
{
   if(strncmp(a, b, strlen(b)) == 0) return 1;
   return 0;
}

