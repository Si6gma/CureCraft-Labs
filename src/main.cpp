#include <iostream>
#include <linux/input.h>

#include "math.h"
#include "input.h"

int main()
{
    int a = 5;
    int b = 120;
    int sum = add(a, b);
    std::cout << "The sum of " << a << " and " << b << " is: " << sum << std::endl;

    InputReader input("/dev/input/event3"); // CHANGE THIS

    if (!input.isOpen())
        return 1;

    input_event ev;

    while (true)
    {
        if (!input.readEvent(ev))
            continue;

        if (ev.type == EV_KEY)
        {
            if (ev.value == 1)
                std::cout << "Key pressed:  " << ev.code << '\n';
            else if (ev.value == 0)
                std::cout << "Key released: " << ev.code << '\n';
            else if (ev.value == 2)
                std::cout << "Key repeat:   " << ev.code << '\n';
        }
    }

    return 0;
}