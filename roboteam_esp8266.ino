#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "SPISlave.h"

#define DATA_READY 5

char ssid[32];
char password[32];
char mqtt_server[32];
char publish_data[32];
volatile bool new_publish_data = false;

uint32_t robot_id;

volatile int wifi_status = 0;

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  pinMode(DATA_READY, OUTPUT);
  digitalWrite(DATA_READY, LOW);
  
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  // data has been received from the master. Beware that len is always 32
  // and the buffer is autofilled with zeroes if data is less than 32 bytes long
  // It's up to the user to implement protocol for handling data length
  SPISlave.onData([](uint8_t * data, size_t len) {
    switch(wifi_status) {
      case 0:
        memcpy(ssid, data, len);
        wifi_status++;
        break;
      case 1:
        memcpy(password, data, len);
        wifi_status++;
        break;
      case 2:
        memcpy(mqtt_server, data, len);
        wifi_status++;
        break;
      case 3:
        memcpy(publish_data, data, len);
        new_publish_data = true;
        //if (client.connected()) {
          //client.publish("robot/feedback", data, len);
          //Serial.println("<");
        //}
        break;
    }
  });

  // The master has read out outgoing data buffer
  // that buffer can be set with SPISlave.setData
  SPISlave.onDataSent([]() {
    digitalWrite(DATA_READY, LOW);
    Serial.println("A");
  });

  // status has been received from the master.
  // The status register is a special register that bot the slave and the master can write to and read from.
  // Can be used to exchange small data or status information
  SPISlave.onStatus([](uint32_t data) {
    Serial.printf("Status: %u\n", data);
    robot_id = data;
    wifi_status = 0;
    //SPISlave.setStatus(millis()); //set next status
  });

  // The master has read the status register
  SPISlave.onStatusSent([]() {
    Serial.println("S");
  });

  // Setup SPI Slave registers and pins
  SPISlave.begin();

  // Set the status register (if the master reads it, it will read this value)
  //SPISlave.setStatus(robot_id);

  // Sets the data registers. Limited to 32 bytes at a time.
  // SPISlave.setData(uint8_t * data, size_t len); is also available with the same limitation
  //SPISlave.setData("Ask me a question!");

  // wait for wifi settings
  while(wifi_status < 3) {
    yield();
  }
  
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  Serial.println(password);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int len) {
    SPISlave.setData(payload, len);
    digitalWrite(DATA_READY, HIGH);
    Serial.println(">");
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe("robot/commands");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void loop() {
  if (wifi_status = 3) {
    if (!client.connected()) {
      reconnect();
    } else {
      if (new_publish_data) {
        client.publish("robot/feedback", publish_data, 32);
        new_publish_data = false;
        Serial.println("<");
      }
      client.loop();
    }
  }
}
