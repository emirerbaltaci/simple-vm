/**
 * @file main.h
 * @author Emir Erbaltacı (erbaltaciemir@hotmail.com)
 * @brief Header File for Main Program
 * @version 0.1.0
 * @date 21-09-2026
 * 
 * @copyright Copyright (c) 2026 Emir Erbaltacı. Licensed under the MIT License.
 * 
 */

#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include "mem.h"
#include "cpu.h"

typedef enum {
    MEM_INIT_FAIL = 1,
    CPU_INIT_FAIL,
    MEM_PAGEALLOC_FAIL,
}Runtime_Error_t;

#endif // MAIN_H
