/**
 * @file cpu.c
 * @author Emir Erbaltacı (erbaltaciemir@hotmail.com)
 * @brief Implementation File for CPU Functions and Instruction Set
 * @version 0.1.0
 * @date 21-09-2026
 * 
 * @copyright Copyright (c) 2026 Emir Erbaltacı. Licensed under the MIT License.
 * 
 */

#include "cpu.h"

CPU_Status_t CPU_Init(CPU_t* cpu)
{
    if(cpu == NULL)
        return CPU_NULL_POINTER;

    cpu->status = CPU_BUSY;

    for(int i = 0; i < CPU_REG_COUNT; i++) cpu->reg[i] = 0;
    for(int i = 0; i < CPU_REG_FP_COUNT; i++) cpu->reg_fp[i] = 0;
    cpu->flags = 0;
    cpu->pc = 0;
    cpu->sp = 0xFFFFFFFCU;

    cpu->status = CPU_OK;
    return CPU_OK;
}

static inline CPU_Status_t CPU_DecodeRegisterFormat(const uint32_t* ins, uint32_t* pRd, uint32_t* pRs1, uint32_t* pRs2)
{
    if(ins == NULL || pRd == NULL || pRs1 == NULL || pRs2 == NULL)
        return CPU_NULL_POINTER;

    *pRd = (*ins >> CPU_INS_RD_OFFSET) & CPU_INS_RD_MASK;
    *pRs1 = (*ins >> CPU_INS_RS1_OFFSET) & CPU_INS_RS1_MASK;
    *pRs2 = (*ins >> CPU_INS_RS2_OFFSET) & CPU_INS_RS2_MASK;

    if(*pRd >= CPU_REG_COUNT || *pRs1 >= CPU_REG_COUNT || *pRs2 >= CPU_REG_COUNT)
        return CPU_INVALID_REGISTER;

    return CPU_OK;
}

static inline void CPU_ResetALUFlags(CPU_t* cpu)
{
    cpu->flags &= ~(CPU_FLAG_Z | CPU_FLAG_N | CPU_FLAG_C | CPU_FLAG_V);
}

static inline void CPU_HandleBranchOffset(CPU_t* cpu, uint32_t ins)
{
    int32_t offset = (int32_t)(ins & 0x03FFFFFFU);

    if(offset & 0x02000000)
        offset |= (int32_t)0xFC000000U;
    
    cpu->pc += ((uint32_t)offset << 2);
}

