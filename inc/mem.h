/**
 * @file mem.h
 * @author Emir Erbaltacı (erbaltaciemir@hotmail.com)
 * @brief Header File for Memory Controller Structures and Function Prototypes
 * @version 0.1.0
 * @date 17-09-2026
 * 
 * @copyright Copyright (c) 2026 Emir Erbaltacı. Licensed under the MIT License.
 * 
 */

#ifndef MEM_H
#define MEM_H

#include <stdint.h>

#define MEM_PAGE_SIZE 4096 /// Bytes
#define MEM_PAGE_COUNT (1U << 20) /// 2^20 Pages
#define MEM_PAGE_OFFSET_BITS 12 /// Starting with LSB (least significant bit, inclusive)
#define MEM_PAGE_OFFSET_MASK 0xFFF  /// Proper bitmask for MEM_PAGE_OFFSET_BITS

/// @brief Enumeration of memory controller responses.
typedef enum {
    MEM_OK = 0,
    MEM_BUSY,
    MEM_NULL_POINTER,
    MEM_ALLOC_FAIL,
    MEM_ALREADY_FREE,
    MEM_PAGENUMBER_OUTOFRANGE,
    MEM_NEXTPAGENUMBER_OUTOFRANGE,
    MEM_NEXTPAGE_NOT_ALLOCATED,
    MEM_PAGENUMBER_OCCUPIED,
    MEM_ACCESS_TO_FREE_PAGE,
    MEM_UNKNOWN_ERROR
}MEM_Status_t;

/// @brief Memory page structure.
typedef struct {
    uint32_t page_num;  /// Page number of the memory page itself.
    uint8_t* data;  /// Pointer to the byte array of size MEM_PAGE_SIZE.
}MEM_Page_t;

/// @brief Memory controller structure.
typedef struct {
    MEM_Page_t** pageTable; /// Pointer to page pointers. Will be replaced by multi-level paging in near future.
    MEM_Status_t status;    /// Memory controller status arising from the last operation.
}MEM_t;

/**
 * @brief Memory Controller Initializer
 * 
 * @param[in,out] mem > Pointer to memory controller structure
 * 
 * @retval MEM_OK > Controller initialization was successful.
 * @retval MEM_NULL_POINTER > The provided pointer (mem) is NULL.
 * @retval MEM_ALLOC_FAIL > Memory allocation failed, device might be out of free contiguous memory.
 */
MEM_Status_t MEM_Init(MEM_t* mem); 

/**
 * @brief Memory Controller Deinitializer
 * 
 * @param[in,out] mem > Pointer to memory controller structure
 * 
 * @retval MEM_OK > Controller deinitialization was successful.
 * @retval MEM_NULL_POINTER > Either mem or mem->pageTable is NULL (or both are NULL).
 * @retval MEM_PAGENUMBER_OUTOFRANGE > mem->pageTable[i] does not exist. Normally this should not return.
 * @retval MEM_ALREADY_FREE > mem->pageTable[i] is NULL where i denotes any page number. Normally this should not return.
 * 
 * @warning This function DOES NOT free mem internally. 
 * If mem is allocated on the HEAP, you MUST DO ''' free(mem); mem = NULL; ''' explicitly in your application.
 * Only internals are freed in the function.
 */
MEM_Status_t MEM_Free(MEM_t* mem);

/**
 * @brief Memory Page Allocator
 * 
 * @param[in,out] mem > Pointer to memory controller structure
 * @param pageNum > Desired page number to be allocated
 * 
 * @retval MEM_OK > Page allocation was successful.
 * @retval MEM_NULL_POINTER > The provided pointer (mem) is NULL.
 * @retval MEM_PAGENUMBER_OUTOFRANGE > Page number (pageNum) is larger than (MEM_PAGE_COUNT - 1)
 * @retval MEM_PAGENUMBER_OCCUPIED > The page mem->pageTable[pageNum] has already been allocated.
 * @retval MEM_ALLOC_FAIL > Memory allocation failed, device might be out of free contiguous memory.
 */
MEM_Status_t MEM_PageAlloc(MEM_t* mem, uint32_t pageNum);

/**
 * @brief Memory Page Deallocator
 * 
 * @param[in,out] mem > Pointer to memory controller structure
 * @param pageNum > Desired page number to be deallocated
 * 
 * @retval MEM_OK > Page deallocation was successful.
 * @retval MEM_NULL_POINTER > Either mem or mem->pageTable is NULL (or both are NULL).
 * @retval MEM_PAGENUMBER_OUTOFRANGE > Page number (pageNum) is larger than (MEM_PAGE_COUNT - 1)
 * @retval MEM_ALREADY_FREE > mem->pageTable[pageNum] is NULL. No need for deallocation.
 */
