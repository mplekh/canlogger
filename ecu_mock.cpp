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

int getRandomValue(int current, int min, int max) {
    static bool seeded = false;
    if (!seeded) {
        std::srand(static_cast<unsigned int>(std::time(0)));
        seeded = true;
    }

    const int range = max - min + 1;
    int max_change = range / 50;  // Base max_change as 2% of the range

    if (current < min) current = min;
    if (current > max) current = max;

    int lower_bound = std::max(min, current - max_change);
    int upper_bound = std::min(max, current + max_change);
    if (lower_bound > upper_bound) return current;

    return lower_bound + std::rand() % (upper_bound - lower_bound + 1);
}

int main() {
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

    int engineRPM = 0;
    int coolantTemp = 0;
    int oilTemp = 0;
    int oilPressure = 0;
    int hydraulicOilTemp = 0;
    int hydraulicOilPressure = 0;


    while (true) {
        engineRPM = getRandomValue(engineRPM, 800, 6500);
        coolantTemp = getRandomValue(coolantTemp, -40, 120); // in Celsius
        oilTemp = getRandomValue(oilTemp, -30, 150);     // in Celsius
        oilPressure = getRandomValue(oilPressure, 0, 700);    // in kPa
        hydraulicOilTemp = getRandomValue(hydraulicOilTemp, -20, 140); // in Celsius
        hydraulicOilPressure = getRandomValue(hydraulicOilPressure, 0, 600); // in bar

        std::cout << "\rEngine RPM: " << engineRPM 
                  << ", Coolant Temp: " << coolantTemp << "°C"
                  << ", Oil Temp: " << oilTemp << "°C"
                  << ", Oil Pressure: " << oilPressure << " kPa"
                  << ", Hydraulic Oil Temp: " << hydraulicOilTemp << "°C"
                  << ", Hydraulic Oil Pressure: " << hydraulicOilPressure << " bar"
                  << std::flush;

        // Prepare CAN frames for each signal

        // Engine RPM (0x100)
        frame.can_id = 0x100;
        frame.can_dlc = 2; // Data length code
        frame.data[0] = engineRPM >> 8;
        frame.data[1] = engineRPM & 0xFF;

        if (write(s, &frame, sizeof(struct can_frame)) != sizeof(struct can_frame)) {
            perror("Write");
            return 1;
        }

        // Coolant Temp (0x101)
        frame.can_id = 0x101;
        frame.can_dlc = 1; // Data length code
        frame.data[0] = coolantTemp;

        if (write(s, &frame, sizeof(struct can_frame)) != sizeof(struct can_frame)) {
            perror("Write");
            return 1;
        }

        // Oil Temp (0x102)
        frame.can_id = 0x102;
        frame.can_dlc = 1; // Data length code
        frame.data[0] = oilTemp;

        if (write(s, &frame, sizeof(struct can_frame)) != sizeof(struct can_frame)) {
            perror("Write");
            return 1;
        }

        // Oil Pressure (0x103)
        frame.can_id = 0x103;
        frame.can_dlc = 2; // Data length code
        frame.data[0] = oilPressure >> 8;
        frame.data[1] = oilPressure & 0xFF;

        if (write(s, &frame, sizeof(struct can_frame)) != sizeof(struct can_frame)) {
            perror("Write");
            return 1;
        }

        // Hydraulic Oil Temp (0x104)
        frame.can_id = 0x104;
        frame.can_dlc = 1; // Data length code
        frame.data[0] = hydraulicOilTemp;

        if (write(s, &frame, sizeof(struct can_frame)) != sizeof(struct can_frame)) {
            perror("Write");
            return 1;
        }

        // Hydraulic Oil Pressure (0x105)
        frame.can_id = 0x105;
        frame.can_dlc = 2; // Data length code
        frame.data[0] = hydraulicOilPressure >> 8;
        frame.data[1] = hydraulicOilPressure & 0xFF;

        if (write(s, &frame, sizeof(struct can_frame)) != sizeof(struct can_frame)) {
            perror("Write");
            return 1;
        }

        usleep(500000); // 500 ms
    }

    close(s);
    return 0;
}
