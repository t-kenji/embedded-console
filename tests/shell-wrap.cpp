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
    enum {
        PIPE_READ,
        PIPE_WRITE,
        PIPE_LENGTH
    };

    int pipe_in[PIPE_LENGTH],
        pipe_out[PIPE_LENGTH],
        pipe_err[PIPE_LENGTH];

    if ((pipe(pipe_in) != 0) || (pipe(pipe_out) != 0) || (pipe(pipe_err) != 0)) {
        perror("pipe2");
        return 1;
    }

    pid_t cpid = fork();
    if (cpid < 0) {
        close(pipe_in[PIPE_READ]);
        close(pipe_in[PIPE_WRITE]);
        close(pipe_out[PIPE_READ]);
        close(pipe_out[PIPE_WRITE]);
        close(pipe_err[PIPE_READ]);
        close(pipe_err[PIPE_WRITE]);
        perror("fork");
        return 1;
    } else if (cpid == 0) {
        if (dup2(pipe_in[PIPE_READ], STDIN_FILENO) < 0) {
            perror("dup2");
            exit(2);
        }
        close(pipe_in[PIPE_WRITE]);
        if (dup2(pipe_out[PIPE_WRITE], STDOUT_FILENO) < 0) {
            perror("dup2");
            exit(2);
        }
        close(pipe_out[PIPE_READ]);
        if (dup2(pipe_err[PIPE_WRITE], STDERR_FILENO) < 0) {
            perror("dup2");
            exit(2);
        }
        close(pipe_err[PIPE_READ]);

        execvp(argv[0], argv);
        perror("execvp");
        exit(2);
    } else {
        close(pipe_in[PIPE_READ]);
        close(pipe_out[PIPE_WRITE]);
        close(pipe_err[PIPE_WRITE]);

        do {
            int epfd;
            struct epoll_event ev;

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
            ev.data.fd = pipe_out[PIPE_READ];
            if (epoll_ctl(epfd, EPOLL_CTL_ADD, pipe_out[PIPE_READ], &ev) != 0) {
                perror("epoll_ctl");
                break;
            }
            ev.data.fd = pipe_err[PIPE_READ];
            if (epoll_ctl(epfd, EPOLL_CTL_ADD, pipe_err[PIPE_READ], &ev) != 0) {
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
                        written_len = write(pipe_in[PIPE_WRITE], buf, read_len);
                        if (written_len < 0) {
                            perror("write");
                        }
                    } else if ((events & EPOLLIN) && (fd == pipe_out[PIPE_READ])) {
                        read_len = read(pipe_out[PIPE_READ], buf, sizeof(buf));
                        if (read_len < 0) {
                            perror("read");
                        }
                        written_len = write(STDOUT_FILENO, buf, read_len);
                        if (written_len < 0) {
                            perror("write");
                        }
                    } else if ((events & EPOLLIN) && (fd == pipe_err[PIPE_READ])) {
                        read_len = read(pipe_err[PIPE_READ], buf, sizeof(buf));
                        if (read_len < 0) {
                            perror("read");
                        }
                        written_len = write(STDERR_FILENO, buf, read_len);
                        if (written_len < 0) {
                            perror("write");
                        }
                    } else {
                        is_exit = !!(events & (EPOLLHUP | EPOLLERR));
                    }
                }
            } while (!is_exit);
        } while (0);

        close(pipe_in[PIPE_WRITE]);
        close(pipe_out[PIPE_READ]);
        close(pipe_err[PIPE_READ]);
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
