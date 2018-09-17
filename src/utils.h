/** @file       utils.h
 *  @brief      Utilities.
 *
 *  @author     t-kenji <protect.2501@gmail.com>
 *  @date       2018-09-16 create new.
 *  @copyright  Copyright © 2018 t-kenji
 *
 *  This code is licensed under the MIT License.
 */
#ifndef __ECON_UTILS_H__
#define __ECON_UTILS_H__

/**
 *  get array length.
 *
 *  @param      [in]    array   array valiable.
 *  @return     array length.
 *  @warning    only array. (not pointer)
 */
#define lengthof(array) (sizeof(array)/sizeof(array[0]))

#endif /* __ECON_UTILS_H__ */
