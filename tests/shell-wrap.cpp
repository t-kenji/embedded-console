/** @file       shell-wrap.cpp
 *  @brief      Standard IO wrapper.
 *
 *  @author     t-kenji <protect.2501@gmail.com>
 *  @date       2018-09-16 create new.
 *  @copyright  Copyright © 2018 t-kenji
 *
 *  This code is licensed under the MIT License.
 */
#include <cstdio>
#include <cstdlib>
#include <cstdbool>
#include <cstddef>
#include <cstdint>
#include <cerrno>
#include <inttypes.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>

#include "ascii.h"

/**
 *  command usage.
 *
 *  @param  [in]    name    command name.
 */
static void usage(const char *name)
{
    printf("usage: %s command [args]\n", name);
}

static int invoke(int argc, char **argv)
{
    struct termios old_term;
    struct winsize old_size;

    tcgetattr(STDIN_FILENO, &old_term);
    ioctl(STDIN_FILENO, TIOCGWINSZ, (char *)&old_size);

    int pty_master = posix_openpt(O_RDWR);
    grantpt(pty_master);
    unlockpt(pty_master);
    char *pts_name = ptsname(pty_master);
    printf("using pts: %s\n", pts_name);

    pid_t cpid = fork();
    if (cpid < 0) {
        close(pty_master);
        perror("fork");
        return 1;
    } else if (cpid == 0) {
        setsid();
        close(pty_master);

        int pty_slave = open(pts_name, O_RDWR);
        tcsetattr(pty_slave, TCSAFLUSH, &old_term);
        ioctl(pty_slave, TIOCSWINSZ, &old_size);

        if (dup2(pty_slave, STDIN_FILENO) < 0) {
            perror("dup2");
            exit(2);
        }
        if (dup2(pty_slave, STDOUT_FILENO) < 0) {
            perror("dup2");
            exit(2);
        }
        if (dup2(pty_slave, STDERR_FILENO) < 0) {
            perror("dup2");
            exit(2);
        }
        close(pty_slave);

        execvp(argv[0], argv);
        perror("execvp");
        exit(2);
    } else {

        do {
            int epfd;
            struct epoll_event ev;
            struct termios new_term = old_term;

            new_term.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
            new_term.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
            new_term.c_cflag &= ~(CSIZE | PARENB);
            new_term.c_cflag |= CS8;
            new_term.c_oflag &= ~(OPOST);
            new_term.c_cc[VMIN] = 1;
            new_term.c_cc[VTIME] = 0;
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_term);

            epfd = epoll_create1(0);
            if (epfd < 0) {
                perror("epoll_create1");
                break;
            }
            ev.events = EPOLLIN;
            ev.data.fd = STDIN_FILENO;
            if (epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev) != 0) {
                perror("epoll_ctl");
                break;
            }
            ev.data.fd = pty_master;
            if (epoll_ctl(epfd, EPOLL_CTL_ADD, pty_master, &ev) != 0) {
                perror("epoll_ctl");
                break;
            }

            bool is_exit = false;
            do {
                static char buf[BUFSIZ];
                static struct epoll_event evs[10];
                int nevs = epoll_wait(epfd, evs, 10, -1);
                if (nevs < 0) {
                    perror("epoll_wait");
                    continue;
                }
                for (int i = 0; i < nevs; ++i) {
                    ssize_t read_len, written_len;
                    uint32_t events = evs[i].events;
                    int fd = evs[i].data.fd;

                    if ((events & EPOLLIN) && (fd == STDIN_FILENO)) {
                        read_len = read(STDIN_FILENO, buf, sizeof(buf));
                        if (read_len < 0) {
                            perror("read");
                        }
                        switch (buf[0]) {
                        case ETX:
                            is_exit = true;
                            printf("^C\r\n");
                            break;
                        default:
                            written_len = write(pty_master, buf, read_len);
                            if (written_len < 0) {
                                perror("write");
                            }
                            break;
                        }
                    } else if ((events & EPOLLIN) && (fd == pty_master)) {
                        read_len = read(pty_master, buf, sizeof(buf));
                        if (read_len < 0) {
                            perror("read");
                        }
                        written_len = write(STDOUT_FILENO, buf, read_len);
                        if (written_len < 0) {
                            perror("write");
                        }
                    } else {
                        is_exit = !!(events & (EPOLLHUP | EPOLLERR));
                    }
                }
            } while (!is_exit);

            kill(cpid, SIGTERM);
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &old_term);
            close(pty_master);
        } while (0);
    }

    return 0;
}

/**
 *  startup.
 *
 *  @param  [in]    argc    command-line argument count.
 *  @param  [in]    argv    command-line argument values.
 *  @return returns 0 on success.
 *          on error, 1 returned.
 */
int main(int argc, char **argv)
{
    enum {
        ARG_SELF,
        ARG_COMMAND,
        ARG_COMMAND_ARGS,
    };

    if (argc < ARG_COMMAND_ARGS) {
        usage(argv[ARG_SELF]);
        exit(1);
    }

    return invoke(argc - ARG_COMMAND, &argv[ARG_COMMAND]);
}
