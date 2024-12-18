#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sqlite3.h>
#include <ctime>
#include <vector>

constexpr unsigned MIN_ENTRIES_TO_SEND = 100;

struct CanData {
    int can_id;
    int value;
    int64_t timestamp;
};

bool sockSend(const std::vector<CanData>& messages) {
    // Dummy logic for now
    for (const auto& msg : messages) {
        std::cout << "Sending data: can_id=" << msg.can_id 
                  << ", value=" << msg.value 
                  << ", timestamp=" << msg.timestamp << std::endl;
    }
    return true;
}

static int fetchCallback(void* data, int argc, char** argv, char** azColName) {
    std::vector<CanData>* messages = static_cast<std::vector<CanData>*>(data);

    CanData msg;
    msg.can_id = std::stoi(argv[1]);
    msg.value = std::stoi(argv[2]);
    msg.timestamp = std::stoll(argv[3]);

    messages->push_back(msg);
    return 0;
}

bool wlanAvailable() {
    return true;
}

bool uploadData(sqlite3* db) {
    if (!wlanAvailable()) return false;

    std::vector<CanData> messages;
    char* errorMessage = nullptr;
    int rc = sqlite3_exec(db, "SELECT * FROM can_data", fetchCallback, &messages, &errorMessage);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errorMessage << std::endl;
        sqlite3_free(errorMessage);
        return false;
    }
    return sockSend(messages);
}

void printData(CanData entry) {
    std::string name;
    std::string unit;
    switch (entry.can_id) {
        case 0x100: {
            name = "Engine RPM: ";
            unit = "RPM";
            break;
        }
        case 0x101: {
            name = "Coolant Temperature: ";
            unit = "°C";
            break;
        }
        case 0x102: {
            name = "Engine Oil Temperature: ";
            unit = "°C";
            break;
        }
        case 0x103: {
            name = "Engine Oil Pressure: ";
            unit = "kPa";
            break;
        }
        case 0x104: {
            name = "Hydraulic Oil Temperature: ";
            unit = "°C";
            break;
        }
        case 0x105: {
            name = "Hydraulic Oil Pressure: ";
            unit = "bar";
            break;
        }
        case 0x200: {
            name = "Scoop Bucket Load Weight: ";
            unit = "kg";
            break;
        }
        default: {
            name = "Unknown value: ";
            break;
        }
    }
    std::cout << entry.timestamp << ": "<< name << entry.value << unit << std::endl;
};

void insertData(sqlite3* db, int can_id, int value) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    const int64_t timestamp_ms = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    printData({can_id, value, timestamp_ms});

    const char * sql = "INSERT INTO can_data (can_id, value, timestamp) VALUES (?, ?, ?);";
    char* errMsg = nullptr;
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        exit(1);
    }
    sqlite3_bind_int(stmt, 1, can_id);
    sqlite3_bind_int(stmt, 2, value);
    sqlite3_bind_int64(stmt, 3, timestamp_ms);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "SQL error: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_free(errMsg);
    } else {
        sqlite3_finalize(stmt);
    }
}

void deleteAllEntries(sqlite3* db) {
    const char* sql = "DELETE FROM can_data;";
    char* errMsg = nullptr;

    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    } else {
        std::cout << "Buffered entries deleted." << std::endl;
    }
}

int main() {
    // CAN socket setup
    int s;
    struct sockaddr_can addr;
    struct ifreq ifr;

    const char *ifname = "vcan0";

    sqlite3* db;
    int rc;

    //Use in-memory database as buffer
    rc = sqlite3_open(":memory:", &db);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        exit(1);
    }
    const char* createTableSQL = "CREATE TABLE can_data ("
                                 "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                 "can_id INT NOT NULL,"
                                 "value INT NOT NULL,"
                                 "timestamp INT64 NOT NULL);";
    rc = sqlite3_exec(db, createTableSQL, nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        exit(1);
    }

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

    // CAN frame buffer
    struct can_frame frame;
    int packetCount = 0;
    while (true) {
        // Read a CAN frame from the socket
        ssize_t nbytes = read(s, &frame, sizeof(struct can_frame));
        if (nbytes < 0) {
            perror("Read");
            return 1;
        }
        if (frame.can_dlc >=sizeof(int)) {
            perror("Too much data per frame, at most int expected from mock");
            return 1;
        }

        int value = 0;
        for (int i = 0; i < frame.can_dlc; i++) {
            value = (value << 8) | frame.data[i];
        }
        insertData(db, frame.can_id, value);
        if (++packetCount >= MIN_ENTRIES_TO_SEND && uploadData(db)) {
            deleteAllEntries(db);
            packetCount = 0;
        }
    }

    // never reach here in this version
    sqlite3_close(db);
    close(s);
    return 0;
}
