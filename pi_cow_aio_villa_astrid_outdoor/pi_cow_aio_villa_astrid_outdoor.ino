/**
 * pi_cow_aio_villa_astrid_tupa

 * Pico W Adafruit IO MQTT subcribe and publish
 * https://learn.adafruit.com/mqtt-adafruit-io-and-you/intro-to-adafruit-mqtt
 * https://github.com/infrapale/pico_arduino_sdk.git
 *
 */
//#define PIRPANA
//#define LILLA_ASTRID
#define VILLA_ASTRID
#include <stdint.h>
#include "stdio.h"
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include <WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "secrets.h"
//#include <Adafruit_PCT2075.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"
#include "Adafruit_VEML7700.h"

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

#define SEALEVELPRESSURE_HPA (1013.25)

#define PIN_SERIAL1_TX  (0u)
#define PIN_SERIAL1_RX  (1u)
#define PIN_I2C_PWR_EN  (15u)

typedef struct
{
  uint16_t  mqtt_fault;
  uint16_t  bme_fault;
  uint16_t  lux_fault;  
} fault_cntr_st;

typedef struct 
{
    uint8_t     publ_indx;
    float       temp;
    float       humidity;
    float       lux;
    float       ldr1;
    float       ldr2;
    uint16_t    set_temp;
    bool        heat_on;
    fault_cntr_st   fault_cntr;
} control_st;

WiFiClient client;

//Adafruit_PCT2075 PCT2075;
Adafruit_BME680 bme; // I2C
Adafruit_VEML7700 veml = Adafruit_VEML7700();


// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
// infrapale/feeds/lillaastrid.studio-temp
// infrapale/feeds/lillaastrid.studio-set-tmp
// infrapale/feeds/villaastrid.khh-temperature
// infrapale/feeds/villaastrid.khh-set-temperature
// infrapale/feeds/villaastrid.tupa-temp
// infrapale/feeds/villaastrid.tupa-hum
// infrapale/feeds/villaastrid.ulko-temp
// infrapale/feeds/villaastrid.ulko-hum
// infrapale/feeds/villaastrid.ulko-ldr-1
// infrapale/feeds/villaastrid.ulko-ldr-2

Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
Adafruit_MQTT_Publish sensor_temperature = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/villaastrid.ulko-temp");
Adafruit_MQTT_Publish sensor_humidity = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/villaastrid.ulko-hum");
Adafruit_MQTT_Publish sensor_lux = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/villaastrid.ulko-lux");
Adafruit_MQTT_Publish sensor_ldr1 = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/villaastrid.ulko-ldr-1");
Adafruit_MQTT_Publish sensor_ldr2 = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/villaastrid.ulko-ldr-2");

// Adafruit_MQTT_Subscribe set_temperature = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/villaastrid.khh-set-temperature");

control_st ctrl = 
{
    .publ_indx = 0,
    .temp = 20.0,
    .humidity = 50.0,
    .set_temp = 18,
    .heat_on = false,
    .fault_cntr =
    {
        .mqtt_fault = 0,
        .bme_fault   = 0,
        .lux_fault   = 0        
    }
};
//float temp = 20.0; //to store the temperature value
uint32_t next_publ;

