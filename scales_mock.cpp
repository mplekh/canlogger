/* Copyright (C) 2024 Maxim Plekh - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the GPLv3 license.
 *
 * You should have received a copy of the GPLv3 license with this file.
 * If not, please visit : http://choosealicense.com/licenses/gpl-3.0/
 */

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>

int getRandomValue(int min, int max) {
    return rand() % (max - min + 1) + min;
}

int main() {
    srand(time(0));

    // CAN socket setup
    int s;
    struct sockaddr_can addr;
    struct ifreq ifr;

    const char *ifname = "vcan0";

    if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
        perror("Socket");
        return 1;
    }

    strcpy(ifr.ifr_name, ifname);
    ioctl(s, SIOCGIFINDEX, &ifr);

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Bind");
        return 1;
    }

    struct can_frame frame;

    while (true) {
        int bucketLoad = getRandomValue(10000, 25000);

        frame.can_id = 0x200;
        frame.can_dlc = 2;
        frame.data[0] = bucketLoad >> 8;
        frame.data[1] = bucketLoad & 0xFF;

        if (write(s, &frame, sizeof(struct can_frame)) != sizeof(struct can_frame)) {
            perror("Write");
            return 1;
        }

        std::cout << "Scoop Bucket Load Weight: " << bucketLoad << std::endl;

        int interval = getRandomValue(5, 20); // seconds between unload events
        usleep(interval * 1000000);
    }

    close(s);
    return 0;
}
