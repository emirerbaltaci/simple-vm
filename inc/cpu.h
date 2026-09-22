/**
 * @file cpu.h
 * @author Emir Erbaltacı (erbaltaciemir@hotmail.com)
 * @brief Header File for CPU Properties, Controller Structure, Functions Prototypes and Instruction Set
 * Some properties of the CPU:
 *  1. The CPU has 32-bit architecture. Every instruction is 32 bits long, but some instructions do not utilize all bits.
 *  2. The stack pointer starts from address 0xFFFFFFFC and decrements towards 0.
 *  3. Currently, all instructions have 6-bit OPCODEs, followed by several different bit field configurations depending on the OPCODE.
 * More explanations and details are present below in this file and src/cpu.c, which holds implementation details. 
 * @version 0.1.0
 * @date 21-09-2026
 * 
 * @copyright Copyright (c) 2026 Emir Erbaltacı. Licensed under the MIT License.
 * 
 */

#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include "mem.h" /// Memory Controller

#define CPU_REG_COUNT 16 /// General purpose register count
#define CPU_REG_FP_COUNT 16 /// Floating point register count

/**
 * @brief Instructions come in 3 formats:
 * 1. Register Format: OPCODE (6 bits) | RD (4 bits) | RS1 (4 bits) | RS2 (4 bits) | RESERVED (14 bits)
 *  This format is typically used when any source register (RSx) is utilized to execute the instruction.
 *  Examples are ALU operations (AND, OR, ADD, SUB, CMP etc.) and JMP.
 *  If an instruction in this format does not use a certain register, the value in the corresponding bit field is simply ignored.
 * 2. Immediate Format: OPCODE (6 bits) | RD (4 bits) | IMM (22 bits)
 *  This format is used to generate a value (IMM value) immediately in RD.
 *  Since IMM is only 22 bits, more instuctions are generated to obtain values that require more than 22 bits.
 * 3. OPCODE-only format: OPCODE (6 bits) | Any (26 bits)
 *  This format is mainly used for branching.
 *  For example: After CMP sets the CPU_FLAG_Z flag, JZ will make the PC jump by an offset, stored as signed 26 bits after OPCODE.
 */
#define CPU_INS_OPCODE_BITS 6 /// Opcode bits range from 26 to 31 (inclusive)
#define CPU_INS_RD_BITS 4 /// Destination register bits range from 22 to 25 (inclusive)
#define CPU_INS_RS1_BITS 4 /// Source register 1 bits range from 18 to 21 (inclusive)
#define CPU_INS_RS2_BITS 4 /// Source register 2 bits range from 14 to 17 (inclusive)

#define CPU_INS_IMM_BITS 22 /// Immediate bits range from 0 to 21 (inclusive)

#define CPU_INS_RD_MASK 0xF
#define CPU_INS_RS1_MASK 0xF
#define CPU_INS_RS2_MASK 0xF

#define CPU_INS_IMM_LOW22_MASK 0x003FFFFF
#define CPU_INS_IMM_HIGH10_MASK 0x3FF

#define CPU_INS_OPCODE_OFFSET (32 - CPU_INS_OPCODE_BITS)
#define CPU_INS_RD_OFFSET (CPU_INS_OPCODE_OFFSET - CPU_INS_RD_BITS)
#define CPU_INS_RS1_OFFSET (CPU_INS_RD_OFFSET - CPU_INS_RS1_BITS)
#define CPU_INS_RS2_OFFSET (CPU_INS_RS1_OFFSET - CPU_INS_RS2_BITS)

/**
 * @brief ALU Flags
 * Following flags are reset before any ALU operation.
 * They are set if the corresponding conditions in the comments are met.
 */
#define CPU_FLAG_Z (1U << 0)    /// Set if result is 0
#define CPU_FLAG_N (1U << 1)    /// Set if result is negative
#define CPU_FLAG_C (1U << 2)    /// ADD: Set if unsigned overflow happened, SUB/CMP: Set if no borrow was required
#define CPU_FLAG_V (1U << 3)    /// Set if signed overflow happened

/**
 * @brief CPU State Enumeration
 * ALL CPU API functions return one of these values, useful for error handling and debugging. 
 */
typedef enum {
    CPU_OK = 0,
    CPU_BUSY,
    CPU_NULL_POINTER,
    CPU_MEM_FETCH_ERROR,
    CPU_MEM_STORE_ERROR,
    CPU_INVALID_OPCODE,
    CPU_INVALID_REGISTER,
    CPU_EMPTY_STACK_POP,
    CPU_EMPTY_STACK_RETURN,
    CPU_UNKNOWN_ERR,
}CPU_Status_t;

