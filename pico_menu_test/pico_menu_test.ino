#include <TaHa.h> 
#include "BtnPinOnOff.h"

#define NBR_MENU_KEYS 3

TaHa TaHa_10ms;

BtnPinOnOff  menu_btn[NBR_MENU_KEYS];

void run_10ms()
{
    // Serial.println(F("run_10ms"));
    for( uint8_t i= 0; i < NBR_MENU_KEYS; i++)
    {
        menu_btn[i].Scan();
    }
}

void setup() {
    // put your setup code here, to run once:
    delay(3000);
    Serial.begin(9600); // For debug
    while (!Serial) 
    {
      ;  // wait for serial port to connect. Needed for native USB port only
    }

    TaHa_10ms.set_interval(500, RUN_RECURRING, run_10ms); 
    Serial.println(F("Setup done"));
}

void loop() {
    // put your main code here, to run repeatedly:
    TaHa_10ms.run();

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
}