MEM_Status_t MEM_FreePage(MEM_t* mem, uint32_t pageNum);

/**
 * @brief Reads 8 bits (1 byte) from memory.
 * 
 * @param mem > Pointer to memory controller structure
 * @param addr > Address of the memory location, bits 0-11 are offset while bits 12-31 are page number.
 * @param dest > Pointer to destination variable
 * 
 * @retval MEM_OK > Read operation successful.
 * @retval MEM_NULL_POINTER > At least one is NULL: mem, mem->pageTable, dest. If not, something is wrong with implementation code.
 * @retval MEM_PAGENUMBER_OUTOFRANGE > Page number is larger than (MEM_PAGE_COUNT - 1). Check address value.
 * @retval MEM_ACCESS_TO_FREE_PAGE > The page including specified address is not allocated. Either the page is really not allocated or the address value is wrong.
 * 
 * @note To get '''addr''' by specifying page number and offset, you may also use MEM_GetAddr function.
 */
MEM_Status_t MEM_Read8(MEM_t* mem, uint32_t addr, uint8_t* dest);

/**
 * @brief Reads 16 bits (2 bytes) from memory.
 * 
 * @param mem > Pointer to memory controller structure
 * @param addr > Address of the memory location, bits 0-11 are offset while bits 12-31 are page number.
 * @param dest > Pointer to destination variable
 * 
 * @retval MEM_OK > Read operation successful.
 * @retval MEM_NULL_POINTER > At least one is NULL: mem, mem->pageTable, dest. If not, something is wrong with implementation code.
 * @retval MEM_PAGENUMBER_OUTOFRANGE > Page number is larger than (MEM_PAGE_COUNT - 1). Check address value.
 * @retval MEM_ACCESS_TO_FREE_PAGE > The page including specified address is not allocated. Either the page is really not allocated or the address value is wrong.
 * @retval MEM_NEXTPAGE_NOT_ALLOCATED > Offset value is large so a cross-page read should take place, but the next page is not allocated.
 * Might be encountered when offset is maximum (MAX_PAGE_SIZE - 1). Check address value. Consider switching to MEM_Read8.
 * @retval MEM_NEXTPAGENUMBER_OUTOFRANGE > Offset value is large so a cross-page read should take place, but the next page number is MEM_PAGE_COUNT.
 * Encountered when page number is (MEM_PAGE_COUNT - 1) and offset is (MEM_PAGE_SIZE - 1). Check address value. Consider switching to MEM_Read8.
 * 
 * @note To get '''addr''' by specifying page number and offset, you may also use MEM_GetAddr function.
 * @note The function takes care of cross-page reads.
 */
MEM_Status_t MEM_Read16(MEM_t* mem, uint32_t addr, uint16_t* dest);

/**
 * @brief Reads 32 bits (4 bytes) from memory.
 * 
 * @param mem > Pointer to memory controller structure
 * @param addr > Address of the memory location, bits 0-11 are offset while bits 12-31 are page number.
 * @param dest > Pointer to destination variable
 * 
 * @retval MEM_OK > Read operation successful.
 * @retval MEM_NULL_POINTER > At least one is NULL: mem, mem->pageTable, dest. If not, something is wrong with implementation code.
 * @retval MEM_PAGENUMBER_OUTOFRANGE > Page number is larger than (MEM_PAGE_COUNT - 1). Check address value.
 * @retval MEM_ACCESS_TO_FREE_PAGE > The page including specified address is not allocated. Either the page is really not allocated or the address value is wrong.
 * @retval MEM_NEXTPAGE_NOT_ALLOCATED > Offset value is large so a cross-page read should take place, but the next page is not allocated.
 * Might be encountered when offset is near maximum (MAX_PAGE_SIZE - 3 <= offset <= MAX_PAGE_SIZE - 1). Check address value. Consider switching to MEM_Read8 or MEM_Read16.
 * @retval MEM_NEXTPAGENUMBER_OUTOFRANGE > Offset value is large so a cross-page read should take place, but the next page number is MEM_PAGE_COUNT.
 * Encountered when page number is (MEM_PAGE_COUNT - 1) and offset is near maximum (MAX_PAGE_SIZE - 3 <= offset <= MEM_PAGE_SIZE - 1). 
 * Check address value. Consider switching to MEM_Read8 or MEM_Read16.
 * @retval MEM_UNKNOWN_ERROR > Should never return this. If returns, the implementation is wrong.
 * 
 * @note To get '''addr''' by specifying page number and offset, you may also use MEM_GetAddr function.
 * @note The function takes care of cross-page reads.
 */
MEM_Status_t MEM_Read32(MEM_t* mem, uint32_t addr, uint32_t* dest);

