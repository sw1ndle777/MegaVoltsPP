#pragma once
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <span>
#include <boost/unordered/unordered_flat_map.hpp>
constexpr uint32_t iDB_FIELD_NAME_LEN = 30;
enum FIELD_TYPE : uint32_t
{
    FIELD_UINT8_T = 0,
    FIELD_BOOL,
    FIELD_UINT16_T,
    FIELD_ENUM,
    FIELD_UINT32_T,
    FIELD_STRING
};

class CDBHeader
{
public:
    CDBHeader() : m_iFieldCount(0), m_iRowCount(0), m_iRowSize(0) {}
    ~CDBHeader() {}
public:
    uint32_t m_iFieldCount;
    uint32_t m_iRowCount;
    uint32_t m_iRowSize;
};

class CDBField {
public:
    CDBField();
    CDBField(FIELD_TYPE eType, uint32_t iSize, uint32_t iOffset);
    CDBField(const CDBField& o);
    CDBField& operator=(const CDBField& o);
    ~CDBField() {}
    void Clear();
    FIELD_TYPE GetFieldType() { return m_eType; }
    const char* GetFieldTypeStr();
    uint8_t  GetChar();
    bool GetBool();
    uint16_t GetShort();
    uint32_t GetInt();
    std::string GetString();
    std::string GetAnyString();
    uint32_t GetOffset() const { return m_iOffset; }
    uint32_t GetSize() const { return m_iSize; }
    void SetFieldData(std::span<uint8_t> newData) { field_data = newData; }
    void SetName(const char* szName) { strcpy_s(m_szName, iDB_FIELD_NAME_LEN, szName); }
    const char* GetName() { return m_szName; }
    std::span<uint8_t> GetFieldData() const { return field_data; };
    FIELD_TYPE m_eType;
    uint32_t m_iSize;
    uint32_t m_iOffset;
    char m_szName[iDB_FIELD_NAME_LEN] = "";
protected:
    std::span<uint8_t> field_data;
};

class CDBM {
public:
    CDBM() {}
    virtual ~CDBM() {}
    bool Empty() { return m_kBox.empty(); }
    void Clear();
    uint32_t GetFieldCount() { return m_kHeader.m_iFieldCount; }
    uint32_t GetDataCount() { return m_kHeader.m_iRowCount; }
    uint32_t GetDataSize() { return m_kHeader.m_iRowSize; }
    uint32_t GetFileSize() { return m_fileSize; }
    std::span<uint8_t> GetData(uint32_t iRow, uint32_t iOffset, uint32_t iSize);
    CDBField& GetField(const char* szName);
    CDBField& GetField(uint32_t uiIndex);
    bool LoadCDB(std::span<uint8_t> contents);
    bool SaveCDB(const char* szPath);
    std::vector<boost::unordered_flat_map<std::string, CDBField>>& GetDataRows();
protected:
    CDBHeader m_kHeader;
    std::vector<CDBField> m_kBox;
    std::span<uint8_t> m_Data;
    uint32_t m_kBoxIterator = 0;
    uint32_t m_fileSize = 0;
    std::vector<boost::unordered_flat_map<std::string, CDBField>> m_DataRows;
   
    std::vector<std::string> m_DataFieldNames;
    uint32_t m_TempDataRowIterator = 0;
    uint32_t m_TempFieldIterator = 0;
private:
};