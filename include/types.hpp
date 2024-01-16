#pragma once

#include <switch.h>

#define MHz *1E6

#define BUFFER_SIZE 0x10000

typedef union {
    u8 _u8;
    s8 _s8;
    u16 _u16;
    s16 _s16;
    u32 _u32;
    s32 _s32;
    u64 _u64;
    s64 _s64;
    float _f32;
    double _f64;
} searchValue_t;

typedef enum {
    SEARCH_TYPE_NONE = -1,
    SEARCH_TYPE_UNSIGNED_8BIT,
    SEARCH_TYPE_SIGNED_8BIT,
    SEARCH_TYPE_UNSIGNED_16BIT,
    SEARCH_TYPE_SIGNED_16BIT,
    SEARCH_TYPE_UNSIGNED_32BIT,
    SEARCH_TYPE_SIGNED_32BIT,
    SEARCH_TYPE_UNSIGNED_64BIT,
    SEARCH_TYPE_SIGNED_64BIT,
    SEARCH_TYPE_FLOAT_32BIT,
    SEARCH_TYPE_FLOAT_64BIT,
    SEARCH_TYPE_POINTER
} searchType_t;

typedef enum {
    SEARCH_MODE_NONE = -1,
    SEARCH_MODE_EQ,
    SEARCH_MODE_NEQ,
    SEARCH_MODE_GT,
    SEARCH_MODE_GTE,
    SEARCH_MODE_LT,
    SEARCH_MODE_LTE,
    SEARCH_MODE_RANGE,
    SEARCH_MODE_SAME,
    SEARCH_MODE_DIFF,
    SEARCH_MODE_INC,
    SEARCH_MODE_DEC
} searchMode_t;

typedef enum {
    SEARCH_REGION_NONE = -1,
    SEARCH_REGION_HEAP,
    SEARCH_REGION_MAIN,
    SEARCH_REGION_HEAP_AND_MAIN,
    SEARCH_REGION_RAM
} searchRegion_t;

typedef enum DumpType {
    UNDEFINED = '-', ADDR = 'A', DATA = 'D'
} DumpType;

typedef struct DataHeader {
    u32 magic;                              // EDZN 0x4E5A4445 (Reversed for LE)
    char dumpType;                          // '-' (0x2D) for not set yet, 'A' (0x41) for addresses, 'D' (0x44) for data
    u32 dataSize;                           // Size of data
    searchType_t searchDataType;
    searchMode_t searchMode;
    searchRegion_t searchRegion;
    searchValue_t searchValue[2];
    u64 heapBaseAddress;
    u64 heapSize;
    u64 mainBaseAddress;
    u64 mainSize;
    u64 addrSpaceBaseAddress;
    u8 endOfHeader;                         // '@' - Signals the end of the header
} __attribute__((packed)) data_header_t;
