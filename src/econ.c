/** @file       econ.c
 *  @brief      Embedded shell core implementation.
 *
 *  @author     t-kenji <protect.2501@gmail.com>
 *  @date       2018-09-16 create new.
 *  @copyright  Copyright © 2018 t-kenji
 *
 *  This code is licensed under the MIT License.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "econ.h"
#include "debug.h"
#include "utils.h"

/**
 *  parse arguments.
 *
 *  @param  [in,out]    buf     input buffer.
 *  @param  [out]       argv    command argument vector.
 *  @param  [in]        length  command argument vector length.
 *  @return returns argument count on success.
 */
static int parse_argument(char *buf, char **argv, size_t length)
{
    int count = 0;
    bool is_word = false;

    for (int i = 0; buf[i] != '\0'; ++i) {
        int c = buf[i];

        if ((c != ' ') && (c != '\t') && (c != '\n')) {
            if (!is_word) {
                argv[count++] = &buf[i];
            }
            is_word = true;
        } else {
            buf[i] = '\0';
            if (is_word && (count >= length)) {
                break;
            }
            is_word = false;
        }
    }

    return count;
}

/**
 *  @details    input handling with show prompt.
 *
 *  @param      [in]    prompt  prompt string.
 *  @param      [out]   argv    argument vector.
 *  @param      [in]    length  argument vector length.
 *  @return     returns argument count on success.
 *              on error, -1 returned, and @c errno set.
 */
int econ_prompt(const char *prompt, char **argv, size_t length)
{
    static char buf[BUFSIZ];

    int count = 0;

    prompt = (prompt) ?: "econ>";

    printf("%s ", prompt);
    do {
        int c = getchar();
        if ((0x00 <= c) && (c < 0xFF)) {
            if (c == '\r') {
                c = '\n';
            }
            if (c == '\n') {
                buf[count] = '\0';
                break;
            } else {
                buf[count++] = c;
            }
        }
    } while (true);

    return parse_argument(buf, argv, length);
}

/**
 *  @details    invoke @c argv command from @c cmds.
 *
 *  @param      [in]    argc    command argument count.
 *  @param      [in]    argv    command argument values.
 *  @param      [in]    cmds    command list.
 *  @return     returns 0 on success.
 *              on error, -1 is returned, and @c errno set.
 */
int econ_invoke(int argc, char **argv, struct econ_command *cmds)
{
    int col_length = 0;
    for (int i = 0; cmds[i].command != NULL; ++i) {
        struct econ_command *cmd = &cmds[i];
        size_t length = strlen(cmd->command);

        if ((argc > 0) && (strcmp(cmd->command, argv[0]) == 0)) {
            if (cmd->sub_cmds != NULL) {
                return econ_invoke(argc - 1, &argv[1], cmd->sub_cmds);
            } else if (cmd->func != NULL) {
                int ret = cmd->func(argc, argv);
                if ((ret != 0) && (cmd->usage != NULL)) {
                    cmd->usage(argv[0]);
                }
                return ret;
            }
        }

        if (length > col_length) {
            col_length = length;
        }
    }

    if (argc > 0) {
        printf("%s: command not found\n", argv[0]);
    }
    printf("\navailable list.\n");
    for (int i = 0; cmds[i].command != NULL; ++i) {
        struct econ_command *cmd = &cmds[i];

        printf("* %-*s: %s\n", col_length + 1, cmd->command, cmd->help);
    }

    errno = ENOENT;
    return -1;
}
