/**
 * pi_cow_aio_test1
 * Pico W Adafruit IO MQTT subcribe and publish
 * https://learn.adafruit.com/mqtt-adafruit-io-and-you/intro-to-adafruit-mqtt
 * https://github.com/infrapale/pico_arduino_sdk.git
 *
 */
#define PIRPANA

#include <WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "secrets.h"
#include <Adafruit_PCT2075.h>
#include "Wire.h"

// WiFi parameters
#define WLAN_SSID       WIFI_SSID
#define WLAN_PASS       WIFI_PASS

// Adafruit IO
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    IO_USERNAME
#define AIO_KEY         IO_KEY
#define AIO_PUBLISH_INTERVAL_ms  60000 
WiFiClient client;

Adafruit_PCT2075 PCT2075;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
Adafruit_MQTT_Publish Temperature = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/home-tampere.sisa-temperature");
Adafruit_MQTT_Subscribe studio_lampotila = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/home-tampere.sisa-switch");

float temp = 20.0; //to store the temperature value
uint32_t next_publ;

void setup() 
{
    //Serial.begin(115200);
    Serial.begin(9600);
    while (!Serial) {
      ;  // wait for serial port to connect. Needed for native USB port only
    }
    Serial.println(F("Adafruit IO Example"));
    // Connect to WiFi access point.
    Serial.print(F("Connecting to "));
    Serial.println(WLAN_SSID);
    Wire1.setSCL(7);
    Wire1.setSDA(6);
    Wire1.begin();
    PCT2075 = Adafruit_PCT2075();
    if (!PCT2075.begin(0x37,&Wire1)) 
    {
        Serial.println("Couldn't find PCT2075 chip");
        while (1);
    }

    WiFi.begin(WLAN_SSID, WLAN_PASS);
    while (WiFi.status() != WL_CONNECTED) 
    {
        delay(500);
        Serial.print(F("."));
    }

    Serial.println();
    Serial.println(F("WiFi connected"));
    Serial.println(F("IP address: "));
    Serial.println(WiFi.localIP());

    // connect to adafruit io
    mqtt.subscribe(&studio_lampotila);
    connect();
    next_publ = millis() + 5000;
}

// connect to adafruit io via MQTT
void connect() {
    Serial.print(F("Connecting to Adafruit IO… "));
    int8_t ret;
    while ((ret = mqtt.connect()) != 0) 
    {
        switch (ret) 
        {
            case 1: Serial.println(F("Wrong protocol")); break;
            case 2: Serial.println(F("ID rejected")); break;
            case 3: Serial.println(F("Server unavail")); break;
            case 4: Serial.println(F("Bad user/pass")); break;
            case 5: Serial.println(F("Not authed")); break;
            case 6: Serial.println(F("Failed to subscribe")); break;
            default: Serial.println(F("Connection failed")); break;
        }

        if(ret >= 0) mqtt.disconnect();
        Serial.println(F("Retrying connection…"));
        delay(10000);
    }
    Serial.println(F("Adafruit IO Connected!"));
}

void loop() 
{
    // ping adafruit io a few times to make sure we remain connected
    if(! mqtt.ping(3)) 
    {
        if(! mqtt.connected())
            connect();
    }
    if (millis() > next_publ)
    {
        next_publ = millis() + AIO_PUBLISH_INTERVAL_ms;
        temp = PCT2075.getTemperature();

        Serial.print("temperature = ");
        Serial.print(temp);

        if (! Temperature.publish(temp))  //Publish to Adafruit
        {                     
            Serial.println(F(" - Failed"));
        }
        else 
        {
            Serial.println(F(" - Sent!"));
        }

    }
    Adafruit_MQTT_Subscribe *subscription;
    while ((subscription = mqtt.readSubscription(5000))) 
    {
        if (subscription == &studio_lampotila) 
        {
            Serial.print(F("Set temperature: "));
            Serial.println((char *)studio_lampotila.lastread);
        }
    }
  
}