void setup() 
{
    //Serial.begin(115200);
    Serial.begin(9600);
    Serial1.begin(9600);
    delay(4000);
    pinMode(PIN_I2C_PWR_EN, OUTPUT);
    digitalWrite(PIN_I2C_PWR_EN,LOW);
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
    bme = Adafruit_BME680(&Wire1); 
   

    if (!bme.begin(0x77)) {
        Serial.println("Could not find a valid BME680 sensor, check wiring!");
      while (1);
    }

    /*
    if (!veml.begin(&Wire1)) 
    {
        Serial.println("Sensor not found");
        while (1);
    }
  

  Serial.println("Sensor found");
  */
    // Set up oversampling and filter initialization
    bme.setTemperatureOversampling(BME680_OS_8X);
    bme.setHumidityOversampling(BME680_OS_2X);
    bme.setPressureOversampling(BME680_OS_4X);
    bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
    bme.setGasHeater(320, 150); // 320*C for 150 ms

    uint32_t count_down_ms = Watchdog.enable(60000);
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
    //mqtt.subscribe(&set_temperature);
    connect();
    next_publ = millis() + 5000;
    ctrl.publ_indx = 0;
    while(0)
    {
        //Serial1.println("<OL1:2345.1>");
        //Serial.println("<OL1:2345.1>");
        int ldr1 = analogRead(A0);
        Serial.print("LDR1 = "); Serial.println(ldr1);
        int ldr2 = analogRead(A1);
        Serial.print("LDR2 = "); Serial.println(ldr2);
        delay(1000);
        Watchdog.reset();
    }
    
    
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



void report_publ_status(bool publ_status)
{
    if (publ_status)
    {                     
        Serial.println(F(" - Sent!"));
    }
    else 
    {
        Serial.println(F(" - Failed"));
    }
}

/**
  * Syntax example: <TEMP:-10.1>\n
  *   < = start char
  *   XXXX = ID 4 char
  *   : = separator
  *   signed float value 
  *   > = end char
  *   \n  = newline (makes testing easier)
 */
void send_meas_to_uart(const char *id_4, float value)
{
    char buff[40];
    sprintf(buff, "<%s:%f>\n",id_4,value);
    Serial.print(buff);
    /*
    Serial.print('<');
    Serial.print(id_4);
    Serial.print(':');
    Serial.print(value);
    Serial.println('>');
    */
}
void loop() 
{
    bool publ_status;

    // ping adafruit io a few times to make sure we remain connected
    if(! mqtt.ping(3)) 
    {
        if(! mqtt.connected()) connect();
    }
    if(!mqtt.connected())  
    {
        ctrl.fault_cntr.mqtt_fault++;      
    }
    else
    {
        ctrl.fault_cntr.mqtt_fault = 0;      

        if (millis() > next_publ)
        {
            next_publ = millis() + AIO_PUBLISH_INTERVAL_ms;
            switch(ctrl.publ_indx)
            {
                case 0:
                    if ( bme.performReading()) 
                    {
                        ctrl.temp = bme.temperature;
                        //Serial.print("temperature = ");
                        //Serial.print(ctrl.temp);
                        send_meas_to_uart("TEMP",ctrl.temp);
                        publ_status = sensor_temperature.publish(ctrl.temp); //Publish to Adafruit
                        report_publ_status(publ_status);
                        ctrl.fault_cntr.bme_fault = 0;
                    }
                    else
                    {
                        ctrl.fault_cntr.bme_fault++;
                        Serial.println("Error when reading BME680");                            
                    }
                    ctrl.publ_indx++;
                    break;
                case 1:
                    if ( bme.performReading()) 
                    {
                        ctrl.humidity = bme.humidity;
                        send_meas_to_uart("HUMI",ctrl.humidity);
                        publ_status = sensor_humidity.publish(ctrl.humidity); //Publish to Adafruit
                        report_publ_status(publ_status);
                        ctrl.fault_cntr.bme_fault = 0;
                    }
                    else
                    {                                                        
                        Serial.println("Error when reading BME680");
                    }
                    ctrl.publ_indx += 2;
                    break;
                case 2:
                    ctrl.lux = veml.readLux(VEML_LUX_AUTO);
                    if (ctrl.lux > 0) 
                    {
                        ctrl.fault_cntr.lux_fault=0;
                        send_meas_to_uart("LUX1",ctrl.lux);
                        Serial.println("------------------------------------");
                        Serial.print("Lux = "); Serial.println(ctrl.lux);
                        Serial.println("Settings used for reading:");
                        Serial.print(F("Gain: "));
                        switch (veml.getGain()) {
                            case VEML7700_GAIN_1: Serial.println("1"); break;
                            case VEML7700_GAIN_2: Serial.println("2"); break;
                            case VEML7700_GAIN_1_4: Serial.println("1/4"); break;
                            case VEML7700_GAIN_1_8: Serial.println("1/8"); break;
                        }
                        Serial.print(F("Integration Time (ms): "));
                        switch (veml.getIntegrationTime()) {
                            case VEML7700_IT_25MS: Serial.println("25"); break;
                            case VEML7700_IT_50MS: Serial.println("50"); break;
                            case VEML7700_IT_100MS: Serial.println("100"); break;
                            case VEML7700_IT_200MS: Serial.println("200"); break;
                            case VEML7700_IT_400MS: Serial.println("400"); break;
                            case VEML7700_IT_800MS: Serial.println("800"); break;
                        }
                        publ_status = sensor_lux.publish(ctrl.lux); //Publish to Adafruit
                        report_publ_status(publ_status);
                    }
                    else
                    {
                        ctrl.fault_cntr.lux_fault++;
                        Serial.println("Lux reading failed");
                    }    
                    ctrl.publ_indx++;
                    break;  
                case 3:     
                    ctrl.ldr1 = (float) analogRead(A0);
                    publ_status = sensor_ldr1.publish(ctrl.ldr1); 
                    report_publ_status(publ_status);                    
                    ctrl.publ_indx++;
                    break;  
                case 4:     
                    ctrl.ldr2 = (float) analogRead(A1);
                    publ_status = sensor_ldr2.publish(ctrl.ldr2); 
                    report_publ_status(publ_status);                    
                    ctrl.publ_indx = 0;
                    break;  
                default:
                    ctrl.publ_indx = 0;
                    break;
            }
        }
        if ((ctrl.fault_cntr.mqtt_fault < 2) &&
            (ctrl.fault_cntr.bme_fault < 2) &&
            (ctrl.fault_cntr.lux_fault < 4))
        {
            Watchdog.reset();
        }
        else
        {
            Serial.println("A Watchdog reset will occur");             
        }                               
        
    }

    /*
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
    */
    
  
}