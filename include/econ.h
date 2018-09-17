/** @file       econ.h
 *  @brief      Embedded console core library.
 *
 *  @author     t-kenji <protect.2501@gmail.com>
 *  @date       2018-09-16 create new.
 *  @copyright  Copyright © 2018 t-kenji
 *
 *  This code is licensed under the MIT License.
 */
#ifndef __ECON_H__
#define __ECON_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  command structure.
 */
struct econ_command {
    const char *command;           /**< command name. */
    struct econ_command *sub_cmds; /**< sub-command. */
    int (*func)(int, char **);     /**< command function. */
    const char *help;              /**< help message. */
    void (*usage)(const char *);   /**< command usage. */
};

/**
 *  command registration helper.
 */
#define ECON_COMMAND(c, f, h, u) \
    {.command=(c), .sub_cmds=NULL, .func=(f), .help=(h), .usage=(u)}

/**
 *  sub-command registration helper.
 */
#define ECON_SUBCOMMAND(c, s, h) \
    {.command=(c), .sub_cmds=(s), .func=NULL, .help=(h), .usage=NULL}

/**
 *  command list terminator.
 */
#define ECON_END_OF_COMMAND() \
    {.command=NULL, .sub_cmds=NULL, .func=NULL, .help=NULL, .usage=NULL}

/**
 *  command prompt.
 */
int econ_prompt(const char *prompt, char **argv, size_t length);

/**
 *  command invoke.
 */
int econ_invoke(int argc, char **argv, struct econ_command *cmds);

#ifdef __cplusplus
}
#endif

#endif /* __ECON_H__ */
