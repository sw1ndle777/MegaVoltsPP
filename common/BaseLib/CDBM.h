#pragma once
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <span>
#include <boost/unordered/unordered_flat_map.hpp>
constexpr std::uint32_t iDB_FIELD_NAME_LEN = 30;
enum FIELD_TYPE : std::uint32_t
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
    std::uint32_t m_iFieldCount;
    std::uint32_t m_iRowCount;
    std::uint32_t m_iRowSize;
};

class CDBField {
public:
    CDBField();
    CDBField(FIELD_TYPE eType, std::uint32_t iSize, std::uint32_t iOffset);
    CDBField(const CDBField& o);
    CDBField& operator=(const CDBField& o);
    ~CDBField() {}
    void Clear();
    FIELD_TYPE GetFieldType() { return m_eType; }
    const char* GetFieldTypeStr();
    std::uint8_t  GetChar();
    bool GetBool();
    std::uint16_t GetShort();
    std::uint32_t GetInt();
    std::string GetString();
    std::string GetAnyString();
    std::uint32_t GetOffset() const { return m_iOffset; }
    std::uint32_t GetSize() const { return m_iSize; }
    void SetFieldData(std::span<std::uint8_t> newData) { field_data = newData; }
    void SetName(const char* szName) { strcpy_s(m_szName, iDB_FIELD_NAME_LEN, szName); }
    const char* GetName() { return m_szName; }
    std::span<std::uint8_t> GetFieldData() const { return field_data; };
    FIELD_TYPE m_eType;
    std::uint32_t m_iSize;
    std::uint32_t m_iOffset;
    char m_szName[iDB_FIELD_NAME_LEN] = "";
protected:
    std::span<std::uint8_t> field_data;
};

class CDBM {
public:
    CDBM() {}
    virtual ~CDBM() {}
    bool Empty() { return m_kBox.empty(); }
    void Clear();
    std::uint32_t GetFieldCount() { return m_kHeader.m_iFieldCount; }
    std::uint32_t GetDataCount() { return m_kHeader.m_iRowCount; }
    std::uint32_t GetDataSize() { return m_kHeader.m_iRowSize; }
    std::uint32_t GetFileSize() { return m_fileSize; }
    std::span<std::uint8_t> GetData(std::uint32_t iRow, std::uint32_t iOffset, std::uint32_t iSize);
    CDBField& GetField(const char* szName);
    CDBField& GetField(std::uint32_t uiIndex);
    bool LoadCDB(std::span<std::uint8_t> contents);
    bool SaveCDB(const char* szPath);
    std::vector<boost::unordered_flat_map<std::string, CDBField>>& GetDataRows();
protected:
    CDBHeader m_kHeader;
    std::vector<CDBField> m_kBox;
    std::span<std::uint8_t> m_Data;
    std::uint32_t m_kBoxIterator = 0;
    std::uint32_t m_fileSize = 0;
    std::vector<boost::unordered_flat_map<std::string, CDBField>> m_DataRows;
   
    std::vector<std::string> m_DataFieldNames;
    std::uint32_t m_TempDataRowIterator = 0;
    std::uint32_t m_TempFieldIterator = 0;
private:
};