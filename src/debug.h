/** @file       debug.h
 *  @brief      Debug functions .
 *
 *  @author     t-kenji <protect.2501@gmail.com>
 *  @date       2018-09-16 create new.
 *  @copyright  Copyright © 2018 t-kenji
 *
 *  This code is licensed under the MIT License.
 */
#ifndef __ECON_DEBUG_H__
#define __ECON_DEBUG_H__

#if NODEBUG != 1
#define DEBUG(fmt, ...)                          \
    do {                                         \
        fprintf(stderr, "D:%s:%d:%s$ " fmt "\n", \
                __FILE__, __LINE__, __func__,    \
                ##__VA_ARGS__);                  \
    } while (0)
#else
#define DEBUG(fmt, ...)
#endif

#endif /* __ECON_DEBUG_H__ */
