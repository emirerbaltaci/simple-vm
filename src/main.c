/**
 * @file main.c
 * @author Emir Erbaltacı (erbaltaciemir@hotmail.com)
 * @brief Main Program Implementation
 * @version 0.1.0
 * @date 21-09-2026
 * 
 * @copyright Copyright (c) 2026 Emir Erbaltacı. Licensed under the MIT License.
 * 
 */

#include "main.h"

int main(void)
{
    MEM_t mem;
    CPU_t cpu;

    if(MEM_Init(&mem) != MEM_OK)
    {
        printf("Error: Memory could not be initialized.");
        return MEM_INIT_FAIL;
    }

    if(CPU_Init(&cpu) != CPU_OK)
    {
        printf("Error: CPU could not be initialized.");
        return CPU_INIT_FAIL;
    }

    if(MEM_PageAlloc(&mem, (MEM_PAGE_COUNT - 1)) != MEM_OK)
    {
        printf("Error: Memory allocation failed for the stack page.");
        return MEM_PAGEALLOC_FAIL;
    }

    while(1)
    {
        getchar();
    }

    return 0;
}
