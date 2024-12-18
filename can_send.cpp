#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <unistd.h>
#include <net/if.h>

int main() {
    int s;
    struct sockaddr_can addr;
    struct can_frame frame;

    // Create socket
    if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
        perror("Socket");
        return 1;
    }

    // Setup address structure
    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = if_nametoindex("vcan0");

    // Bind socket to the CAN interface
    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Bind");
        return 1;
    }

    // Prepare a CAN frame
    memset(&frame, 0, sizeof(frame));
    frame.can_id = 0x123; // CAN ID
    frame.can_dlc = 8;   // Data length code (number of bytes)
    for (int i = 0; i < 8; i++) {
        frame.data[i] = i + 'A'; // Example data
    }

    // Send the CAN frame
    if (write(s, &frame, sizeof(frame)) != sizeof(frame)) {
        perror("Write");
        return 1;
    }

    std::cout << "CAN message sent successfully" << std::endl;

    close(s);
    return 0;
}
