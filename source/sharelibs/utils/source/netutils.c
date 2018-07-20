#include "netutils.h"
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>

#define IF_INTERFACE            "wlan0"

int get_mac_string(char* mac)
{
    struct ifreq tmp;
    int sock_mac;
    char mac_addr[30];
    sock_mac = socket(AF_INET, SOCK_STREAM, 0);

    if( sock_mac == -1){
        perror("create socket fail\n");
        return -1;
    }

    memset(&tmp, 0, sizeof(tmp));
    strncpy(tmp.ifr_name, IF_INTERFACE, sizeof(tmp.ifr_name) - 1);

    if( (ioctl( sock_mac, SIOCGIFHWADDR, &tmp)) < 0 ){
        printf("mac ioctl error\n");
        return -1;
    }

    sprintf(mac_addr, "%02x%02x%02x%02x%02x%02x",
            (unsigned char)tmp.ifr_hwaddr.sa_data[0],
            (unsigned char)tmp.ifr_hwaddr.sa_data[1],
            (unsigned char)tmp.ifr_hwaddr.sa_data[2],
            (unsigned char)tmp.ifr_hwaddr.sa_data[3],
            (unsigned char)tmp.ifr_hwaddr.sa_data[4],
            (unsigned char)tmp.ifr_hwaddr.sa_data[5]
            );

    printf("local mac:%s\n", mac_addr);

    close(sock_mac);
    memcpy(mac, mac_addr, strlen(mac_addr));

    return 0;
}