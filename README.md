# Simple VM

>**Version**: 0.1.0 - Initial Commit  
>**Status**: In development, far from complete to be honest

A simple virtual machine in development. Written fully in C.  
I wanted to practice C and low level programming, so I came up with this project.  
This README file will be expanded as the project goes on. If the project grows substantially, I will create other markdown files or any other documentation as well. Also, I'm trying to make comments in codes clean and descriptive.

## Overview
Main goal in this project to build a virtual computer architecture with fundamentals completely implemented. In the end, at least these should be implemented:
* CPU Controller
* Memory Controller with Multi-level Paging
* Instruction Set
  * Basic Register and Bitwise operations
  * ALU and FPU operations
  * Stack and Heap operations
  * Branching operations
  * Function calls
* A simple compiler for this architecture

Currently, basic CPU and memory controllers are implemented. Memory controller has single-level paging now, multi-level paging will be implemented in near future. Both CPU and memory controllers will have more features in the future. The instruction set is in active development and implemented to a certain extent. The compiler will be developed last, I haven't started yet.

## Architecture
The CPU has 32-bit architecture. All instructions are fixed to 32-bit in length. Some instructions do not utilize all bits, which are ignored when executing those instructions. All instructions have fixed 6-bit OPCODEs in higher bits (26-31), resulting in 64 possible unique instructions. As we have 32-bit architecture, address space is 4 GiB. Also, both general purpose and floating point registers are 32-bit wide, and the CPU has 16 each. The architecture is little endian.  
Here is a summary table:

| Property | Default Value |
| -------- | ----- |
| Architecture | 32-bit |
| Instruction Size | 32 bits |
| Possible Instructions | 64 |
| OPCODE Length | 6 bits |
| Endianness | Little-endian |
| General Purpose Registers | 16 |
| Floating Point Registers | 16 |
| Register Width (All) | 32 bits |
| Address Space | 4 GiB (32-bit address size) |

## CPU
### CPU Controller Structure
The CPU is represented by the ```CPU_t``` structure, located in [inc/cpu.h][inc/cpu.h].
```c  
/// @brief CPU Controller Structure
typedef struct {  
    uint32_t reg[CPU_REG_COUNT]; /// General purpose registers  
    uint32_t reg_fp[CPU_REG_FP_COUNT]; /// Floating point registers  
    uint32_t pc; /// Program Counter, initially 0  
    uint32_t sp; /// Stack Pointer, initially 0xFFFFFFFC  
    uint32_t flags; /// CPU operation flags  
    CPU_Status_t status; /// CPU response to last operation  
}CPU_t;
```
```CPU_REG_COUNT``` and ```CPU_REG_FP_COUNT``` are configuration macros defined in the same file. By default, they are both 16. It may be decreased, implementation takes invalid register access possibility to account. But to increase it, one needs to modify the instructions as register bit fields are 4 bits long.  
### CPU Status and API Return Values
Note that CPU controller holds the response to last operation. ```CPU_Status_t``` is an enumeration of possible responses. All CPU API functions return ```CPU_Status_t```, which is helpful for error handling and debugging. The enumeration is implemented in [inc/cpu.h][inc/cpu.h] tabulated below.

| Response | Value | Description |
| -------- | ----- | ----------- |
| CPU_OK | 0 | Function did not encounter any problems, task completed successfully. |
| CPU_BUSY | 1 | Used for updating the status of the CPU controller. Not returned by any function as of version 0.1.0. | 
| CPU_NULL_POINTER | 2 | Arguments contain a ```NULL``` pointer. Note that the parameter might be a structure containing a ```NULL``` pointer. | 
| CPU_MEM_FETCH_ERROR | 3 | ```MEM_ReadX``` failed where ```X``` denotes 8, 16 or 32 (bytes). Check memory controller status for more context. |
| CPU_MEM_STORE_ERROR | 4 | ```MEM_WriteX``` failed where ```X``` denotes 8, 16 or 32 (bytes). Check memory controller status for more context. |
| CPU_INVALID_OPCODE | 5 | Instruction contains OPCODE that is invalid. Check if the OPCODE corresponds to an enumerated value. |
| CPU_INVALID_REGISTER | 6 | Instruction parameters target a register number that is out of range. Can't be returned when register count and instruction bit fields for registers are default since they are 16 and 4 respectively. |
| CPU_EMPTY_STACK_POP | 7 | ```POP``` while the stack is empty and the stack pointer targets ```0xFFFFFFFC```, which is the initial value, cannot be executed. |
| CPU_EMPTY_STACK_RETURN | 8 | ```RET``` while the stack is empty and the stack pointer targets ```0xFFFFFFFC```, which is the initial value, cannot be executed. |
| CPU_UNKNOWN_ERR | 9 | CPU encountered an unidentified error. |

Note that a function may or may not be able to return all these values. For example, ```CPU_Init``` function can only return ```CPU_OK``` or ```CPU_NULL_POINTER``` while ```CPU_Step``` function can return all enumerated values (except ```CPU_BUSY```) as of version 0.1.0.  

### CPU Lifecycle
The CPU lifecycle: Fetch -> Decode -> Execute -> Store (if needed)  
One cycle is completed in each call to ```CPU_Step``` function, implemented in [src/cpu.c][src/cpu.c]:

```c  
CPU_Status_t CPU_Step(CPU_t* cpu, MEM_t* mem);  
```

Where ```mem``` is a pointer to the memory controller structure. 

