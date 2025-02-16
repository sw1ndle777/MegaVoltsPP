#include "CDBM.h"
template <typename T> void serialize(std::vector<std::uint8_t>& v, const T& obj) 
{
    auto size = v.size();
    v.resize(size + sizeof(T));
    std::memcpy(&v[size], &obj, sizeof(T));
}
void serialize_string(std::vector<std::uint8_t>& v, std::string obj, std::size_t objSize)
{
    auto size = v.size();
    std::vector<uint8_t> vec(obj.begin(), obj.end());
    std::vector<uint8_t> vec2;
    for (int i = 0; i < objSize - vec.size(); i++)
        vec2.push_back(0);

    vec.insert(vec.end(), vec2.begin(), vec2.end());
    v.insert(v.end(), vec.begin(), vec.end());
}

template <typename T>
void deserialize(const std::vector<std::uint8_t>& v, std::size_t offset, T& obj) 
{
    std::memcpy(&obj, &v[offset], sizeof(T));
}

template <typename T>
T deserialize(const std::vector<std::uint8_t>& v, std::size_t offset) 
{
    T obj{};
    deserialize(v, offset, obj);
    return obj;
}

std::int32_t GetSizeFromFieldType(FIELD_TYPE fieldType)
{
    switch (fieldType)
    {
    case FIELD_TYPE::FIELD_UINT8_T:
        return sizeof(std::uint8_t);
    case FIELD_TYPE::FIELD_BOOL:
        return sizeof(bool);
    case FIELD_TYPE::FIELD_UINT16_T:
        return sizeof(std::uint16_t);
    case FIELD_TYPE::FIELD_UINT32_T:
        return sizeof(std::uint32_t);
    default:
        return 0;
    }
}
const char* CDBField::GetFieldTypeStr()
{
    switch (this->m_eType)
    {
    case FIELD_TYPE::FIELD_UINT8_T:
        return "FIELD_UINT8_T";
    case FIELD_TYPE::FIELD_BOOL:
        return "FIELD_BOOL";
    case FIELD_TYPE::FIELD_UINT16_T:
        return "FIELD_UINT16_T";
    case FIELD_TYPE::FIELD_UINT32_T:
        return "FIELD_UINT32_T";
    default:
        return "FIELD_STRING";
    }
}
std::uint8_t CDBField::GetChar()
{
    return *reinterpret_cast<std::uint8_t*>(&field_data.data()[0]);
}
bool CDBField::GetBool()
{
    return *reinterpret_cast<bool*>(&field_data.data()[0]);
}
std::uint16_t CDBField::GetShort()
{
    return *reinterpret_cast<uint16_t*>(&field_data.data()[0]);
}
std::uint32_t CDBField::GetInt()
{
    return *reinterpret_cast<uint32_t*>(&field_data.data()[0]);
}
std::string CDBField::GetString()
{
    return std::string(&field_data.data()[0], &field_data.data()[0] + field_data.size());
}
std::string CDBField::GetAnyString()
{
    switch (this->m_eType)
    {
    case FIELD_TYPE::FIELD_UINT8_T:
        return std::to_string(this->GetChar());
    case FIELD_TYPE::FIELD_BOOL:
        return std::to_string(this->GetBool());
    case FIELD_TYPE::FIELD_UINT16_T:
        return std::to_string(this->GetShort());
    case FIELD_TYPE::FIELD_UINT32_T:
        return std::to_string(this->GetInt());
    default:
        return this->GetString().c_str();
    }
}
CDBField::CDBField()
{
    this->m_iSize = 0;
    this->m_iOffset = 0;
    memset(this->m_szName, 0, sizeof(this->m_szName));
    this->m_eType = FIELD_TYPE::FIELD_BOOL;
}
CDBField::CDBField(FIELD_TYPE eType, std::uint32_t iSize, std::uint32_t iOffset)
{
    this->m_eType = eType;
    this->m_iSize = iSize;
    this->m_iOffset = iOffset;
    memset(this->m_szName, 0, sizeof(this->m_szName));
}
CDBField::CDBField(CDBField& o)
{
    this->m_eType = o.m_eType;
    this->m_iSize = o.m_iSize;
    this->m_iOffset = o.m_iOffset;
    strcpy_s(this->m_szName, o.m_szName);
}

void CDBField::Clear()
{
    this->m_iSize = 0;
    this->m_iOffset = 0;
    std::memset(this->m_szName, 0, sizeof(this->m_szName));
    this->m_eType = FIELD_TYPE::FIELD_BOOL;
}
void CDBM::Clear()
{
    this->m_kHeader.m_iFieldCount = 0;
    this->m_kHeader.m_iRowCount = 0;
    this->m_kHeader.m_iRowSize = 0;
    this->m_kBox.clear();
    this->m_Data.clear();
    this->m_kBoxIterator = 0;
    this->m_fileSize = 0;
}
std::vector<uint8_t> CDBM::GetData(std::uint32_t iRow, std::uint32_t iOffset, std::uint32_t iSize)
{
  return { &this->m_Data.data()[0] + this->m_kHeader.m_iRowSize * iRow + iOffset , &this->m_Data.data()[0] + this->m_kHeader.m_iRowSize * iRow + iOffset + iSize };
}
CDBField* CDBM::GetField(const char* szName)
{
    if (this->m_kBox.empty() || !szName) return nullptr;
    for (this->m_kBoxIterator = 0; this->m_kBoxIterator < this->m_kBox.size(); this->m_kBoxIterator++)
        if (this->m_kBox[this->m_kBoxIterator] && !strcmp(this->m_kBox[this->m_kBoxIterator]->GetName(), szName))
            return this->m_kBox[this->m_kBoxIterator];

    return nullptr;
}
CDBField* CDBM::GetField(std::uint32_t uiIndex)
{
    if (this->m_kBox.empty() || uiIndex < 0 || uiIndex == this->m_kBox.size()) return nullptr;
    return this->m_kBox[uiIndex];
}
//std::vector<std::unordered_map<std::string, CDBField*>> CDBM::GetDataRows()
std::vector<boost::unordered_flat_map<std::string, CDBField*>> CDBM::GetDataRows()
{
    return this->m_DataRows;
}