CPU_Status_t CPU_Step(CPU_t* cpu, MEM_t* mem)
{
    if(cpu == NULL || mem == NULL)
        return CPU_NULL_POINTER;
    
    cpu->status = CPU_BUSY;

    uint32_t ins, opcode, rd, rs1, rs2, imm;
    CPU_Status_t cpuRet = CPU_OK;

    MEM_Status_t memRet = MEM_Read32(mem, cpu->pc, &ins);
    if(memRet != MEM_OK)
    {
        cpu->status = CPU_MEM_FETCH_ERROR;
        return CPU_MEM_FETCH_ERROR;
    }

    opcode = ins >> CPU_INS_OPCODE_OFFSET;
    
    cpu->pc += 4;

    switch (opcode)
    {
        case NOP:
            break;
        
        case MOV:
            cpuRet = CPU_DecodeRegisterFormat(&ins, &rd, &rs1, &rs2);
            if(cpuRet != CPU_OK) 
                break;

            cpu->reg[rd] = cpu->reg[rs1];

            break;
        
        case MOVI:
            rd = (ins >> CPU_INS_RD_OFFSET) & CPU_INS_RD_MASK;
            imm = ins & CPU_INS_IMM_LOW22_MASK;
        
            cpu->reg[rd] = imm;

            break;

        case MOVHI:
            rd = (ins >> CPU_INS_RD_OFFSET) & CPU_INS_RD_MASK;
            imm = ins & CPU_INS_IMM_HIGH10_MASK;

            cpu->reg[rd] = imm << CPU_INS_IMM_BITS;

            break;

        case OR:
            cpuRet = CPU_DecodeRegisterFormat(&ins, &rd, &rs1, &rs2);
            if(cpuRet != CPU_OK) 
                break;

            cpu->reg[rd] = cpu->reg[rs1] | cpu->reg[rs2];
        
            break;
        
        case AND:
            cpuRet = CPU_DecodeRegisterFormat(&ins, &rd, &rs1, &rs2);
            if(cpuRet != CPU_OK) 
                break;

            cpu->reg[rd] = cpu->reg[rs1] & cpu->reg[rs2];

            break;

        case XOR:
            cpuRet = CPU_DecodeRegisterFormat(&ins, &rd, &rs1, &rs2);
            if(cpuRet != CPU_OK) 
                break;

            cpu->reg[rd] = cpu->reg[rs1] ^ cpu->reg[rs2];

            break;

        case NOT:
            cpuRet = CPU_DecodeRegisterFormat(&ins, &rd, &rs1, &rs2);
            if(cpuRet != CPU_OK) 
                break;

            cpu->reg[rd] = ~cpu->reg[rs1];

            break;

        case SHL:
            cpuRet = CPU_DecodeRegisterFormat(&ins, &rd, &rs1, &rs2);
            if(cpuRet != CPU_OK) 
                break;

            cpu->reg[rd] = cpu->reg[rs1] << (cpu->reg[rs2] & 0x1F);

            break;

        case SHR:
            cpuRet = CPU_DecodeRegisterFormat(&ins, &rd, &rs1, &rs2);
            if(cpuRet != CPU_OK) 
                break;

            cpu->reg[rd] = cpu->reg[rs1] >> (cpu->reg[rs2] & 0x1F);

            break;
        
        case ADD:
        {
            cpuRet = CPU_DecodeRegisterFormat(&ins, &rd, &rs1, &rs2);
            if(cpuRet != CPU_OK)
                break;

            CPU_ResetALUFlags(cpu);

            uint32_t res = cpu->reg[rs1] + cpu->reg[rs2];

            if(res == 0)
                cpu->flags |= CPU_FLAG_Z;
            if(res & 0x80000000U)
                cpu->flags |= CPU_FLAG_N;
            if(res < cpu->reg[rs1])
                cpu->flags |= CPU_FLAG_C;
            if( (cpu->reg[rs1] < 0x80000000U && cpu->reg[rs2] < 0x80000000U && res >= 0x80000000U)
                || (cpu->reg[rs1] >= 0x80000000U && cpu->reg[rs2] >= 0x80000000U && res < 0x80000000U) )
                cpu->flags |= CPU_FLAG_V;
            
            cpu->reg[rd] = res;

            break;
        }

        case SUB:
        {
            cpuRet = CPU_DecodeRegisterFormat(&ins, &rd, &rs1, &rs2);
            if(cpuRet != CPU_OK)
                break;
            
            CPU_ResetALUFlags(cpu);
            
            uint32_t res = cpu->reg[rs1] - cpu->reg[rs2];
            
            if(res == 0)
                cpu->flags |= CPU_FLAG_Z;
            if(res & 0x80000000U)
                cpu->flags |= CPU_FLAG_N;
            if(cpu->reg[rs1] >= cpu->reg[rs2])
                cpu->flags |= CPU_FLAG_C;
            if( (cpu->reg[rs1] < 0x80000000U && cpu->reg[rs2] >= 0x80000000U && res >= 0x80000000U) 
                || (cpu->reg[rs1] >= 0x80000000U && cpu->reg[rs2] < 0x80000000U && res < 0x80000000U) )
                cpu->flags |= CPU_FLAG_V;

            cpu->reg[rd] = res;

            break;
        }

        case CMP:
        {
            cpuRet = CPU_DecodeRegisterFormat(&ins, &rd, &rs1, &rs2);
            if(cpuRet != CPU_OK)
                break;
            
            CPU_ResetALUFlags(cpu);

            uint32_t res = cpu->reg[rs1] - cpu->reg[rs2];

            if(res == 0)
                cpu->flags |= CPU_FLAG_Z;
            if(res & 0x80000000U)
                cpu->flags |= CPU_FLAG_N;
            if(cpu->reg[rs1] >= cpu->reg[rs2])
                cpu->flags |= CPU_FLAG_C;
            if( (cpu->reg[rs1] < 0x80000000U && cpu->reg[rs2] >= 0x80000000U && res >= 0x80000000U) 
                || (cpu->reg[rs1] >= 0x80000000U && cpu->reg[rs2] < 0x80000000U && res < 0x80000000U) )
                cpu->flags |= CPU_FLAG_V;
            
            break;
        }

        case JZ:
            if(cpu->flags & CPU_FLAG_Z)
                CPU_HandleBranchOffset(cpu, ins);

            break;
        
        case JNZ:
            if(!(cpu->flags & CPU_FLAG_Z))
                CPU_HandleBranchOffset(cpu, ins);

            break;

        case JC:
            if(cpu->flags & CPU_FLAG_C)
                CPU_HandleBranchOffset(cpu, ins);

            break;
        
        case JNC:
            if(!(cpu->flags & CPU_FLAG_C))
                CPU_HandleBranchOffset(cpu, ins);

            break;

        case JMP:
            cpuRet = CPU_DecodeRegisterFormat(&ins, &rd, &rs1, &rs2);
            if(cpuRet != CPU_OK)
                break;
            
            cpu->pc = cpu->reg[rs1];
            
            break;
        
        case PUSH:
            cpuRet = CPU_DecodeRegisterFormat(&ins, &rd, &rs1, &rs2);
            if(cpuRet != CPU_OK)
                break;
            
            cpu->sp -= 4;
            memRet = MEM_Write32(mem, cpu->sp, cpu->reg[rs1]);
            if(memRet != MEM_OK)
            {
                cpu->sp += 4;
                cpu->status = CPU_MEM_STORE_ERROR;
                return CPU_MEM_STORE_ERROR;
            }

            break;

        case POP:
            if(cpu->sp <= 0xFFFFFFF8U)
            {
                cpuRet = CPU_DecodeRegisterFormat(&ins, &rd, &rs1, &rs2);
                if(cpuRet != CPU_OK)
                    break;
            
                memRet = MEM_Read32(mem, cpu->sp, &cpu->reg[rd]);
                if(memRet != MEM_OK)
                {
                    cpu->status = CPU_MEM_FETCH_ERROR;
                    return CPU_MEM_FETCH_ERROR;
                }
                cpu->sp += 4;
            }
            else
            {
                cpu->status = CPU_EMPTY_STACK_POP;
                return CPU_EMPTY_STACK_POP;
            }

            break;

        case CALL:
            cpuRet = CPU_DecodeRegisterFormat(&ins, &rd, &rs1, &rs2);
            if(cpuRet != CPU_OK)
                break;
            
            cpu->sp -= 4;
            memRet = MEM_Write32(mem, cpu->sp, cpu->pc);
            if(memRet != MEM_OK)
            {
                cpu->sp += 4;
                cpu->status = CPU_MEM_STORE_ERROR;
                return CPU_MEM_STORE_ERROR;
            }

            cpu->pc = cpu->reg[rs1];
            
            break;

        case RET:
            
            if(cpu->sp <= 0xFFFFFFF8U)
            {
                memRet = MEM_Read32(mem, cpu->sp, &cpu->pc);
                if(memRet != MEM_OK)
                {
                    cpu->status = CPU_MEM_FETCH_ERROR;
                    return CPU_MEM_FETCH_ERROR;
                }
                cpu->sp += 4;
            }
            else
            {
                cpu->status = CPU_EMPTY_STACK_RETURN;
                return CPU_EMPTY_STACK_RETURN;
            }
            
            break;

        default:
            cpu->status = CPU_INVALID_OPCODE;
            return CPU_INVALID_OPCODE;
    }

    cpu->status = cpuRet;
    return cpuRet;
}