/**
 * @brief Instruction Set Enumeration
 * Since OPCODE is 6 bits, 64 unique instructions are possible.
 * For the comments below, which describe the OPCODEs:
 *  RD, RS1, RS2, SP, PC, etc. -> Represents the registers
 *  *RD, *RS1, *RS2, *SP, *PC, etc. -> Represents the value stored in the register with the same name
 *  **SP, **RS1, etc. -> Represents the value which the address stored in the register points to, implying a pointer stored in the register
 * Shortly, dereferencing is used here.
 * @note IMM is a value stored in the instruction, not a register.
 */
typedef enum {
    NOP = 0,    /// No Operation
    MOV,        /// Copy *RS1 to RD
    MOVI,       /// Generate value of IMM (22 bits) in lower bits of RD (0-21), reset higher bits 
    MOVHI,      /// Generate value of IMM (10 bits) in higher bits of RD (22-31), reset lower bits
    OR,         /// Perform a bitwise OR on *RS1 and *RS2, store the result in RD
    AND,        /// Perform a bitwise AND on *RS1 and *RS2, store the result in RD
    XOR,        /// Perform a bitwise XOR on *RS1 and *RS2, store the result in RD
    NOT,        /// Perform a bitwise NOT on *RS1, store the result in RD
    SHL,        /// Perform a bitwise left shift on *RS1 by amount *RS2, store the result in RD
    SHR,        /// Perform a bitwise right shift on *RS1 by amount *RS2, store the result in RD
    ADD,        /// Add *RS1 and *RS2, update ALU flags, store the result in RD (Integer addition)
    SUB,        /// Subtract *RS2 from *RS1, update ALU flags, store the result in RD (Integer subtraction)
    CMP,        /// Compare: Similar to SUB, but does not store in RD. Integers are compared via ALU flags.
    JZ,         /** If CPU_FLAG_Z is set, jump to another instruction located at an offset (lower 26 bits of the instruction, signed)
                    Note that program counter is incremented by 4 BEFORE execution, thus JZ with an offset of 0 results in next instruction.
                    Also, offset is by word/instruction (4 bytes), not by byte. Offset value of 1 will increment PC by another 4. 
                    These facts apply to all 3 Jx/Jxx operations below*/
    JNZ,        /// Similar to JZ, but executes if CPU_FLAG_Z is reset
    JC,         /// Similar to JZ, but executes if CPU_FLAG_C is set
    JNC,        /// Similar to JZ, but executes if CPU_FLAG_C is reset
    JMP,        /// Copy *RS1 to PC. RS1 holds absolute address instead of an offset, in contrast to all 4 Jx/Jxx operations above
    PUSH,       /// Decrement SP, then copy *RS1 to *SP.
    POP,        /// Copy **SP to RD, then increment the stack pointer.
    CALL,       /// Decrement SP, copy *PC to *SP, and copy *RS1 to PC
    RET,        /// Copy **SP to PC, then increment the stack pointer.
}CPU_Opcode_t;

/// @brief CPU Controller Structure
typedef struct {
    uint32_t reg[CPU_REG_COUNT]; /// General purpose registers
    uint32_t reg_fp[CPU_REG_FP_COUNT]; /// Floating point registers
    uint32_t pc; /// Program Counter, initially 0
    uint32_t sp; /// Stack Pointer, initially 0xFFFFFFFC
    uint32_t flags; /// CPU operation flags
    CPU_Status_t status; /// CPU response to last operation
}CPU_t;

/**
 * @brief CPU Controller Initializer
 * 
 * @param[in,out] cpu > Pointer to CPU controller structure 
 * 
 * @retval CPU_OK > Controller initialization was successful.
 * @retval CPU_NULL_POINTER > The provided pointer (cpu) is NULL.
 */
CPU_Status_t CPU_Init(CPU_t* cpu);

/**
 * @brief CPU Main Lifecycle Function
 * Fetch, decode, execute, store (if needed).
 * 
 * @param cpu > Pointer to CPU controller structure
 * @param mem > Pointer to memory controller structure
 * 
 * @retval CPU_OK > Cycle completed successfully. 
 * @retval CPU_NULL_POINTER > Any provided pointer (cpu, mem) is NULL. If not (unlikely), something is wrong with the implementation.
 * @retval CPU_MEM_FETCH_ERROR > Read operation to memory failed. Check if the corresponding page is allocated.
 * @retval CPU_MEM_STORE_ERROR > Write operation to memory failed. Check if the corresponding page is allocated.
 * @retval CPU_EMPTY_STACK_POP > POP cannot be executed when the stack pointer is pointing 0xFFFFFFFC, nothing to pop.
 * @retval CPU_EMPTY_STACK_RETURN > RET cannot be executed when the stack pointer is pointing 0xFFFFFFFC, nothing to return.
 * @retval CPU_INVALID_OPCODE > The instruction has an undefined OPCODE. Check the number and use enumerated values when constructing instructions.
 * @retval CPU_INVALID_REGISTER > Instruction contains register number that is out of range. If using default register count (16), this can't be returned.
 */
CPU_Status_t CPU_Step(CPU_t* cpu, MEM_t* mem);

#endif // CPU_H