/**
 * @brief Writes 8 bits (1 byte) to memory.
 * 
 * @param mem > Pointer to memory controller structure
 * @param addr > Address of the memory location, bits 0-11 are offset while bits 12-31 are page number.
 * @param val > Value to write
 * 
 * @retval MEM_OK > Write operation successful.
 * @retval MEM_NULL_POINTER > Either mem or mem->pageTable is NULL.
 * @retval MEM_PAGENUMBER_OUTOFRANGE > Page number is larger than (MEM_PAGE_COUNT - 1). Check address value.
 * @retval MEM_ACCESS_TO_FREE_PAGE > The page including specified address is not allocated. Either the page is really not allocated or the address value is wrong.
 * 
 * @note To get '''addr''' by specifying page number and offset, you may also use MEM_GetAddr function.
 */
MEM_Status_t MEM_Write8(MEM_t* mem, uint32_t addr, uint8_t val);

/**
 * @brief Writes 16 bits (2 bytes) to memory.
 * 
 * @param mem > Pointer to memory controller structure
 * @param addr > Address of the memory location, bits 0-11 are offset while bits 12-31 are page number.
 * @param val > Value to write
 * 
 * @retval MEM_OK > Write operation successful.
 * @retval MEM_NULL_POINTER > Either mem or mem->pageTable is NULL.
 * @retval MEM_PAGENUMBER_OUTOFRANGE > Page number is larger than (MEM_PAGE_COUNT - 1). Check address value.
 * @retval MEM_ACCESS_TO_FREE_PAGE > The page including specified address is not allocated. Either the page is really not allocated or the address value is wrong.
 * @retval MEM_NEXTPAGE_NOT_ALLOCATED > Offset value is large so a cross-page write should take place, but the next page is not allocated.
 * Might be encountered when offset is maximum (MAX_PAGE_SIZE - 1). Check address value. Consider switching to MEM_Write8.
 * @retval MEM_NEXTPAGENUMBER_OUTOFRANGE > Offset value is large so a cross-page write should take place, but the next page number is MEM_PAGE_COUNT.
 * Encountered when page number is (MEM_PAGE_COUNT - 1) and offset is (MEM_PAGE_SIZE - 1). Check address value. Consider switching to MEM_Write8.
 * 
 * @note To get '''addr''' by specifying page number and offset, you may also use MEM_GetAddr function.
 * @note The function takes care of cross-page writes.
 */
MEM_Status_t MEM_Write16(MEM_t* mem, uint32_t addr, uint16_t val);

/**
 * @brief Writes 32 bits (4 bytes) to memory.
 * 
 * @param mem > Pointer to memory controller structure
 * @param addr > Address of the memory location, bits 0-11 are offset while bits 12-31 are page number.
 * @param val > Value to write
 * 
 * @retval MEM_OK > Write operation successful.
 * @retval MEM_NULL_POINTER > Either mem or mem->pageTable is NULL.
 * @retval MEM_PAGENUMBER_OUTOFRANGE > Page number is larger than (MEM_PAGE_COUNT - 1). Check address value.
 * @retval MEM_ACCESS_TO_FREE_PAGE > The page including specified address is not allocated. Either the page is really not allocated or the address value is wrong.
 * @retval MEM_NEXTPAGE_NOT_ALLOCATED > Offset value is large so a cross-page write should take place, but the next page is not allocated.
 * Might be encountered when offset is near maximum (MAX_PAGE_SIZE - 3 <= offset <= MAX_PAGE_SIZE - 1). Check address value. Consider switching to MEM_Write8 or MEM_Write16.
 * @retval MEM_NEXTPAGENUMBER_OUTOFRANGE > Offset value is large so a cross-page write should take place, but the next page number is MEM_PAGE_COUNT.
 * Encountered when page number is (MEM_PAGE_COUNT - 1) and offset is near maximum (MAX_PAGE_SIZE - 3 <= offset <= MEM_PAGE_SIZE - 1). 
 * Check address value. Consider switching to MEM_Write8 or MEM_Write16.
 * @retval MEM_UNKNOWN_ERROR > Should never return this. If returns, the implementation is wrong.
 * 
 * @note To get '''addr''' by specifying page number and offset, you may also use MEM_GetAddr function.
 * @note The function takes care of cross-page writes.
 */
MEM_Status_t MEM_Write32(MEM_t* mem, uint32_t addr, uint32_t val);

/**
 * @brief Gets memory address from page number and offset.
 * 
 * @param pageNum > Memory page number, maximum is (MEM_PAGE_COUNT - 1)
 * @param offset > Offset from 0th byte in a page in bytes, maximum is (MEM_PAGE_SIZE - 1)
 * 
 * @return uint32_t > Corresponding address
 */
inline uint32_t MEM_GetAddr(uint32_t pageNum, uint32_t offset);

#endif // MEM_H
