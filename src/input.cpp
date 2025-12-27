#include "input.h"
#include <termios.h>
#include <unistd.h>
#include <cstdlib>

static struct termios old_tio;

static void restore_terminal()
{
    tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
}

char read_keypress()
{
    struct termios new_tio;

    // Save terminal state once
    static bool initialized = false;
    if (!initialized)
    {
        tcgetattr(STDIN_FILENO, &old_tio);
        new_tio = old_tio;
        new_tio.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
        atexit(restore_terminal);
        initialized = true;
    }

    char c;
    read(STDIN_FILENO, &c, 1);
    return c;
}
