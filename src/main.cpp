#include <iostream>

#include "math.h"
#include "input.h"

int main()
{
    int a = 5;
    int b = 120;
    int sum = add(a, b);
    std::cout << "The sum of " << a << " and " << b << " is: " << sum << std::endl;

    return 0;
}