/** @file       main.cpp
 *  @brief      Embedded console tests main.
 *
 *  @author     t-kenji <protect.2501@gmail.com>
 *  @date       2018-09-16 create new.
 *  @copyright  Copyright © 2018 t-kenji
 *
 *  This code is licensed under the MIT License.
 */
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <termios.h>

#include "econ.h"

extern "C" {

#include "debug.h"

static void dummy_usage(const char *name)
{
    printf("usage: %s\n", name);
}

static int dummy(int argc, char **argv)
{
    if (argc < 2) {
        return -1;
    }

    DEBUG("called: %s %s", argv[0], argv[1]);

    return 0;
}

static int aaa(int argc, char **argv)
{
    DEBUG("called: %d", argc);

    return -1;
}

static struct termios saved_term;
static int cmd_exit(int argc, char **argv)
{
    if (isatty(STDIN_FILENO)) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_term);
    }
    exit(0);
}

static struct econ_command sub_cmds[] = {
    ECON_COMMAND("dummy", dummy, "dummy help", NULL),
    ECON_END_OF_COMMAND()
};

static struct econ_command test_cmds[] = {
    ECON_COMMAND("dummy", dummy, "help message", dummy_usage),
    ECON_COMMAND("dummmmmmmmmmmmmmmmmmmmmmmy", dummy, "help message", dummy_usage),
    ECON_SUBCOMMAND("sub", sub_cmds, "sub-commands help"),
    ECON_COMMAND("aaa", aaa, "aaa help", NULL),
    ECON_COMMAND("exit", cmd_exit, "exit console", NULL),
    ECON_END_OF_COMMAND()
};

}

/**
 *  main desc.
 *
 *  @param  [in]    argc    command-line argument count.
 *  @param  [in]    argv    command-line argument values.
 *  @return returns 0 on success.
 *          on error, 1 is returned.
 */
int main(int argc, char **argv)
{
    if (isatty(STDIN_FILENO)) {
        tcgetattr(STDIN_FILENO, &saved_term);

        struct termios new_term;
        new_term = saved_term;
        new_term.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
        new_term.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
        new_term.c_cflag &= ~(CSIZE | PARENB);
        new_term.c_cflag |= CS8;
        new_term.c_oflag &= ~(OPOST);
        new_term.c_cc[VMIN] = 1;
        new_term.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_term);
    }

    do {
        char *cmd_args[24] = {0};
        int cmd_argc = econ_prompt("test $", cmd_args, 24);

        if (cmd_argc > 0) {
            econ_invoke(cmd_argc, cmd_args, test_cmds);
        }
    } while (1);

    return 0;
}
