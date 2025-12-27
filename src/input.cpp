#include "input.h"

#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

InputReader::InputReader(const std::string &devicePath)
    : fd(-1)
{
    fd = open(devicePath.c_str(), O_RDONLY);
    if (fd < 0)
    {
        std::cerr << "Failed to open " << devicePath
                  << ": " << strerror(errno) << std::endl;
    }
}

InputReader::~InputReader()
{
    if (fd >= 0)
        close(fd);
}

bool InputReader::isOpen() const
{
    return fd >= 0;
}

bool InputReader::readEvent(input_event &ev)
{
    ssize_t bytes = read(fd, &ev, sizeof(ev));
    return bytes == sizeof(ev);
}
