#ifndef __TASX_H__
#define __TASX_H__

typedef void (*task_cb)(void);

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



void task_start( task_struct_st *task_ptr, task_cb cb, uint32_t interval );

void task_stop( task_struct_st *task_ptr);

void task_print_header(void);

void task_print(task_struct_st *task_ptr);

void task_print_all(task_struct_st *task_ptr,uint8_t nbr_task);

#endif