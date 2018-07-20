#include "timerhelper.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <memory.h>
#include <time.h>
#include <signal.h>
#include "stringutils.h"
#include "ini_parser.h"
#include "ini_parser_helper.h"

#define CLOCKID CLOCK_REALTIME
static timer_t g_scheduleTimerId  = 0;

/*
 setup timer
*/
int timer_helper_setup_timer(int baseInterval, void (*notify_function)(int))
{
    printf("timer_helper_setup_timer\n");

    struct sigevent evp;
    struct itimerspec its;
    int rc = 1;

    memset((void *)&evp, 0, sizeof(evp));

    evp.sigev_notify = SIGEV_THREAD;
    evp.sigev_notify_function = notify_function;
    evp.sigev_signo = SIGALRM;
    evp.sigev_value.sival_ptr = &g_scheduleTimerId;
    rc = timer_create(CLOCKID, &evp, &g_scheduleTimerId);
    if (rc != 0) {
        printf("create timer failed, %d %s\n", errno,strerror(errno));
        return rc;
    }

    /*
     start the timer
    */
    its.it_value.tv_sec = baseInterval;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = its.it_value.tv_sec;
    its.it_interval.tv_nsec = its.it_value.tv_nsec;

    rc = timer_settime(g_scheduleTimerId, 0, &its, NULL);
    if (rc != 0) {
        printf("create timer failed, %d %s\n", errno,strerror(errno));
        return rc;
    }

    return 0;
}

/*
 update timer interval
*/
void timer_helper_update_interval_from_config(int baseInterval, schedule_job *jobList, char *iniFile)
{
    printf("timer_helper_update_interval_from_config\n");

    struct ini_parser *parser = (struct ini_parser*)create_app_ini_parser(iniFile);
    if (!parser) {
        return;
    }

    schedule_job *job = &jobList[0];
    while (job->doJob) {
        if (string_is_empty(job->name)) {
            job++;
            continue;
        }

        char *strInterval = parser->value(parser, job->name);
        if (strInterval == NULL) {
            printf("No %s config\n", job->name);
            job++;
            continue;
        }

        strInterval = string_trim(strInterval,' ');

        if (string_is_empty(strInterval)) {
            printf("No %s config\n", job->name);
        }
        else {
            if (string_is_number(strInterval)) {
                int interval = atoi(strInterval);
                if (interval == 0 || interval % baseInterval == 0) {
                    job->interval = atoi(strInterval);
                    printf("Update [%s] to %d\n", job->name, job->interval);
                }
                else {
                    printf("%s is not a multiple of %d, discard it\n", job->name,baseInterval);
                }
            }
            else {
                printf("Not a valid number format\n");
            }
        }

        job++;
    }

    parser->deletor(parser);
}

/*
 free timer
*/
int timer_helper_destroy_timer()
{
     printf("timer_helper_destroy_timer\n");
    
    int rc = 0;
    if (g_scheduleTimerId > 0) {
        rc = timer_delete(g_scheduleTimerId);
    }
    return rc;
}