#include "cpu.h"

void CPU_Init(CPU_t* cpu)
{
    if(cpu == NULL)
        return;

    cpu->status = CPU_BUSY;

    for(int i = 0; i < CPU_REG_COUNT; i++) cpu->reg[i] = 0;
    for(int i = 0; i < CPU_REG_FP_COUNT; i++) cpu->reg_fp[i] = 0;
    cpu->flags = 0;
    cpu->pc = 0;
    cpu->sp = 0;

    cpu->status = CPU_OK;
}

void CPU_Reset(CPU_t* cpu)
{
    CPU_Init(cpu);
}

static inline CPU_Status_t CPU_DecodeRegisterFormat(uint32_t* ins, uint32_t* pRd, uint32_t* pRs1, uint32_t* pRs2)
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

CPU_Status_t CPU_Step(CPU_t* cpu, MEM_t* mem)
{
    if(cpu == NULL)
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

        default:
            cpu->status = CPU_INVALID_OPCODE;
            return CPU_INVALID_OPCODE;
    }

    cpu->status = cpuRet;
    return cpuRet;
}
