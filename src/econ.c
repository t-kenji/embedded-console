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
#include <fcntl.h>
#include <errno.h>
#include <sys/epoll.h>

#include "econ.h"
#include "ascii.h"
#include "debug.h"
#include "utils.h"

FILE *logger = NULL;

/**
 *  stdin polling fd.
 */
int epfd = -1;

/**
 *  get length of array.
 */
#define lengthof(array) (sizeof(array)/sizeof(array[0]))

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

    for (int i = 0; buf[i] != NUL; ++i) {
        int c = buf[i];

        if ((c != SP) && (c != TAB) && (c != LF)) {
            if (!is_word) {
                argv[count++] = &buf[i];
            }
            is_word = true;
        } else {
            buf[i] = NUL;
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
    bool has_eol = false;
    char buf[BUFSIZ] = {0};
    int count = 0,
        cursor = 0;

    if (epfd < 0) {
        int val = fcntl(STDIN_FILENO, F_GETFL, 0);
        if ((val & O_NONBLOCK) == 0) {
            fcntl(STDIN_FILENO, F_SETFL, val | O_NONBLOCK);
        }

        epfd = epoll_create1(0);
        if (epfd < 0) {
            perror("epoll_create1");
            return -1;
        }
        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = STDIN_FILENO;
        if (epoll_ctl(epfd, EPOLL_CTL_ADD,ev.data.fd, &ev) != 0) {
            perror("epoll_ctl");
            close(epfd);
            epfd = -1;
            return -1;
        }
    }

    prompt = (prompt) ?: "econ>";

    printf("%s ", prompt);
    fflush(stdout);
    do {
        static struct epoll_event events[10];
        int nevs = epoll_wait(epfd, events, lengthof(events), -1);
        if (nevs < 0) {
            perror("epoll_wait");
            break;
        }
        for (int i = 0; i < nevs; ++i) {
            if ((events[i].events & EPOLLIN) && (events[i].data.fd == STDIN_FILENO)) {
                int c;
                while ((c = getchar()) != EOF) {
                    if ((0x00 <= c) && (c < 0xFF)) {
                        if (c == CR) {
                            c = LF;
                        }
                        if (c == LF) {
                            buf[count] = NUL;
                            putchar(CR);
                            putchar(LF);
                            has_eol = true;
                        } else {
                            if ((SP <= c) && (c < DEL)) {
                                if (cursor == count) {
                                    putchar(c);
                                } else {
                                    for (int i = count; i > cursor; --i) {
                                        buf[i] = buf[i - 1];
                                    }
                                }
                                buf[cursor++] = c;
                                ++count;
                                if (cursor != count) {
                                    printf("\r%s %s\033[%dD", prompt, buf, count - cursor);
                                }
                            } else {
                                switch (c) {
                                case ESC:
                                    getchar(); /* skip '[' */
                                    c = getchar();
                                    switch (c) {
                                    case '3': /* delete */
                                        getchar(); /* skip '~' */
                                        if (cursor < count) {
                                            for (int i = cursor; i < count; ++i) {
                                                buf[i] = buf[i + 1];
                                            }
                                            --count;
                                            printf("\r%s %s\033[K\033[%dD", prompt, buf, count - cursor);
                                        }
                                        break;
                                    case 'A': /* up */
                                        break;
                                    case 'B': /* down */
                                        break;
                                    case 'C': /* right */
                                        if (++cursor > count) {
                                            cursor = count;
                                        } else {
                                            printf("\033[1C");
                                        }
                                        break;
                                    case 'D': /* left */
                                        if (--cursor < 0) {
                                            cursor = 0;
                                        } else {
                                            printf("\033[1D");
                                        }
                                        break;
                                    default:
                                        break;
                                    }
                                    break;
                                case DEL: /* backspace */
                                    if (cursor == count) {
                                        if (count > 0) {
                                            --count;
                                            --cursor;
                                            buf[cursor] = NUL;
                                            printf("\033[1D\033[K");
                                        }
                                    } else {
                                        --cursor;
                                        for (int i = cursor; i < count; ++i) {
                                            buf[i] = buf[i + 1];
                                        }
                                        --count;
                                        printf("\r%s %s\033[K\033[%dD", prompt, buf, count - cursor);
                                    }
                                    break;
                                default:
                                    break;
                                }
                            }
                        }
                    }
                }
            } else {
                DEBUG("events: %x, fd: %d", events[i].events, events[i].data.fd);
                has_eol = true;
            }
        }
    } while (!has_eol);

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
        printf("%s: command not found\r\n", argv[0]);
    }
    printf("\navailable list.\r\n");
    for (int i = 0; cmds[i].command != NULL; ++i) {
        struct econ_command *cmd = &cmds[i];

        printf("* %-*s: %s\r\n", col_length + 1, cmd->command, cmd->help);
    }

    errno = ENOENT;
    return -1;
}
