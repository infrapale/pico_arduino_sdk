#include "stdint.h"
#include "stdbool.h"
#include "tasx.h"
#include "time.h"


void task_start( task_struct_st *task_ptr, task_cb cb, uint32_t interval )
{
    task_ptr->type = TASK_TYPE_INTERVAL;
    task_ptr->interval = interval;
    task_ptr->cb = cb;
    task_ptr->next_run = millis() + interval;
    task_ptr->state = 0;
    task_ptr->status = 0;
    task_ptr->is_ready = false;
    task_ptr->is_active = true;

}

void task_stop( task_struct_st *task_ptr )
{
    task_ptr->type = TASK_TYPE_IDLE;
    task_ptr->is_ready = true;
    task_ptr->is_active = false;
}

void task_schedule( task_struct_st *task_ptr, uint8_t nbr_task)
{
    static uint8_t indx = 0;
    
    uint32_t time_now = millis();
    if (task_ptr[indx].is_active)
    {
        if (time_now > task_ptr[indx].next_run )
        {
            switch(task_ptr[indx].type)
            {
                case TASK_TYPE_IDLE:
                    break;
                case TASK_TYPE_INTERVAL:
                    task_ptr[indx].next_run = time_now + task_ptr[indx].interval;
                    (*task_ptr[indx].cb)();
                    break;
                case TASK_TYPE_RUN_ONCE:
                    (*task_ptr[indx].cb)();
                    task_ptr[indx].is_active = false;
                    break;
            }
        }
    }
    indx++;
    if ( indx >= nbr_task ) indx = 0;
    
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

void task_print_all( task_struct_st *task_ptr, uint8_t nbr_tasks)
{
    task_print_header();
    for (uint8_t i = 0; i < nbr_tasks;i++)
    {
        task_print(&task_ptr[i]);
    }
}

