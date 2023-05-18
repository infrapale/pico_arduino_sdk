/**
 * pi_cow_aio_test1
 * Pico W Adafruit IO MQTT subcribe and publish
 * https://learn.adafruit.com/mqtt-adafruit-io-and-you/intro-to-adafruit-mqtt
 * https://github.com/infrapale/pico_arduino_sdk.git
 *
 */
#define PIRPANA
//#define LILLA_ASTRID
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
#define AIO_SUBS_FEEDS  4

typedef enum
{
    SUBS_KHH_TEMP = 0,
    SUBS_TUPA_TEMP,
    SUBS_TUPA_HUM,
    SUBS_STUDIO_TEMP
} aio_feeds_et;

typedef struct 
{
    float       temp;
    uint16_t    set_temp;
    bool        heat_on;
} control_st;

typedef struct
{
    //char topic[64];
    char label[16];
    char unit[4]; 
    Adafruit_MQTT_Subscribe *mqtt;
} mqtt_subs_feed_st;


Adafruit_MQTT_Subscribe *mqtt_subs[AIO_SUBS_FEEDS];

WiFiClient client;

Adafruit_PCT2075 PCT2075;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
// infrapale/feeds/lillaastrid.studio-temp
// infrapale/feeds/lillaastrid.studio-set-tmp
// infrapale/feeds/villaastrid.villa-astrid-khh-temperature
// infrapale/feeds/villaastrid.tupa-temp
// infrapale/feeds/villaastrid.tupa-hum
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
Adafruit_MQTT_Subscribe astrid_od_temperature   = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/villaastrid.villa-astrid-khh-temperature");
Adafruit_MQTT_Subscribe astrid_tupa_temperature = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/villaastrid.tupa-temp");
Adafruit_MQTT_Subscribe astrid_tupa_humidity    = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/feeds/villaastrid.tupa-hum");
Adafruit_MQTT_Subscribe astrid_parvi_temperature = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/villaastrid.parvi-temp");
Adafruit_MQTT_Subscribe astrid_studio_temperature = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/lillaastrid.studio-tmp");

mqtt_subs_feed_st subs[AIO_SUBS_FEEDS] =
{
    // [SUBS_KHH_TEMP] = {.topic = "/feeds/villaastrid.villa-astrid-khh-temperature" }
    {"khh temp",    "C",  &astrid_od_temperature},
    {"tupa temp",   "C",  &astrid_tupa_temperature},
    {"tupa hum",    "%",  &astrid_tupa_humidity},
    {"studio temp", "C",  &astrid_studio_temperature}
    
};



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
    while (!Serial) {
      ;  // wait for serial port to connect. Needed for native USB port only
    }
    Serial.println(F("Adafruit IO Example"));
    // Connect to WiFi access point.
    Serial.print(F("Connecting to "));
    Serial.print(WLAN_SSID);
    Serial.print(F(" pass: "));
    Serial.println(WLAN_PASS);
    Wire1.setSCL(7);
    Wire1.setSDA(6);
    // Wire1.begin();

    // Watchdog.reset();
    Serial.print(F(" WiFi.begin "));
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
    for (uint8_t i = 0; i < AIO_SUBS_FEEDS; i++ )
    {
        mqtt.subscribe(subs[i].mqtt);
    }
    //mqtt.subscribe(&astrid_od_temperature);
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
    Watchdog.reset();

    if (millis() > next_publ)
    {
        next_publ = millis() + AIO_PUBLISH_INTERVAL_ms;

    }
    Adafruit_MQTT_Subscribe *subscription;
    while ((subscription = mqtt.readSubscription(5000))) 
    {
        for (uint8_t indx=0; indx < AIO_SUBS_FEEDS; indx++)
        {
            if (subscription ==  subs[indx].mqtt)
            {
                Serial.print(subs[indx].mqtt->topic);
                Serial.println((char *) subs[indx].mqtt->lastread); 
            }
        }
          
    }
    //Watchdog.reset();
}