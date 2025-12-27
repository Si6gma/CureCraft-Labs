#include <iostream>

#include "math.h"
#include "input.h"

int main()
{
    int a = 5;
    int b = 120;
    int sum = add(a, b);
    std::cout << "The sum of " << a << " and " << b << " is: " << sum << std::endl;

    std::cout << "Press keys (q to quit)\n";

    while (true)
    {
        char c = read_keypress();

        std::cout << "Key: '" << c
                  << "' ASCII: " << static_cast<int>(c) << "\n";

        if (c == 'q')
            break;
    }

    return 0;
}