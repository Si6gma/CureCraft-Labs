#include <iostream>
#include <string>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <thread>
#include <chrono>

#define HUB_ADDR 0x08
#define CMD_PING 0x00
#define CMD_SCAN 0x01

int open_i2c(int bus) {
    std::string device = "/dev/i2c-" + std::to_string(bus);
    int fd = open(device.c_str(), O_RDWR);
    if (fd < 0) {
        std::cerr << "Failed to open " << device << ": " << strerror(errno) << std::endl;
        return -1;
    }
    if (ioctl(fd, I2C_SLAVE, HUB_ADDR) < 0) {
        std::cerr << "Failed to set slave address 0x" << std::hex << HUB_ADDR << ": " << strerror(errno) << std::endl;
        close(fd);
        return -1;
    }
    return fd;
}

bool test_ping(int fd) {
    std::cout << "\n[TEST] Testing PING (0x00)..." << std::endl;
    uint8_t cmd = CMD_PING;
    
    // Write Command
    if (write(fd, &cmd, 1) != 1) {
        std::cerr << "  ERROR: Write failed: " << strerror(errno) << std::endl;
        return false;
    }
    std::cout << "  > Sent PING command" << std::endl;

    // Small delay (simulating driver behavior)
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Read Response
    uint8_t resp = 0;
    if (read(fd, &resp, 1) != 1) {
        std::cerr << "  ERROR: Read failed: " << strerror(errno) << std::endl;
        return false;
    }

    std::cout << "  < Received: 0x" << std::hex << (int)resp << std::dec << std::endl;
    if (resp == 0x42) {
        std::cout << "  ✓ PASS: Got expected 0x42" << std::endl;
        return true;
    } else {
        std::cout << "  ✗ FAIL: Expected 0x42" << std::endl;
        return false;
    }
}

bool test_scan(int fd) {
    std::cout << "\n[TEST] Testing SCAN (0x01)..." << std::endl;
    uint8_t cmd = CMD_SCAN;
    
    // Write Command
    if (write(fd, &cmd, 1) != 1) {
        std::cerr << "  ERROR: Write failed: " << strerror(errno) << std::endl;
        return false;
    }
    std::cout << "  > Sent SCAN command" << std::endl;

    // Wait for hub processing (mimics driver logic)
    std::cout << "  ... Waiting 50ms ..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Read Response
    uint8_t resp = 0;
    if (read(fd, &resp, 1) != 1) {
        std::cerr << "  ERROR: Read failed: " << strerror(errno) << std::endl;
        return false;
    }

    std::cout << "  < Received: 0x" << std::hex << (int)resp << std::dec << std::endl;
    std::cout << "  ✓ PASS: Read successful" << std::endl;
    return true;
}

int main(int argc, char* argv[]) {
    int bus = 1;
    if (argc > 1) {
        bus = std::stoi(argv[1]);
    }

    std::cout << "Opening I2C Bus " << bus << "..." << std::endl;
    int fd = open_i2c(bus);
    if (fd < 0) return 1;

    bool ping_ok = test_ping(fd);
    bool scan_ok = test_scan(fd);

    close(fd);
    
    if (ping_ok && scan_ok) {
        std::cout << "\n✓ ALL TESTS PASSED" << std::endl;
        return 0;
    } else {
        std::cout << "\n✗ SOME TESTS FAILED" << std::endl;
        return 1;
    }
}
