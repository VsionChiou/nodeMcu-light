/*
 Basic ESP8266 MQTT example
 This sketch demonstrates the capabilities of the pubsub library in combination
 with the ESP8266 board/library.
 It connects to an MQTT server then:
  - publishes "hello world" to the topic "outTopic" every two seconds
  - subscribes to the topic "inTopic", printing out any messages
    it receives. NB - it assumes the received payloads are strings not binary
  - If the first character of the topic "inTopic" is an 1, switch ON the ESP Led,
    else switch it off
 It will reconnect to the server if the connection is lost using a blocking
 reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
 achieve the same result without blocking the main loop.
 To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SimpleDHT.h>
#include <Ticker.h>

// Update these with values suitable for your network.


#define light_high 2
#define light_low 5
#define light_en_pin 4
#define pinAM2320 3
#define pinLOW 1

const char* ssid = "Mansion";
const char* password = "12345666";
const char* mqtt_server = "192.168.50.150";

const char* light_command_topic =  "light/switch";                    
const char* brightness_topic = "light/brightness";                    
const char* rgb_topic =   "light/rgb";

const char* client_id = "client-light";     // 标识当前设备的客户端编号
const char* device_name = "esp_01_2_light_1";

const char* mqtt_user = "mqtt";
const char* mqtt_password = "12345666";

int last_brightness = 0;
int brightness = 1024;
int m_rgb_brightness = 100;
bool last_state = 0;

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

SimpleDHT22 dht22(pinAM2320);

float temperature = 0;
float humidity = 0;


int err = SimpleDHTErrSuccess;
Ticker ticker;


void HT(){
    if ((err = dht22.read2(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
      Serial.print("Read DHT22 failed, err="); Serial.print(SimpleDHTErrCode(err));
      Serial.print(","); Serial.println(SimpleDHTErrDuration(err)); delay(2000);
      return;
    }
    
    Serial.print("Sample OK: ");
    Serial.print((float)temperature); Serial.print(" *C, ");
    Serial.print((float)humidity); Serial.println(" RH%");
    char f[50];
    snprintf(f,50,"%f",temperature);
    client.publish("am2320/temperature",f);
    snprintf(f,50,"%f",humidity);
    client.publish("am2320/humidity",f);
}


void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  String message;

  int count=0;
  for (uint8_t i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);
  if(String(topic).equals(light_command_topic)){
      // Switch on the LED if an 1 was received as first character
      if (message.equals(String("OFF"))) {
        if(brightness>0){
          count = brightness;
          for(int i=count;i>=0;i--){
            Serial.println(i);
            analogWrite(light_high, i);
            delay(5);
          }
        }
        last_brightness = 0;
        last_state = 0;
        // but actually the LED is on; this is because
        // it is active low on the ESP-01)
      } else if(message.equals(String("ON"))){
        if(brightness>0&&last_brightness>0){
          count = abs(brightness-last_brightness);
          if(brightness-last_brightness>0&&!last_state){
              for(int i=0;i<=brightness;i++){
                Serial.println(i);
                analogWrite(light_high, i);
                delay(5);
              }
          }
          if(brightness-last_brightness>0){
            for(int i=last_brightness;i<=brightness;i++){
              Serial.println(i);
              analogWrite(light_high, i);
              delay(5);
            }
          }else{
            for(int i=last_brightness;i>=brightness;i--){
              Serial.println(i);
              analogWrite(light_high, i);
              delay(5);
            }
          }
          
        }else{
          count = brightness;
          for(int i=0;i<=count;i++){
            Serial.println(i);
            analogWrite(light_high, i);
            delay(5);
          }
        }
        last_state = 1;
      }
   }
   else if(String(topic).equals(brightness_topic)){
       last_brightness = brightness;
       brightness = message.toInt();
       brightness = 0.01*brightness*1024;
   }
  
  
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-HOME";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(client_id, mqtt_user, mqtt_password)) {
      Serial.println("connected");
      client.subscribe(light_command_topic);
      client.subscribe(brightness_topic);
      client.subscribe(rgb_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pinMode(light_high, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  //pinMode(1, OUTPUT);
  //pinMode(light_en_pin, OUTPUT);
  analogWrite(light_high, 0);
  //digitalWrite(1, LOW);
  //digitalWrite(light_en_pin, HIGH);
  Serial.begin(115200);
  setup_wifi();
  ticker.attach(2,HT);
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
}
