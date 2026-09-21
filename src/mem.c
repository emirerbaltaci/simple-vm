/**
 * @file mem.c
 * @author Emir Erbaltacı (erbaltaciemir@hotmail.com)
 * @brief Implementation File for Memory Functions
 * @version 0.1.0
 * @date 21-09-2026
 * 
 * @copyright Copyright (c) 2026 Emir Erbaltacı. Licensed under the MIT License.
 * 
 */

#include "mem.h"
#include <stdlib.h>
#include <stdbool.h>

MEM_Status_t MEM_Init(MEM_t* mem)
{
    if(mem == NULL)
        return MEM_NULL_POINTER;

    mem->status = MEM_BUSY;

    mem->pageTable = (MEM_Page_t**)calloc(MEM_PAGE_COUNT, sizeof(MEM_Page_t*));

    if(mem->pageTable == NULL)
    {
        mem->status = MEM_ALLOC_FAIL;
        return MEM_ALLOC_FAIL;
    }

    mem->status = MEM_OK;
    return MEM_OK;
}

MEM_Status_t MEM_Free(MEM_t* mem)
{
    if(mem == NULL)
        return MEM_NULL_POINTER;
    
    mem->status = MEM_BUSY;
    
    if(mem->pageTable == NULL)
    {
        mem->status = MEM_NULL_POINTER;
        return MEM_NULL_POINTER;
    }
    
    MEM_Status_t ret = MEM_OK;
    
    for(uint32_t i = 0; i < MEM_PAGE_COUNT; i++)
    {
        if(mem->pageTable[i] != NULL) 
        {
            ret = MEM_FreePage(mem, i);
            if(ret != MEM_OK)
            {
                mem->status = ret;
                return ret;
            }
        }
    }

    free(mem->pageTable);
    mem->pageTable = NULL;

    mem->status = MEM_OK;
    return MEM_OK;
}

MEM_Status_t MEM_PageAlloc(MEM_t* mem, uint32_t pageNum)
{
    if(mem == NULL)
        return MEM_NULL_POINTER;
    
    mem->status = MEM_BUSY;

    if(pageNum >= MEM_PAGE_COUNT)
    {
        mem->status = MEM_PAGENUMBER_OUTOFRANGE;
        return MEM_PAGENUMBER_OUTOFRANGE;
    }

    if(mem->pageTable[pageNum] != NULL)
    {
        mem->status = MEM_PAGENUMBER_OCCUPIED;
        return MEM_PAGENUMBER_OCCUPIED;
    }

    MEM_Page_t* page = (MEM_Page_t*)malloc(sizeof(MEM_Page_t));

    if(page == NULL)
    {
        mem->status = MEM_ALLOC_FAIL;
        return MEM_ALLOC_FAIL;
    }

    page->page_num = pageNum;
    page->data = (uint8_t*)calloc(MEM_PAGE_SIZE, sizeof(uint8_t));

    if(page->data == NULL)
    {
        free(page);
        mem->status = MEM_ALLOC_FAIL;
        return MEM_ALLOC_FAIL;
    }

    mem->pageTable[pageNum] = page;

    mem->status = MEM_OK;
    return MEM_OK;
}

MEM_Status_t MEM_FreePage(MEM_t* mem, uint32_t pageNum)
{

    if(mem == NULL)
        return MEM_NULL_POINTER;
    
    mem->status = MEM_BUSY;

    if(mem->pageTable == NULL)
    {
        mem->status = MEM_NULL_POINTER;
        return MEM_NULL_POINTER;
    }

    if(pageNum >= MEM_PAGE_COUNT)
    {
        mem->status = MEM_PAGENUMBER_OUTOFRANGE;
        return MEM_PAGENUMBER_OUTOFRANGE;
    }
    
    if(mem->pageTable[pageNum] == NULL)
    {
        mem->status = MEM_ALREADY_FREE;
        return MEM_ALREADY_FREE;
    }
    

    free(mem->pageTable[pageNum]->data);
    free(mem->pageTable[pageNum]);
    mem->pageTable[pageNum] = NULL;

    mem->status = MEM_OK;
    return MEM_OK;
}

static inline MEM_Status_t MEM_RWNullPtr_Control(bool isReadOperation, MEM_t* mem, void* ptr)
{
    if(mem == NULL)
        return MEM_NULL_POINTER;

    if(mem->pageTable == NULL)
        return MEM_NULL_POINTER;

    if(isReadOperation)
    {
        if(ptr == NULL)
            return MEM_NULL_POINTER;
    }

    return MEM_OK;
}

