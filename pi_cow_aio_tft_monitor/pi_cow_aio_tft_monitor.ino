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
#include <TFT_eSPI.h> // Hardware-specific library
#include <SPI.h>
#include "Free_Fonts.h" // Include the header file attached to this sketch
#include <TaHa.h> 
#include "BtnPinOnOff.h"



#define TFT_GREY      0x5AEB
#define LOOP_PERIOD   35 // Display updates every 35 ms
#define NBR_SECTIONS  4
#define NBR_MENU_KEYS 3

TFT_eSPI tft = TFT_eSPI();       // Invoke custom library


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

typedef struct {
  int16_t x;
  int16_t y;
} xy_st;


typedef struct
{
    //char topic[64];
    char label[16];
    char unit[4];
    xy_st pos;
    Adafruit_MQTT_Subscribe *mqtt;
} mqtt_subs_feed_st;

typedef struct
{
    xy_st       pos;
    xy_st       dim;
    uint32_t    background_color;
    uint32_t    border_color;
    uint32_t    text_color;
    uint8_t     font_index;
    char        text[20];
} text_box_st;

Adafruit_MQTT_Subscribe *mqtt_subs[AIO_SUBS_FEEDS];

BtnPinOnOff  menu_btn[NBR_MENU_KEYS];

TaHa TaHa_10ms;

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
    {"khh    ", "C", {0, 40}, &astrid_od_temperature},
    {"tupa   ", "C", {0, 80}, &astrid_tupa_temperature},
    {"tupa   ", "%", {0, 120},  &astrid_tupa_humidity},
    {"studio ", "C", {0, 160},  &astrid_studio_temperature}
    
};


//TFT_WIDTH
//TFT_HEIGHT



#define  BOX_HEIGHT  (TFT_WIDTH / 5)
text_box_st text_box[4] =
{
    {{0, BOX_HEIGHT*0},{TFT_HEIGHT, BOX_HEIGHT}, TFT_BROWN,     TFT_BLACK,  TFT_GOLD,      3, "Box 1 "},
    {{0, BOX_HEIGHT*1},{TFT_HEIGHT, BOX_HEIGHT}, TFT_DARKGREY,  TFT_GREY,   TFT_WHITE,     2, "Box 2 "},
    {{0, BOX_HEIGHT*2},{TFT_HEIGHT, BOX_HEIGHT}, TFT_DARKCYAN,  TFT_NAVY,   TFT_RED,       3, "Box 3 "},
    {{0, BOX_HEIGHT*3},{TFT_HEIGHT, BOX_HEIGHT}, TFT_DARKGREEN, TFT_GOLD,   TFT_LIGHTGREY, 1, "Box 4 "},
};

#define MENU_YPOS         BOX_HEIGHT *4
#define MENU_BOX_WIDTH    TFT_HEIGHT /3
text_box_st menu_box[3] = 
{
    {{MENU_BOX_WIDTH * 0, MENU_YPOS},{MENU_BOX_WIDTH, BOX_HEIGHT}, TFT_BROWN,     TFT_BLACK,  TFT_GOLD,     0, "Menu 1 "},
    {{MENU_BOX_WIDTH * 1, MENU_YPOS},{MENU_BOX_WIDTH, BOX_HEIGHT}, TFT_DARKGREY,  TFT_GREY,   TFT_WHITE,    0, "Menu 2 "},
    {{MENU_BOX_WIDTH * 2, MENU_YPOS},{MENU_BOX_WIDTH, BOX_HEIGHT}, TFT_DARKCYAN,  TFT_NAVY,   TFT_RED,      0, "Menu 3 "},
};




control_st ctrl = 
{
    .temp = 20.0,
    .set_temp = 18,
    .heat_on = false
};
//float temp = 20.0; //to store the temperature value
uint32_t next_publ;


void select_font(uint8_t font_index)
{
    switch(font_index)
    {
        case 0:
            tft.setFreeFont(FSB9);   // Select Free Serif 9 point font, could use:
            break;
        case 1:
            tft.setFreeFont(FSB12);
            break;
        case 2:
            tft.setFreeFont(FSB18); 
            break;
        case 3:
            tft.setFreeFont(FSB24); 
            break;
    }
}

void draw_text_box(text_box_st *box_ptr)
{
    tft.fillRect(box_ptr->pos.x, box_ptr->pos.y, box_ptr->dim.x, box_ptr->dim.y, box_ptr->background_color);
    tft.drawRect(box_ptr->pos.x, box_ptr->pos.y, box_ptr->dim.x, box_ptr->dim.y, TFT_BLUE);
    select_font(box_ptr->font_index);
    tft.setTextColor(box_ptr->text_color);
    tft.setCursor(box_ptr->pos.x+2, box_ptr->pos.y+BOX_HEIGHT-8);  
    tft.print(box_ptr->text);

}

