#include "memory_dump.hpp"
#include <stdio.h>

MemoryDump::MemoryDump(std::string filePath, DumpType dumpType, bool discardFile) : m_filePath(filePath) {
    m_dataHeader = {0};

    m_dataHeader.magic = 0x4E5A4445;
    m_dataHeader.endOfHeader = '@';
    m_dataHeader.dumpType = (char) dumpType;

    m_dataHeader.searchDataType = SEARCH_TYPE_NONE;
    m_dataHeader.searchMode = SEARCH_MODE_NONE;
    m_dataHeader.searchRegion = SEARCH_REGION_NONE;

    m_dumpFile = fopen(filePath.c_str(), "r");
    if (m_dumpFile == nullptr) {
        m_dumpFile = fopen(filePath.c_str(), "w+b");

        writeHeader();
    } else {
        fclose(m_dumpFile);

        if (discardFile) {
            m_dumpFile = fopen(filePath.c_str(), "w+b");
            writeHeader();
        } else {
            m_dumpFile = fopen(filePath.c_str(), "r+b");
            fseek(m_dumpFile, 0, SEEK_END);

            size_t fileSize = ftell(m_dumpFile);

            if (fileSize >= sizeof(DataHeader)) {
                fseek(m_dumpFile, 0, SEEK_SET);
                fread(&m_dataHeader, sizeof(m_dataHeader), 1, m_dumpFile);
            }
        }

    }

    m_data.reserve(BUFFER_SIZE);
}

MemoryDump::~MemoryDump() {
    if (isFileOpen()) {
        FlushBuffer();
        fclose(m_dumpFile);
        m_dumpFile = nullptr;
    }
}

void MemoryDump::SetBaseAddresses(u64 addrSpaceBase, u64 heapBase, u64 mainBase, u64 heapSize, u64 mainSize) {
    m_dataHeader.addrSpaceBaseAddress = addrSpaceBase;
    m_dataHeader.heapBaseAddress = heapBase;
    m_dataHeader.mainBaseAddress = mainBase;

    m_dataHeader.heapSize = heapSize;
    m_dataHeader.mainSize = mainSize;

    writeHeader();
}

void MemoryDump::SetSearchParams(searchType_t searchDataType, searchMode_t searchMode, searchRegion_t searchRegion,
                                 searchValue_t searchValue1, searchValue_t searchValue2) {
    m_dataHeader.searchDataType = searchDataType;
    m_dataHeader.searchMode = searchMode;
    m_dataHeader.searchRegion = searchRegion;
    m_dataHeader.searchValue[0] = searchValue1;
    m_dataHeader.searchValue[1] = searchValue2;

    writeHeader();
}

void MemoryDump::AddData(u8 *buffer, size_t dataSize) {
    if ((m_data.size() + dataSize) < BUFFER_SIZE) {
        std::copy(buffer, buffer + dataSize, std::back_inserter(m_data));
    } else if (dataSize <= BUFFER_SIZE) {
        if (isFileOpen()) {
            FlushBuffer();
            std::copy(buffer, buffer + dataSize, std::back_inserter(m_data));
        }
    } else {
        if (isFileOpen()) {
            fseek(m_dumpFile, 0, SEEK_END);

            FlushBuffer();

            fwrite(buffer, sizeof(u8), dataSize, m_dumpFile);
            m_dataHeader.dataSize += dataSize;
            writeHeader();
        }
    }
}

size_t MemoryDump::Size() {
    FlushBuffer();

    return m_dataHeader.dataSize;
}

void MemoryDump::Clear() {
    m_data.clear();
    m_dataHeader.dataSize = 0;
    m_dataHeader.dumpType = DumpType::UNDEFINED;

    if (isFileOpen()) {
        fclose(m_dumpFile);
        m_dumpFile = nullptr;
    }

    m_dumpFile = fopen(m_filePath.c_str(), "w+b");
    writeHeader();
}

int MemoryDump::GetData(u64 addr, void *buffer, size_t bufferSize) {
    if (!isFileOpen()) return 1;

    FlushBuffer();

    fseek(m_dumpFile, sizeof(struct DataHeader) + addr, SEEK_SET);
    fread(buffer, sizeof(u8), bufferSize, m_dumpFile);

    return 0;
}

u8 MemoryDump::operator[](u64 index) {
    u8 data = 0;

    if (!isFileOpen()) return 0;

    FlushBuffer();

    fseek(m_dumpFile, sizeof(struct DataHeader) + index, SEEK_SET);
    fread(&data, sizeof(u8), 1, m_dumpFile);

    return data;
}

bool MemoryDump::isFileOpen() {
    return m_dumpFile != nullptr;
}

void MemoryDump::FlushBuffer() {
    if (m_data.size() > 0 && isFileOpen()) {
        fseek(m_dumpFile, 0, SEEK_END);
        fwrite(&m_data[0], sizeof(u8), m_data.size(), m_dumpFile);

        m_dataHeader.dataSize += m_data.size();

        m_data.clear();

        writeHeader();
    }
}

void MemoryDump::writeHeader() {
    if (isFileOpen()) {
        fseek(m_dumpFile, 0, SEEK_SET);
        fwrite(&m_dataHeader, sizeof(m_dataHeader), 1, m_dumpFile);
        fflush(m_dumpFile);
    }
}

data_header_t MemoryDump::GetDumpInfo() {
    return m_dataHeader;
}

void MemoryDump::SetDumpType(DumpType dumpType) {
    if (m_dataHeader.dumpType != UNDEFINED) {
        m_dataHeader.dumpType = dumpType;
        Clear();
    } else {
        m_dataHeader.dumpType = dumpType;
        writeHeader();
    }
}
