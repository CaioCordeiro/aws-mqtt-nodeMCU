#include "FS.h"
#include <ESP8266WiFi.h>;
#include <PubSubClient.h>;
#include <NTPClient.h>
#include <WiFiUdp.h>

// Update these with values suitable for your network.

const char* ssid = "";
const char* password = "";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

const char* AWS_endpoint = ""; //MQTT broker ip
int ledState = LOW;

int ledPin = D2;
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  
  if ((char)payload[0] == 'h') {
    Serial.print((char)payload[0]);
     ledState = HIGH;
  } else {
    Serial.print((char)payload[0]);
    ledState = LOW;
  }

  Serial.println();

}
WiFiClientSecure espClient;
PubSubClient client(AWS_endpoint, 8883, callback, espClient); //set MQTT port number to 8883 as per //standard
long lastMsg = 0;
char msg[50];
int value = 0;
int buttonPin = 5;
int powerPin = A0;

int read_button()
{
  int buttonState = digitalRead(buttonPin);
  // You can send any value at any time.
  // Please don't send more that 10 values per second.
  Serial.print("V0 Button State value is: ");
  return (buttonState);
}

int read_power()
{
  int potencia = analogRead(powerPin);
  // You can send any value at any time.
  // Please don't send more that 10 values per second.
  Serial.print("V2 Power  value is: ");
  Serial.println(potencia);
  return (potencia);
}

void send_p_data()
{
  int p = read_power();
  //    Serial.print("Publish message: ");
  Serial.println(p);
  client.publish("powerTopic", String(p).c_str());
  //    Serial.print("Heap: ");
  Serial.println(ESP.getFreeHeap()); //Low heap can cause problems
}

void send_b_data()
{
  int b = read_button();
  //    Serial.print("Publish message: ");
  Serial.println(b);
  client.publish("buttonTopic",  String(b).c_str());
  //    Serial.print("Heap: ");
  Serial.println(ESP.getFreeHeap()); //Low heap can cause problems
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  espClient.setBufferSizes(512, 512);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  timeClient.begin();
  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }

  espClient.setX509Time(timeClient.getEpochTime());

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESPthing")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("ledTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");

      char buf[256];
      espClient.getLastSSLError(buf, 256);
      Serial.print("WiFiClientSecure SSL error: ");
      Serial.println(buf);

      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}



void setup() {

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  pinMode(D2, OUTPUT);
  setup_wifi();
  delay(1000);
  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }

  Serial.print("Heap: "); Serial.println(ESP.getFreeHeap());

  // Load certificate file
  File cert = SPIFFS.open("/cert.der", "r"); //replace cert.crt eith your uploaded file name
  if (!cert) {
    Serial.println("Failed to open cert file");
  }
  else
    Serial.println("Success to open cert file");

  delay(1000);

  if (espClient.loadCertificate(cert))
    Serial.println("cert loaded");
  else
    Serial.println("cert not loaded");

  // Load private key file
  File private_key = SPIFFS.open("/private.der", "r"); //replace private eith your uploaded file name
  if (!private_key) {
    Serial.println("Failed to open private cert file");
  }
  else
    Serial.println("Success to open private cert file");

  delay(1000);

  if (espClient.loadPrivateKey(private_key))
    Serial.println("private key loaded");
  else
    Serial.println("private key not loaded");

  // Load CA file
  File ca = SPIFFS.open("/ca.der", "r"); //replace ca eith your uploaded file name
  if (!ca) {
    Serial.println("Failed to open ca ");
  }
  else
    Serial.println("Success to open ca");

  delay(1000);

  if (espClient.loadCACert(ca))
    Serial.println("ca loaded");
  else
    Serial.println("ca failed");

  Serial.print("Heap: "); Serial.println(ESP.getFreeHeap());
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    send_p_data();
    send_b_data();
  }
  digitalWrite(D2, ledState);
}
