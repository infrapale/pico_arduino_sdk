/**
 * pi_cow_aio_test1
 * Pico W Adafruit IO MQTT subcribe and publish
 * https://learn.adafruit.com/mqtt-adafruit-io-and-you/intro-to-adafruit-mqtt
 * https://github.com/infrapale/pico_arduino_sdk.git
 *
 */
//#define PIRPANA
#define LILLA_ASTRID
//#define VILLA_ASTRID
#include <stdint.h>
#include <WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "secrets.h"
#include <Adafruit_PCT2075.h>
#include "Wire.h"
#include <Adafruit_SleepyDog.h>

// WiFi parameters
#define WLAN_SSID       WIFI_SSID
#define WLAN_PASS       WIFI_PASS

// Adafruit IO
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    IO_USERNAME
#define AIO_KEY         IO_KEY
#define AIO_PUBLISH_INTERVAL_ms  60000*5

typedef struct 
{
    float       temp;
    uint16_t    set_temp;
    bool        heat_on;
} control_st;

WiFiClient client;

Adafruit_PCT2075 PCT2075;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
// infrapale/feeds/lillaastrid.studio-temp
// infrapale/feeds/lillaastrid.studio-set-tmp
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
Adafruit_MQTT_Publish sensor_temperature = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/lillaastrid.studio-temp");
Adafruit_MQTT_Subscribe set_temperature = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/lillaastrid.studio-set-tmp");

control_st ctrl = 
{
    .temp = 20.0,
    .set_temp = 18,
    .heat_on = false
};
//float temp = 20.0; //to store the temperature value
uint32_t next_publ;

void setup() 
{
    //Serial.begin(115200);
    Serial.begin(9600);
    Serial1.setRX(2);
    Serial1.setTX(1);
    Serial1.begin(9600);
    delay(4000);
    //while (!Serial) {
    //  ;  // wait for serial port to connect. Needed for native USB port only
    //}
    Serial.println(F("Adafruit IO Example"));
    // Connect to WiFi access point.
    Serial.print(F("Connecting to "));
    Serial.println(WLAN_SSID);
    Wire1.setSCL(7);
    Wire1.setSDA(6);
    Wire1.begin();
    PCT2075 = Adafruit_PCT2075();
    uint32_t count_down_ms = Watchdog.enable(10000);
    if (!PCT2075.begin(0x37,&Wire1)) 
    {
        Serial.println("Couldn't find PCT2075 chip");
        while (1);
    }
    Watchdog.reset();
    WiFi.begin(WLAN_SSID, WLAN_PASS);
    while (WiFi.status() != WL_CONNECTED) 
    {
        delay(500);
        Serial.print(F("."));
    }
    Watchdog.reset();

    Serial.println();
    Serial.println(F("WiFi connected"));
    Serial.println(F("IP address: "));
    Serial.println(WiFi.localIP());

    // connect to adafruit io
    mqtt.subscribe(&set_temperature);
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
    Serial1.println("test");
    // ping adafruit io a few times to make sure we remain connected
    if(! mqtt.ping(3)) 
    {
        if(! mqtt.connected())
            connect();
    }
    Watchdog.reset();

    if (millis() > next_publ)
    {
        next_publ = millis() + AIO_PUBLISH_INTERVAL_ms;
        ctrl.temp = PCT2075.getTemperature();

        Serial.print("temperature = ");
        Serial.print(ctrl.temp);

        if (! sensor_temperature.publish(ctrl.temp))  //Publish to Adafruit
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
        if (subscription == &set_temperature) 
        {
            Serial.print(F("Set temperature: "));
            Serial.println((char *)set_temperature.lastread);
            ctrl.set_temp = atoi((char *)set_temperature.lastread);
            Serial.println(ctrl.set_temp);
        }
    }
    Watchdog.reset();

  
}