void run_10ms()
{
    // Serial.println(F("run_10ms"));
    for( uint8_t i= 0; i < NBR_MENU_KEYS; i++)
    {
        menu_btn[i].Scan();
    }
}

void setup() 
{
    delay(3000);
    tft.init();
    tft.setRotation(3);
    Serial.begin(9600); // For debug
    while (!Serial) 
    {
      ;  // wait for serial port to connect. Needed for native USB port only
    }

    // Menu buttons
    menu_btn[0].Init(22,'1');   //KEY3
    menu_btn[1].Init(27,'2');   //KEY3
    menu_btn[2].Init(28,'3');   //KEY3
    
    Serial.println("FTFT_Pico" );
    tft.fillScreen(TFT_BLACK);
    pinMode(TFT_BL, OUTPUT);  
  
    digitalWrite(TFT_BL, HIGH);

    //Serial.begin(115200);
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
    /*
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
    */
    TaHa_10ms.set_interval(500, RUN_RECURRING, run_10ms); 
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

        xy_st pos = {0,40};   
    // ping adafruit io a few times to make sure we remain connected
    //header("Using print() method", TFT_NAVY);

    // For comaptibility with Adafruit_GFX library the text background is not plotted when using the print class
    // even if we specify it.
    tft.setTextColor(TFT_YELLOW);
    tft.setCursor(pos.x, pos.y);    // Set cursor near top left corner of screen

    tft.setFreeFont(TT1);     // Select the orginal small TomThumb font
    tft.println();             // Move cursor down a line
    tft.print("The really tiny TomThumb font");    // Print the font name onto the TFT screen
    tft.println();
    tft.println();


    tft.setFreeFont(FSB9);   // Select Free Serif 9 point font, could use:
    // tft.setFreeFont(&FreeSerif9pt7b);
    tft.println();          // Free fonts plot with the baseline (imaginary line the letter A would sit on)
    // as the datum, so we must move the cursor down a line from the 0,0 position
    tft.print("Serif Bold 9pt");  // Print the font name onto the TFT screen

    tft.setFreeFont(FSB12);       // Select Free Serif 12 point font
    tft.println();                // Move cursor down a line
    tft.print("Serif Bold 12pt"); // Print the font name onto the TFT screen

    tft.setFreeFont(FSB18);       // Select Free Serif 12 point font
    tft.println();                // Move cursor down a line
    tft.print("Serif Bold 18pt"); // Print the font name onto the TFT screen

    tft.setFreeFont(FSB24);       // Select Free Serif 24 point font
    tft.println();                // Move cursor down a line
    tft.print("Serif Bold 24pt"); // Print the font name onto the TFT screen


    delay(4000);
    tft.fillScreen(TFT_BLACK);
    for(uint8_t i =0; i < 4; i++)
    {
        draw_text_box(&text_box[i]);
    }

    for(uint8_t i =0; i < 3; i++)
    {
        draw_text_box(&menu_box[i]);
    }

    while(0)
    {
      Watchdog.reset();
    }

    delay(2000);
    //tft.begin();
    //tft.setTextColor(TFT_RED);
}

void loop() 
{
    TaHa_10ms.run();

    /*
    if(! mqtt.ping(3)) 
    {
        if(! mqtt.connected())
            connect();
    }
    */
    Watchdog.reset();

    for( uint8_t i= 0; i < NBR_MENU_KEYS; i++)
    {
        char c = menu_btn[i].Read();
        if (c != 0x00) 
        {
            if ((c & 0b10000000) == 0) 
                Serial.print("On ");
            else 
                Serial.print("Off ");
            Serial.println(c & 0b01111111);

        }
    }
    if (millis() > next_publ)
    {
        next_publ = millis() + AIO_PUBLISH_INTERVAL_ms;

    }
    /*
    Adafruit_MQTT_Subscribe *subscription;
    while ((subscription = mqtt.readSubscription(5000))) 
    {
        for (uint8_t indx=0; indx < AIO_SUBS_FEEDS; indx++)
        {
            if (subscription ==  subs[indx].mqtt)
            {
                Serial.print(subs[indx].mqtt->topic);
                Serial.println((char *) subs[indx].mqtt->lastread); 
                //sprintf(str, "Value of Pi = %f", M_PI);
                sprintf(text_box[indx].text, "%s %s %s", subs[indx].label, (char *) subs[indx].mqtt->lastread, subs[indx].unit );
                draw_text_box(&text_box[indx]);
                //tft.setCursor(subs[indx].pos.x, subs[indx].pos.y);
                //tft.setFreeFont(FSB18);  
                //tft.print(subs[indx].label);
                //tft.print((char *) subs[indx].mqtt->lastread);


            }
        }
          
    }
    */
    //Watchdog.reset();
}