void write_file(const std::string& filepath, std::vector<std::uint8_t> from)
{
    std::ofstream output(filepath.c_str(), std::ios::out | std::ios::binary);
    output.write((char*)from.data(), from.size());
    output.flush();
    output.close();
}

bool CDBM::LoadCDB(std::vector<std::uint8_t> contents)
{
    std::uint32_t lastPos = 0;
    this->m_fileSize = static_cast<std::uint32_t>(contents.size());
    this->m_kHeader.m_iFieldCount = *reinterpret_cast<std::int32_t*>(&contents.data()[0]);
    lastPos += sizeof(std::int32_t);
    std::vector<std::string> fieldNames;
    std::vector<std::uint32_t> fieldTypes;
    for (this->m_kBoxIterator = 0; this->m_kBoxIterator < this->m_kHeader.m_iFieldCount; this->m_kBoxIterator++)
    {
        fieldNames.push_back(std::string(&contents.data()[lastPos], &contents.data()[lastPos] + iDB_FIELD_NAME_LEN));
        lastPos += iDB_FIELD_NAME_LEN;
    }
    for (this->m_kBoxIterator = 0; this->m_kBoxIterator < this->m_kHeader.m_iFieldCount; this->m_kBoxIterator++)
    {
        fieldTypes.push_back(*reinterpret_cast<std::uint32_t*>(&contents.data()[0] + lastPos));
        lastPos += sizeof(std::uint32_t);
    }
    for (this->m_kBoxIterator = 0; this->m_kBoxIterator < this->m_kHeader.m_iFieldCount; this->m_kBoxIterator++)
    {
        auto fieldType = static_cast<FIELD_TYPE>(fieldTypes[this->m_kBoxIterator] > 4 ? FIELD_TYPE::FIELD_STRING : fieldTypes[this->m_kBoxIterator]);
        auto offset = 0;
        if (this->m_kBoxIterator > 0)
            for (std::uint32_t i = 0; i < this->m_kBoxIterator; i++)
                offset += fieldTypes[i];
        auto size = fieldTypes[m_kBoxIterator] > 4 ? fieldTypes[m_kBoxIterator] : GetSizeFromFieldType(static_cast<FIELD_TYPE>(fieldTypes[this->m_kBoxIterator]));
        auto NewField = new CDBField(fieldType, size, offset);

        NewField->SetName(fieldNames[this->m_kBoxIterator].c_str());
        this->m_kBox.push_back(NewField);
    }
    this->m_Data = { &contents.data()[0] + lastPos , &contents.data()[0] + contents.size() };

    if (this->m_kBoxIterator > 0)
        for (std::uint32_t i = 0; i < this->m_kBoxIterator; i++)
            this->m_kHeader.m_iRowSize += this->m_kBox[i]->GetSize();

    this->m_kHeader.m_iRowCount = static_cast<std::uint32_t>(contents.size() - lastPos) / this->m_kHeader.m_iRowSize;
    
    for (m_TempDataRowIterator = 0; m_TempDataRowIterator < this->GetDataCount(); m_TempDataRowIterator++)
    {
        if (!m_TempDataRow.empty())
            m_TempDataRow.clear();

        for (m_TempFieldIterator = 0; m_TempFieldIterator < this->GetFieldCount(); m_TempFieldIterator++)
        {
            auto field = this->GetField(m_TempFieldIterator);
            auto data = this->GetData(m_TempDataRowIterator, field->GetOffset(), field->GetSize());
            auto* field_clone = new CDBField(*field);
            field_clone->SetFieldData(data);
            if (this->m_DataFieldNames.size() != this->GetFieldCount())
                this->m_DataFieldNames.push_back(field->GetName());

            m_TempDataRow[field->GetName()] = field_clone;
        }
        this->m_DataRows.push_back(m_TempDataRow);
    }
    return true;
}
std::vector<std::uint8_t> load_file(const std::string& filepath)
{
    std::ifstream ifs(filepath, std::ios::binary | std::ios::ate);

    if (!ifs)
        std::printf("error loading file\n");

    auto end = ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    auto size = static_cast<std::size_t>(end - ifs.tellg());

    if (size == 0)
        return {};

    std::vector<uint8_t> buffer(size);

    if (!ifs.read(reinterpret_cast<char*>(buffer.data()), buffer.size()))
        std::printf("error reading file\n");

    return buffer;
}
bool CDBM::LoadCDB(const char* szPath)
{
    auto contents = load_file(szPath);
    return LoadCDB(contents);
}

bool CDBM::SaveCDB(const char* szPath)
{
    std::vector<std::uint8_t> outData;
    serialize(outData, this->m_kHeader.m_iFieldCount);
    for (this->m_kBoxIterator = 0; this->m_kBoxIterator < this->m_kHeader.m_iFieldCount; this->m_kBoxIterator++)
        serialize_string(outData, this->m_kBox[this->m_kBoxIterator]->m_szName, iDB_FIELD_NAME_LEN);

    for (this->m_kBoxIterator = 0; this->m_kBoxIterator < this->m_kHeader.m_iFieldCount; this->m_kBoxIterator++)
        serialize(outData, this->m_kBox[this->m_kBoxIterator]->GetSize());

    outData.insert(outData.end(), this->m_Data.begin(), this->m_Data.end());
    write_file(szPath, outData);
    return true;
}



