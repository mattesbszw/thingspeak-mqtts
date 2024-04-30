#include "tscert.h"

#include <DHT.h>
#include <WiFi.h>
#include <PubSubClient.h>                 

#define DHT_PIN 4
#define DHTTYPE DHT11


DHT dht(DHT_PIN, DHTTYPE);


// WLAN-Zugangsdaten
const char* ssid = "CPSLABOR";
const char* password = "A1234567890";

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);           //MQTT-Client erzeugen

const char* MQTT_SERVER = "mqtt3.thingspeak.com";
const long MQTT_PORT = 1883;

// Thingspeak-MQTT-Client-Daten anpassen

const long THINGSPEAK_CHANNEL = 0;
const char MQTT_USERNAME[] = "YOURUSERNAMEHERE";            
const char MQTT_PASSWORD[] = "YOURPASSWORDHERE";         
const char MQTT_CLIENTID[] = "YOURCLIENTIDHERE";


void setup(void){
  
  Serial.begin(115200);
  dht.begin();              // init DHT sensor
  
  connectWiFi();

  //MQTT-Client wird initialisiert
  mqttClient.setServer( MQTT_SERVER, MQTT_PORT );
  mqttClient.setCallback( mqttSubscriptionCallback );
  mqttClient.setBufferSize( 2048 );

}

//hier wird wird der ESP mit dem WLAN verbunden
void connectWiFi(){
  
  WiFi.mode(WIFI_STA);
  Serial.print("Connecting to " + String(ssid));
  while(WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    WiFi.begin(ssid, password); 
    delay(5000);     
  } 
  Serial.println("\nConnected.");  
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  
}

void loop(void){ 

  // Temperatur auslesen
  float hTmp = dht.readTemperature();
  String dhtTemperature = "0";
  if (!isnan(hTmp)) {
    dhtTemperature = hTmp;
  }
  Serial.print("Sensor (*C): "); 
  Serial.println(dhtTemperature);
 
  // Luftfeuchtigkeit auslesen
  float hHum = dht.readHumidity();
  String dhtHumidity = "0";
  if (!isnan(hHum)) {
    dhtHumidity = hHum;
  }
  Serial.print("Sensor (%): "); 
  Serial.println(dhtHumidity);

  //verbinden, falls noch nicht geschehen
  if (WiFi.status() != WL_CONNECTED) {
      connectWiFi();
  }
  if (!mqttClient.connected()) {
     mqttConnect(); 
     mqttSubscribe(THINGSPEAK_CHANNEL);
  }
  
  mqttClient.loop(); 
  
  // Update ThingSpeak channel periodically. The update results in the message to the subscriber.
  mqttPublish(THINGSPEAK_CHANNEL, (String("field1=" + String(dhtTemperature) + "&field2=" + String(dhtHumidity) )));
  
  delay(30000);
}

void mqttConnect(){
  /*hier soll sich der ESP mit dem Thingspeak-MQTT Broker verbinden.
  bitte hier eine Schleife verwenden, damit eine Verbindung garantiert wird*/
  while ( !mqttClient.connected() ){
    // Connect to the MQTT broker.
    if ( mqttClient.connect( MQTT_CLIENTID, MQTT_USERNAME, MQTT_PASSWORD ) ) {
      Serial.print( "MQTT to " );
      Serial.print( MQTT_SERVER );
      Serial.print (" at port ");
      Serial.print( MQTT_PORT );
      Serial.println( " successful." );
    } else {
      Serial.print( "MQTT connection failed, rc = " );
      Serial.print( mqttClient.state() );
      Serial.println( " Will try again in a few seconds" );
      delay(1000);
    }
  }
}

void reconnect(){
  /*Hier soll sich der Client neu verbinden*/
  Serial.println("Attempting MQTT connection");
  if (mqttClient.connect(MQTT_CLIENTID, MQTT_USERNAME, MQTT_PASSWORD))  {
    Serial.println("Conn:"+ String(MQTT_SERVER) + " cl: " + String(MQTT_CLIENTID)+ " Uname:" + String(MQTT_USERNAME));
  } else {
    Serial.println("Failed to connect. Trying to reconnect in 2 seconds");
    delay(2000);
  } 
}

void mqttSubscribe(long subChannelID){
  /*diese Funktion soll den übergebenen Channel abonieren*/
  String myTopic = "channels/"+String( subChannelID )+"/subscribe";
  mqttClient.subscribe(myTopic.c_str());
}

void mqttPublish(long pubChannelID, String message) {
  /*bei Aufruf dieser Funktion soll im übergebenen Channel eine Nachricht veröffentlicht werden
  Diese Nachricht soll verschiendene Felder beinhalten (Die beiden Temperaturen und vll die Luftfeuchtigkeit)*/
  String topicString ="channels/" + String( pubChannelID ) + "/publish";
  mqttClient.publish( topicString.c_str(), message.c_str() );
}

void mqttSubscriptionCallback( char* topic, byte* payload, unsigned int length ) {
  /*Diese Funktion wird aufgerufen, wenn ein abonnierter Channel geupdated wurde
  /Die empfangene Nachricht (payload) und das Topic sollen auf dem seriellen Monitor ausgegeben werden*/
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}
