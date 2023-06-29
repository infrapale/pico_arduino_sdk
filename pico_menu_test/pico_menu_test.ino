//#include <TaHa.h> 
#include "BtnPinOnOff.h"
#include "tasx.h"

#define NBR_MENU_KEYS 3
#define PIN_TEST      13
#define PIN_LDSW      15
#define NBR_OF_TASKS  8


BtnPinOnOff  menu_btn[NBR_MENU_KEYS];



typedef enum 
{
    TASK_0 = 0,
    TASK_1,
    TASK_2,
    TASK_3,
    TASK_4,
    TASK_5,
    TASK_6,
    TASK_7,
    TASK_NBR_OF
} task_et;


void run_10ms(void);
void run_1000ms(void);
void run_print_cnt(void);
void print_all_tasks(void);

task_struct_st task[TASK_NBR_OF] =
{ 
    //12345678901234567890
    {"Task 1000ms        ",run_1000ms,      TASK_TYPE_INTERVAL, 0, 0, false, true,  100,  0, 0},
    {"Print Tasks        ",print_all_tasks, TASK_TYPE_INTERVAL, 0, 0, false, true,  15000,  0, 0},
    {"Task 10ms          ",run_10ms,        TASK_TYPE_INTERVAL, 0, 0, false, true,  10,  0, 0},
    {"Task 100ms         ",run_10ms,        TASK_TYPE_IDLE,     0, 0, false, false, 100,  0, 0},
    {"Task 100ms         ",run_10ms,        TASK_TYPE_IDLE,     0, 0, false, false, 100,  0, 0},
    {"Task 100ms         ",run_10ms,        TASK_TYPE_IDLE,     0, 0, false, false, 100,  0, 0},
    {"Task 100ms         ",run_10ms,        TASK_TYPE_IDLE,     0, 0, false, false, 100,  0, 0},
    {"Task 100ms         ",run_10ms,        TASK_TYPE_IDLE,     0, 0, false, false, 100,  0, 0},
};


void print_all_tasks(void)
{
    task_print_all(&task[0], TASK_NBR_OF);
}


void run_10ms()
{
    // Serial.println(F("run_10ms"));
    for( uint8_t i= 0; i < NBR_MENU_KEYS; i++)
    {
        menu_btn[i].Scan();
    }
    //Serial.println(F("-"));
    //menu_btn[0].Scan();
}

void run_1000ms(void)
{
    if (digitalRead(PIN_TEST) == LOW )  
    {
        Serial1.println(F("0"));
        digitalWrite(PIN_LDSW, LOW);
    }    
    else
    {
        Serial1.println(F("1"));
        digitalWrite(PIN_LDSW, HIGH);
    } 
} 

void run_print_cnt(void)
{
    static uint16_t cnt;

    Serial1.println(cnt++);

}

void setup() {
    // put your setup code here, to run once:
    delay(3000);
    Serial1.begin(9600); // For debug
    pinMode(14, INPUT_PULLUP);
    pinMode(PIN_TEST, INPUT_PULLUP);
    pinMode(PIN_LDSW,OUTPUT);
    digitalWrite(PIN_LDSW, HIGH);
    //pinMode(17, INPUT_PULLUP);
    //pinMode(32, INPUT_PULLUP);
    //pinMode(34, INPUT_PULLUP);

    
    menu_btn[0].Init(14,'A');
    //menu_btn[1].Init(22,'B');
    //menu_btn[2].Init(28,'C');
    //menu_btn[3].Init(34,'D');
    

    while (!Serial1) 
    {
      ;  // wait for serial port to connect. Needed for native USB port only
    }

    Serial1.println(F("Setup done"));
}

void loop() 
{
    //if (digitalRead(TEST_PIN) == LOW )  Serial.println(F("0"));
    //else Serial.println(F("1"));
    task_schedule(task,TASK_NBR_OF);
   
    for( uint8_t i= 0; i < NBR_MENU_KEYS; i++)
    {
        char c = menu_btn[i].Read();
        if (c != 0x00) 
        {
            if ((c & 0b10000000) == 0) 
                Serial1.printf("On");
            else 
                Serial1.printf("Off");
            Serial1.printf("%c\n",c & 0b01111111);

        }
    }
  
}
