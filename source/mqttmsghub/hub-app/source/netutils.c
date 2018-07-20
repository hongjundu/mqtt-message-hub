/*
 *  File: netutils.c
 *  This file is a part of mqttmsghub
 *
 *  Created by Du Hongjun on 2017/06.
 *  Copyright 2018 All rights reserved.
 */

#include "netutils.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "logger.h"
#include "common.h"
#include "stringutils.h"

#define ARP_PROC_FILE "/proc/net/arp"

/*
  Get an IP's MAC address from the ARP cache.
*/
int arp_get_mac(const char *reqIp, char *outMac)
{
    Log_debug("arp_get_mac: %s", safe_string(reqIp));

    int rc = FAILURE;

    FILE *procFile = fopen(ARP_PROC_FILE, "r");
    if (!procFile) {
        printf("%s open failed\n", ARP_PROC_FILE);
        return rc;
    }

    char ip[16] = { 0 };
    char mac[18] = { 0 };

    /* Skip first line */
    while (!feof(procFile) && fgetc(procFile) != '\n');

    /* Find ip, copy mac in out argument */
    while (!feof(procFile) && (fscanf(procFile, " %15[0-9.] %*s %*s %17[A-Fa-f0-9:] %*s %*s", ip, mac) == 2)) {
      if (strcmp(ip, reqIp) == 0) {
            printf("%s\n", mac);
            strcpy(outMac, mac);
            rc = SUCCESS;
            break;
      }
    }

    fclose(procFile);

    if (rc < 0) {
        printf("No mac found for ip: %s\n", reqIp);
    }

    return rc;
}