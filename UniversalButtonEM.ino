/*
 Universal Button: Publish to an MQTT topic whenever a button is pressed
*/

/* Network config */
#define ENABLE_DHCP                 true   // true/false
#define ENABLE_MAC_ADDRESS_ROM      true   // true/false
#define MAC_I2C_ADDRESS             0x50   // Microchip 24AA125E48 I2C ROM address
static uint8_t mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };  // Set if no MAC ROM
static uint8_t ip[]  = { 192, 168, 1, 35 }; // Use if DHCP disabled

/* Required for networking */
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>

/* Required for MAC address ROM */
#include <Wire.h>

// Update with a value suitable for your MQTT broker
IPAddress server(192,168,1,111);

long last_message_time        = 0;
byte last_button_state        = 0;
long minimum_message_interval = 5000;
byte button_pin               = 7;

/**
 * Callback to process data from MQTT broker
 */
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

// Instantiate Ethernet client and MQTT client
EthernetClient ethClient;
PubSubClient   client(ethClient);

/**
 * Repeatedly attempt connection to MQTT broker
 */
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("arduinoClient")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      //client.publish("outTopic","hello world");
      // ... and resubscribe
      //client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


/**
 * Setup
 */
void setup()
{
  Serial.begin(9600);
  pinMode(button_pin, INPUT_PULLUP); // The input will be pulled LOW to trigger an action

  client.setServer(server, 1883);
  client.setCallback(callback);

  // setup the Ethernet library to talk to the Wiznet board
  if( ENABLE_DHCP == true )
  {
    Ethernet.begin(mac);      // Use DHCP
  } else {
    Ethernet.begin(mac, ip);  // Use static address defined above
  }
  // Allow the hardware to sort itself out
  delay(1500);
}


/**
 * Main loop
 */
void loop()
{
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  /* Read the state of a connected button and act if it has been pressed */
  if(digitalRead(button_pin) == LOW)
  {
    if(last_button_state == HIGH)
    {
      // HIGH to LOW transition: we just detected a button press!
      if(millis() - last_message_time > minimum_message_interval)  // Allows debounce / rate limiting
      {
        // Send a message
        client.publish("/universalbutton/1","hello world");
        last_message_time = millis();
      }
    }
    last_button_state = LOW;
  } else {
    last_button_state = HIGH;
  }
}


/**
 * Reads from I2C registers (used for the MAC address ROM)
 */
byte readRegister(byte r)
{
  unsigned char v;
  Wire.beginTransmission(MAC_I2C_ADDRESS);
  Wire.write(r);  // Register to read
  Wire.endTransmission();

  Wire.requestFrom(MAC_I2C_ADDRESS, 1); // Read a byte
  while(!Wire.available())
  {
    // Wait
  }
  v = Wire.read();
  return v;
}