static MEM_Status_t MEM_RWCommon(bool isReadOperation, MEM_t* mem, uint32_t addr, uint32_t* pPageNum, uint32_t* pOffset, void* pDest)
{
    if(pPageNum == NULL || pOffset == NULL)
        return MEM_NULL_POINTER;
    
    MEM_Status_t check = MEM_RWNullPtr_Control(isReadOperation, mem, pDest);
    if(check != MEM_OK)
        return check;
    
    mem->status = MEM_BUSY;
    
    *pPageNum = addr >> MEM_PAGE_OFFSET_BITS;
    *pOffset = addr & MEM_PAGE_OFFSET_MASK;

    if(*pPageNum >= MEM_PAGE_COUNT)
        return MEM_PAGENUMBER_OUTOFRANGE;

    if(mem->pageTable[*pPageNum] == NULL)
        return MEM_ACCESS_TO_FREE_PAGE;

    return MEM_OK;
}

MEM_Status_t MEM_Read8(MEM_t* mem, uint32_t addr, uint8_t* dest)
{
    uint32_t pageNum, offset;
    MEM_Status_t ret = MEM_RWCommon(true, mem, addr, &pageNum, &offset, (void*)dest);
    if(ret != MEM_OK)
    {
        if (mem != NULL) 
            mem->status = ret;
        return ret;
    }

    *dest = mem->pageTable[pageNum]->data[offset];

    mem->status = MEM_OK;
    return MEM_OK;
}

MEM_Status_t MEM_Read16(MEM_t* mem, uint32_t addr, uint16_t* dest)
{
    uint32_t pageNum, offset;
    MEM_Status_t ret = MEM_RWCommon(true, mem, addr, &pageNum, &offset, (void*)dest);
    if(ret != MEM_OK)
    {
        if(mem != NULL) 
            mem->status = ret;
        return ret;
    }

    if((offset + 1) == MEM_PAGE_SIZE)
    {
        uint32_t nextPageNum = pageNum + 1;
        if(nextPageNum == MEM_PAGE_COUNT)
        {
            mem->status = MEM_NEXTPAGENUMBER_OUTOFRANGE;
            return MEM_NEXTPAGENUMBER_OUTOFRANGE;
        }
        if(mem->pageTable[nextPageNum] == NULL)
        {
            mem->status = MEM_NEXTPAGE_NOT_ALLOCATED;
            return MEM_NEXTPAGE_NOT_ALLOCATED;
        }

        *dest = mem->pageTable[pageNum]->data[offset]
                | ((uint16_t)mem->pageTable[nextPageNum]->data[0] << 8);

        mem->status = MEM_OK;
        return MEM_OK;
    }

    *dest = mem->pageTable[pageNum]->data[offset]
            | ((uint16_t)mem->pageTable[pageNum]->data[offset + 1] << 8);
    
    mem->status = MEM_OK;
    return MEM_OK;
}

MEM_Status_t MEM_Read32(MEM_t* mem, uint32_t addr, uint32_t* dest)
{
    uint32_t pageNum, offset, temp;
    MEM_Status_t ret = MEM_RWCommon(true, mem, addr, &pageNum, &offset, (void*)dest);
    if(ret != MEM_OK)
    {
        if(mem != NULL) 
            mem->status = ret;
        return ret;
    }

    temp = (MEM_PAGE_SIZE - 1) - offset;
    if(temp >= 3)
    {
        *dest = mem->pageTable[pageNum]->data[offset]
                | ((uint32_t)mem->pageTable[pageNum]->data[offset + 1] << 8)
                | ((uint32_t)mem->pageTable[pageNum]->data[offset + 2] << 16)
                | ((uint32_t)mem->pageTable[pageNum]->data[offset + 3] << 24);
        
        mem->status = MEM_OK;
        return MEM_OK;
    }
    else
    {
        uint32_t nextPageNum = pageNum + 1;
        if(nextPageNum == MEM_PAGE_COUNT)
        {
            mem->status = MEM_PAGENUMBER_OUTOFRANGE;
            return MEM_PAGENUMBER_OUTOFRANGE;
        }
        if(mem->pageTable[nextPageNum] == NULL)
        {
            mem->status = MEM_NEXTPAGE_NOT_ALLOCATED;
            return MEM_NEXTPAGE_NOT_ALLOCATED;
        }

        *dest = 0;

        switch (temp) {
            case 0:
                *dest = ((uint32_t)mem->pageTable[nextPageNum]->data[0] << 8)
                        | ((uint32_t)mem->pageTable[nextPageNum]->data[1] << 16)
                        | ((uint32_t)mem->pageTable[nextPageNum]->data[2] << 24);
                break;
            case 1:
                *dest = ((uint32_t)mem->pageTable[pageNum]->data[offset + 1] << 8)
                        | ((uint32_t)mem->pageTable[nextPageNum]->data[0] << 16)
                        | ((uint32_t)mem->pageTable[nextPageNum]->data[1] << 24);
                break;
            case 2:
                *dest = ((uint32_t)mem->pageTable[pageNum]->data[offset + 1] << 8)
                        | ((uint32_t)mem->pageTable[pageNum]->data[offset + 2] << 16)
                        | ((uint32_t)mem->pageTable[nextPageNum]->data[0] << 24);
                break;
            default:
                mem->status = MEM_UNKNOWN_ERROR;
                return MEM_UNKNOWN_ERROR;
        }

        *dest |= mem->pageTable[pageNum]->data[offset];

        mem->status = MEM_OK;
        return MEM_OK;
    }
}

