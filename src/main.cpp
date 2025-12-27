#include <iostream>

#include "math.h"

int main()
{
    int a = 5;
    int b = 120;
    int sum = add(a, b);
    std::cout << "The sum of " << a << " and " << b << " is: " << sum << std::endl;

    while (true)
    {
        for (int i = 0; i < 100000; i++)
        {
            std::cout << "This is " << i << std::endl;
        }
    }

    return 0;
}