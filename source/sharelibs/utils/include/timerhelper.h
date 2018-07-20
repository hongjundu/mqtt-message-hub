#pragma once

typedef int *do_job_fun(int);

typedef struct 
{
    char *name;
    int interval;
    do_job_fun *doJob;
} schedule_job;

/*
 update timer interval
*/
void timer_helper_update_interval_from_config(int baseInterval, schedule_job *jobList, char *iniFile);

/*
 setup timer
*/
int timer_helper_setup_timer(int baseInterval, void (*notify_function) (int));

/*
 free timer
*/
int timer_helper_destroy_timer();