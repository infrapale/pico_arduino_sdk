//#include <TaHa.h> 
#include "BtnPinOnOff.h"

#define NBR_MENU_KEYS 3
#define TEST_PIN      17
#define NBR_OF_TASKS  8

//TaHa TaHa_10ms;
//TaHa TaHa_1000ms;

BtnPinOnOff  menu_btn[NBR_MENU_KEYS];

typedef void (*task_cb)(void);

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

typedef enum 
{
    TASK_TYPE_IDLE = 0,
    TASK_TYPE_INTERVAL,
    TASK_TYPE_RUN_ONCE
} task_type_et;


typedef struct 
{
    char     label[20]; 
    task_cb  cb;
    task_type_et type; 
    uint16_t  state;
    uint16_t  status;
    bool      is_ready;
    bool      is_active;
    uint32_t interval;
    uint32_t next_run;
    uint32_t interval_2;
} task_struct_st;

void run_10ms(void);
void run_1000ms(void);
void task_print_all(void);
void task_print_header(void);
void run_print_cnt(void);

task_struct_st task[TASK_NBR_OF] =
{ 
    //12345678901234567890
    {"Task 1000ms        ",run_print_cnt,   TASK_TYPE_INTERVAL, 0, 0, false, true,  5000,  0, 0},
    {"Print Tasks        ",task_print_all,  TASK_TYPE_INTERVAL, 0, 0, false, true, 15000,  0, 0},
    {"Task 10ms          ",run_10ms,        TASK_TYPE_INTERVAL, 0, 0, false, true,  10,  0, 0},
    {"Task 100ms         ",run_10ms,        TASK_TYPE_IDLE,     0, 0, false, false, 100,  0, 0},
    {"Task 100ms         ",run_10ms,        TASK_TYPE_IDLE,     0, 0, false, false, 100,  0, 0},
    {"Task 100ms         ",run_10ms,        TASK_TYPE_IDLE,     0, 0, false, false, 100,  0, 0},
    {"Task 100ms         ",run_10ms,        TASK_TYPE_IDLE,     0, 0, false, false, 100,  0, 0},
    {"Task 100ms         ",run_10ms,        TASK_TYPE_IDLE,     0, 0, false, false, 100,  0, 0},
};

void task_start( uint8_t indx, task_cb cb, uint32_t interval )
{
    if(indx < TASK_NBR_OF)
    {
        task[indx].type = TASK_TYPE_INTERVAL;
        task[indx].interval = interval;
        task[indx].cb = cb;
        task[indx].next_run = millis() + interval;
        task[indx].state = 0;
        task[indx].status = 0;
        task[indx].is_ready = false;
        task[indx].is_active = true;

    }
}

void task_stop( uint8_t indx )
{
    if(indx < TASK_NBR_OF)
    {
        task[indx].type = TASK_TYPE_IDLE;
        task[indx].is_ready = true;
        task[indx].is_active = false;
    }
}

void task_print_header(void)
{
    Serial1.println("Name  Type Interval State Status Ready Active\n");
}

void task_print(task_struct_st *task_ptr)
{  
    Serial1.printf("%s ",task_ptr->label);
    Serial1.printf("%2d ",task_ptr->type);
    Serial1.printf("%8d ",task_ptr->interval);
    Serial1.printf("%4d ",task_ptr->state);
    Serial1.printf("%2d ",task_ptr->status);
    Serial1.printf("%2d ",task_ptr->is_ready);
    Serial1.printf("%2d ",task_ptr->is_active);
    Serial1.printf("\n");
}

void task_print_all(void)
{
    task_print_header();
    for (uint8_t i = 0; i < TASK_NBR_OF;i++)
    {
        task_print(&task[i]);
    }
}


void task_schedule(void)
{
    static uint8_t indx = 0;
    
    uint32_t time_now = millis();
    if (task[indx].is_active)
    {
        if (time_now > task[indx].next_run )
        {
            switch(task[indx].type)
            {
                case TASK_TYPE_IDLE:
                    break;
                case TASK_TYPE_INTERVAL:
                    task[indx].next_run = time_now + task[indx].interval;
                    (*task[indx].cb)();
                    break;
                case TASK_TYPE_RUN_ONCE:
                    (*task[indx].cb)();
                    task[indx].is_active = false;
                    break;
            }
        }
    }
    indx++;
    if ( indx >= TASK_NBR_OF ) indx = 0;
    
}

void run_10ms()
{
    // Serial.println(F("run_10ms"));
    for( uint8_t i= 0; i < NBR_MENU_KEYS; i++)
    {
        //menu_btn[i].Scan();
    }
    //Serial.println(F("-"));
    //menu_btn[0].Scan();
}

void run_1000ms(void)
{
    if (digitalRead(TEST_PIN) == LOW )  Serial1.println(F("0"));
    else Serial1.println(F("1"));
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
    task_schedule();
   
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