MEM_Status_t MEM_Write8(MEM_t* mem, uint32_t addr, uint8_t val)
{
    uint32_t pageNum, offset;
    MEM_Status_t ret = MEM_RWCommon(false, mem, addr, &pageNum, &offset, NULL);
    if(ret != MEM_OK)
    {
        if(mem != NULL)
            mem->status = ret;
        return ret;
    }

    mem->pageTable[pageNum]->data[offset] = val;

    mem->status = MEM_OK;
    return MEM_OK;
}

MEM_Status_t MEM_Write16(MEM_t* mem, uint32_t addr, uint16_t val)
{
    uint32_t pageNum, offset;
    MEM_Status_t ret = MEM_RWCommon(false, mem, addr, &pageNum, &offset, NULL);
    if(ret != MEM_OK)
    {
        if(mem != NULL)
            mem->status = ret;
        return ret;
    }

    if((offset + 1) == MEM_PAGE_SIZE)
    {
        uint32_t nextPageNum = pageNum + 1;
        if(nextPageNum == MEM_PAGE_COUNT)
        {
            mem->status = MEM_NEXTPAGENUMBER_OUTOFRANGE;
            return MEM_NEXTPAGENUMBER_OUTOFRANGE;
        }
        if(mem->pageTable[nextPageNum] == NULL)
        {
            mem->status = MEM_NEXTPAGE_NOT_ALLOCATED;
            return MEM_NEXTPAGE_NOT_ALLOCATED;
        }

        mem->pageTable[nextPageNum]->data[0] = (uint8_t)(val >> 8);
    }
    else
    {
        mem->pageTable[pageNum]->data[offset + 1] = (uint8_t)(val >> 8);
    }

    mem->pageTable[pageNum]->data[offset] = (uint8_t)val;

    mem->status = MEM_OK;
    return MEM_OK;
}

MEM_Status_t MEM_Write32(MEM_t* mem, uint32_t addr, uint32_t val)
{
    uint32_t pageNum, offset, temp;
    MEM_Status_t ret = MEM_RWCommon(false, mem, addr, &pageNum, &offset, NULL);
    if(ret != MEM_OK)
    {
        if(mem != NULL)
            mem->status = ret;
        return ret;
    }

    temp = (MEM_PAGE_SIZE - 1) - offset;
    if(temp >= 3)
    {
        mem->pageTable[pageNum]->data[offset] = (uint8_t)val;
        mem->pageTable[pageNum]->data[offset + 1] = (uint8_t)(val >> 8);
        mem->pageTable[pageNum]->data[offset + 2] = (uint8_t)(val >> 16);
        mem->pageTable[pageNum]->data[offset + 3] = (uint8_t)(val >> 24);

        mem->status = MEM_OK;
        return MEM_OK;
    }
    else
    {
        uint32_t nextPageNum = pageNum + 1;
        if(nextPageNum == MEM_PAGE_COUNT)
        {
            mem->status = MEM_NEXTPAGENUMBER_OUTOFRANGE;
            return MEM_NEXTPAGENUMBER_OUTOFRANGE;
        }
        if(mem->pageTable[nextPageNum] == NULL)
        {
            mem->status = MEM_NEXTPAGE_NOT_ALLOCATED;
            return MEM_NEXTPAGE_NOT_ALLOCATED;
        }
        
        switch (temp)
        {
            case 0:
                mem->pageTable[nextPageNum]->data[0] = (uint8_t)(val >> 8);
                mem->pageTable[nextPageNum]->data[1] = (uint8_t)(val >> 16);
                mem->pageTable[nextPageNum]->data[2] = (uint8_t)(val >> 24);
                break;
            case 1:
                mem->pageTable[pageNum]->data[offset + 1] = (uint8_t)(val >> 8);
                mem->pageTable[nextPageNum]->data[0] = (uint8_t)(val >> 16);
                mem->pageTable[nextPageNum]->data[1] = (uint8_t)(val >> 24);
                break;
            case 2:
                mem->pageTable[pageNum]->data[offset + 1] = (uint8_t)(val >> 8);
                mem->pageTable[pageNum]->data[offset + 2] = (uint8_t)(val >> 16);
                mem->pageTable[nextPageNum]->data[0] = (uint8_t)(val >> 24);
                break;
            default:
                mem->status = MEM_UNKNOWN_ERROR;
                return MEM_UNKNOWN_ERROR;
        }

        mem->pageTable[pageNum]->data[offset] = (uint8_t)val;

        mem->status = MEM_OK;
        return MEM_OK;
    }
}

inline uint32_t MEM_GetAddr(uint32_t pageNum, uint32_t offset)
{
    return ((pageNum << MEM_PAGE_OFFSET_BITS) | (offset & MEM_PAGE_OFFSET_MASK));
}
