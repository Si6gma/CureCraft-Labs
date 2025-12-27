#ifndef INPUT_H
#define INPUT_H

#include <linux/input.h>
#include <string>

class InputReader
{
public:
    explicit InputReader(const std::string &devicePath);
    ~InputReader();

    bool isOpen() const;
    bool readEvent(input_event &ev);

private:
    int fd;
};

#endif
