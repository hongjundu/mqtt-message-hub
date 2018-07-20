/*
 *  File: netutils.h
 *  This file is a part of mqttmsghub
 *
 *  Created by Du Hongjun on 2017/06.
 *  Copyright 2018 All rights reserved.
 */

#pragma once

/*
 Get an IP's MAC address from the ARP cache.
*/
int arp_get_mac(const char *reqIp, char *outMac);