### CPU Flags
Lastly, CPU has a 32-bit flag register. As of v0.1.0, only 4 ALU flags are implemented. Masks for flags are defined in [inc/cpu.h][inc/cpu.h].
| Flag | Bit Number | Description |
| ---- | ---------- | ----------- |
| Z | 0 | ALU Flag: Operation resulted in 0. |
| N | 1 | ALU Flag: Operation resulted in a negative integer. |
| C | 2 | ALU Flag: If ```ADD``` is executed, operation resulted in an unsigned overflow. If ```SUB``` or ```CMP``` is executed, no borrow was required in the operation. |
| V | 3 | ALU Flag: Operation resulted in a signed overflow. |

## Memory
The virtual machine uses 32-bit address space. Currently, a single-level paged memory structure is present. Page size and maximum count are fixed to 4096 bytes and $2^{20}$ respectively. When the memory controller is initialized, no pages are allocated, but 8 MiB is spent for holding the pointers of the page table, all ```NULL``` initially. It's not a good design, especially when multiple memory controllers are used, so will be changed.

### Memory Page and Controller Structure

The structures defining a memory controller are implemented in [inc/mem.h].

```c
/// @brief Memory page structure.
typedef struct {
    uint32_t page_num; /// Page number of the memory page itself.
    uint8_t* data; /// Pointer to the byte array of size MEM_PAGE_SIZE.
}MEM_Page_t;

/// @brief Memory controller structure.
typedef struct {
    MEM_Page_t** pageTable; /// Pointer to page pointers. Will be replaced by multi-level paging in near future.
    MEM_Status_t status; /// Memory controller status arising from the last operation.
}MEM_t;
```
### Memory Controller Status and API Return Values

The memory controller API provides functions for initialization/deinitialization, page allocation/deallocation, 8/16/32 read/write operations, and conversion of page number and offset values into absolute address value. All API functions except ```MEM_GetAddr``` return ```MEM_Status_t```, similar to the CPU controller which returns ```CPU_Status_t```. Enumeration is present in [inc/mem.h] and is tabulated below:

| Response | Value | Description |
|---|---|---|
| MEM_OK | 0 | The operation was successful. |
| MEM_BUSY | 1 | Used for updating the status of the memory controller. Not returned by any function as of version 0.1.0. |
| MEM_NULL_POINTER | 2 | Arguments contain a ```NULL``` pointer. Note that the parameter might be a structure containing a ```NULL``` pointer. |
| MEM_ALLOC_FAIL | 3 | Memory allocation on the host computer failed. Check memory usage. |
| MEM_ALREADY_FREE | 4 | Function tried to free a page pointer that was already ```NULL```. |
| MEM_PAGENUMBER_OUTOFRANGE | 5 | Specified page number is greater than ```(MEM_PAGE_COUNT - 1)```. |
| MEM_NEXTPAGENUMBER_OUTOFRANGE | 6 | 16/32 bits read/write with very high offset value caused a cross-page read/write, but the next page number is greater than ```(MEM_PAGE_COUNT - 1)```, surpassing memory limits. |
| MEM_NEXTPAGE_NOT_ALLOCATED | 7 | 16/32 bits read/write with very high offset value caused a cross-page read/write, but the next page is not allocated. |
| MEM_PAGENUMBER_OCCUPIED | 8 | Function tried to allocate memory for a page that has already been allocated. |
| MEM_ACCESS_TO_FREE_PAGE | 9 | Function tried read/write operation on a free page. |
| MEM_UNKNOWN_ERROR | 10 | Memory controller encountered an unidentified error. |

In [inc/mem.h]; functions for read, write and conversion have the following declarations:

```c
MEM_Status_t MEM_ReadX(MEM_t* mem, uint32_t addr, uintX_t* dest);
MEM_Status_t MEM_WriteX(MEM_t* mem, uint32_t addr, uintX_t val);
inline uint32_t MEM_GetAddr(uint32_t pageNum, uint32_t offset);
```

Where ```X``` in function name and parameters denote the size in bits, replaced by 8/16/32 as such: ```MEM_Read16``` and ```uint16_t* dest```.  On the other hand, ```MEM_GetAddr``` returns absolute address from specified page number and offset.  

# Future of the Project
### Documentation
At the beginning, I was going to build the project first and then write the documentation since this has been a small solo project so far. But writing a documentation is actually fun and I decided to implement the project and documentation simultaneously, it may help growing the project to a bigger extent. Since documentation lags a little behind because of my initial thought, I am working mostly on documentation now. When it catches up, I will speed up coding again.  
I will create a ```doc/``` folder where I will have detailed documentation.
### Implementation
Instruction set is far from complete. This is my first priority. Then, I will have multi-level paging, memory map and file I/O operations. After that, I will probably need a small compiler. 

# License
Copyright (c) Emir Erbaltacı  
This project is licensed under the MIT License.  
(Honestly, I did not think much when choosing it. Anyone is free to do anything with this code.)

[inc/cpu.h]:https://github.com/emirerbaltaci/simple-vm/blob/main/inc/cpu.h
[src/cpu.c]:https://github.com/emirerbaltaci/simple-vm/blob/main/src/cpu.c
[inc/mem.h]:https://github.com/emirerbaltaci/simple-vm/blob/main/inc/mem.h
[src/mem.h]:https://github.com/emirerbaltaci/simple-vm/blob/main/src/mem.c
