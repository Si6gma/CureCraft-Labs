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
        unsigned char uc = static_cast<unsigned char>(c);

        if (uc >= 32 && uc <= 126)
        { // printable ASCII
            std::cout << "Key: '" << c << "'  code=" << (int)uc << "\n";
        }
        else
        {
            std::cout << "Key: (non-printable) code=" << (int)uc << "\n";
        }
        if (c == 'q')
            break;
    }

    return 0;
}