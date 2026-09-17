#ifndef MEM_H
#define MEM_H

#include <stdint.h>

#define MEM_PAGE_SIZE 4096 // Bytes
#define MEM_PAGE_COUNT (1U << 20) // 2^20 Pages
#define MEM_PAGE_OFFSET_BITS 12
#define MEM_PAGE_OFFSET_MASK 0xFFF

typedef enum {
    MEM_OK = 0,
    MEM_BUSY,
    MEM_ALLOC_FAIL,
    MEM_ALREADY_FREE,
    MEM_NULL_POINTER,
    MEM_PAGENUMBER_OUTOFRANGE,
    MEM_NEXTPAGENUMBER_OUTOFRANGE,
    MEM_PAGENUMBER_OCCUPIED,
    MEM_ACCESS_TO_FREE_PAGE,
    MEM_NEXTPAGE_NOT_ALLOCATED,
    MEM_UNKNOWN_ERROR
}MEM_Status_t;

typedef struct {
    uint32_t page_num;
    uint8_t* data;
}MEM_Page_t;

typedef struct {
    MEM_Page_t** pageTable;
    MEM_Status_t status;
}MEM_t;

MEM_Status_t MEM_Init(MEM_t* mem);
MEM_Status_t MEM_Free(MEM_t* mem);

MEM_Status_t MEM_PageAlloc(MEM_t* mem, uint32_t pageNum);
MEM_Status_t MEM_FreePage(MEM_t* mem, uint32_t pageNum);

MEM_Status_t MEM_Read8(MEM_t* mem, uint32_t addr, uint8_t* dest);
MEM_Status_t MEM_Read16(MEM_t* mem, uint32_t addr, uint16_t* dest);
MEM_Status_t MEM_Read32(MEM_t* mem, uint32_t addr, uint32_t* dest);

MEM_Status_t MEM_Write8(MEM_t* mem, uint32_t addr, uint8_t val);
MEM_Status_t MEM_Write16(MEM_t* mem, uint32_t addr, uint16_t val);
MEM_Status_t MEM_Write32(MEM_t* mem, uint32_t addr, uint32_t val);

#endif // MEM_H
