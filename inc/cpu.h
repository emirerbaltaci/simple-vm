#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include "mem.h"

#define CPU_REG_COUNT 16
#define CPU_REG_FP_COUNT 16

#define CPU_INS_OPCODE_BITS 6
#define CPU_INS_RD_BITS 4
#define CPU_INS_RS1_BITS 4
#define CPU_INS_RS2_BITS 4
#define CPU_INS_IMM_BITS 22

#define CPU_INS_RD_MASK 0xF
#define CPU_INS_RS1_MASK 0xF
#define CPU_INS_RS2_MASK 0xF
#define CPU_INS_IMM_LOW22_MASK 0x003FFFFF
#define CPU_INS_IMM_HIGH10_MASK 0x3FF

#define CPU_INS_OPCODE_OFFSET (32 - CPU_INS_OPCODE_BITS)
#define CPU_INS_RD_OFFSET (CPU_INS_OPCODE_OFFSET - CPU_INS_RD_BITS)
#define CPU_INS_RS1_OFFSET (CPU_INS_RD_OFFSET - CPU_INS_RS1_BITS)
#define CPU_INS_RS2_OFFSET (CPU_INS_RS1_OFFSET - CPU_INS_RS2_BITS)

typedef enum {
    CPU_OK = 0,
    CPU_BUSY,
    CPU_NULL_POINTER,
    CPU_MEM_FETCH_ERROR,
    CPU_INVALID_OPCODE,
    CPU_INVALID_REGISTER,
    CPU_UNKNOWN_ERR,
}CPU_Status_t;

typedef enum {
    NOP = 0,
    MOV,
    MOVI,
    MOVHI,
    OR,
    AND,
    XOR,
    NOT,
    SHL,
    SHR
}CPU_Opcode_t;

typedef struct {
    uint32_t reg[CPU_REG_COUNT];
    uint32_t reg_fp[CPU_REG_FP_COUNT];
    uint32_t pc;
    uint32_t sp;
    uint32_t flags;
    CPU_Status_t status;
}CPU_t;

void CPU_Init(CPU_t* cpu);
void CPU_Reset(CPU_t* cpu);

CPU_Status_t CPU_Step(CPU_t* cpu, MEM_t* mem);

#endif // CPU_H
