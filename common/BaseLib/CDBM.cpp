#include "CDBM.h"


template <typename T> static void serialize(std::vector<uint8_t>& v, const T& obj)
{
    const size_t size = v.size();
    v.resize(size + sizeof(T));
    std::memcpy(&v[size], &obj, sizeof(T));
}
static void serialize_string(std::vector<uint8_t>& v, std::string obj, size_t objSize)
{
    std::vector<uint8_t> vec(obj.begin(), obj.end());
    if (vec.size() < objSize) vec.resize(objSize, 0);
    v.insert(v.end(), vec.begin(), vec.end());
}

[[nodiscard]] static constexpr int32_t GetSizeFromFieldType(FIELD_TYPE fieldType)
{
    switch (fieldType)
    {
    case FIELD_UINT8_T:  return static_cast<int32_t>(sizeof(uint8_t));
    case FIELD_BOOL:     return static_cast<int32_t>(sizeof(bool));
    case FIELD_UINT16_T: return static_cast<int32_t>(sizeof(uint16_t));
    case FIELD_ENUM:     return static_cast<int32_t>(sizeof(uint32_t));
    case FIELD_UINT32_T: return static_cast<int32_t>(sizeof(uint32_t));
    default:             return 0;
    }
}

[[nodiscard]] const char* CDBField::GetFieldTypeStr() const
{
    switch (this->m_eType)
    {
    case FIELD_UINT8_T:  return "FIELD_UINT8_T";
    case FIELD_BOOL:     return "FIELD_BOOL";
    case FIELD_UINT16_T: return "FIELD_UINT16_T";
    case FIELD_ENUM:     return "FIELD_ENUM";
    case FIELD_UINT32_T: return "FIELD_UINT32_T";
    default:             return "FIELD_STRING";
    }
}
[[nodiscard]] uint8_t CDBField::GetChar() const
{
    if (field_data.size() < 1) [[unlikely]] return 0;
    return *reinterpret_cast<uint8_t*>(&field_data.data()[0]);
}
[[nodiscard]] bool CDBField::GetBool() const
{
    if (field_data.size() < 1) [[unlikely]] return false;
    return *reinterpret_cast<bool*>(&field_data.data()[0]);
}
[[nodiscard]] uint16_t CDBField::GetShort() const
{
    if (field_data.size() < 2) [[unlikely]] return 0;
    return *reinterpret_cast<uint16_t*>(&field_data.data()[0]);
}
[[nodiscard]] uint32_t CDBField::GetInt() const
{
    if (field_data.size() < 4) [[unlikely]] return 0;
    return *reinterpret_cast<uint32_t*>(&field_data.data()[0]);
}
[[nodiscard]] std::string CDBField::GetString() const
{
    if (field_data.empty()) [[unlikely]] return {};
    return std::string(&field_data.data()[0], &field_data.data()[0] + field_data.size());
}
[[nodiscard]] std::string CDBField::GetAnyString() const
{
    switch (this->m_eType)
    {
    case FIELD_UINT8_T:  return std::to_string(this->GetChar());
    case FIELD_BOOL:     return std::to_string(this->GetBool());
    case FIELD_UINT16_T: return std::to_string(this->GetShort());
    case FIELD_UINT32_T:
    case FIELD_ENUM:     return std::to_string(this->GetInt());
    default:             return this->GetString().c_str();
    }
}
CDBField::CDBField()
{
    this->m_iSize = 0;
    this->m_iOffset = 0;
    std::memset(this->m_szName, 0, sizeof(this->m_szName));
    this->m_eType = FIELD_BOOL;
}
CDBField::CDBField(FIELD_TYPE eType, uint32_t iSize, uint32_t iOffset)
{
    this->m_eType = eType;
    this->m_iSize = iSize;
    this->m_iOffset = iOffset;
    std::memset(this->m_szName, 0, sizeof(this->m_szName));
}
CDBField::CDBField(const CDBField& o)
{
    this->m_eType = o.m_eType;
    this->m_iSize = o.m_iSize;
    this->m_iOffset = o.m_iOffset;
    std::memcpy(this->m_szName, o.m_szName, sizeof(this->m_szName));
}

CDBField& CDBField::operator=(const CDBField& o)
{
    if (this != &o) {
        this->m_eType = o.m_eType;
        this->m_iSize = o.m_iSize;
        this->m_iOffset = o.m_iOffset;
        std::memcpy(this->m_szName, o.m_szName, sizeof(this->m_szName));
        this->field_data = o.field_data;
    }
    return *this;
}

void CDBField::Clear()
{
    this->m_iSize = 0;
    this->m_iOffset = 0;
    std::memset(this->m_szName, 0, sizeof(this->m_szName));
    this->m_eType = FIELD_BOOL;
}

void CDBM::Clear()
{
    this->m_kHeader.m_iFieldCount = 0;
    this->m_kHeader.m_iRowCount = 0;
    this->m_kHeader.m_iRowSize = 0;
    m_kBox.clear(); m_kBox.shrink_to_fit();
    m_DataRows.clear(); m_DataRows.shrink_to_fit();
    m_DataFieldNames.clear(); m_DataFieldNames.shrink_to_fit();
    m_TempDataRowIterator = 0;
    m_TempFieldIterator = 0;
    m_kBoxIterator = 0;
    m_fileSize = 0;
    m_DataStorage.clear(); m_DataStorage.shrink_to_fit();
    m_Data = {};
}
[[nodiscard]] std::span<uint8_t> CDBM::GetData(uint32_t iRow, uint32_t iOffset, uint32_t iSize)
{
    const uint64_t base = static_cast<uint64_t>(this->m_kHeader.m_iRowSize) * iRow;
    const uint64_t start = base + iOffset;
    const uint64_t end = start + iSize;
    if (end > this->m_Data.size()) [[unlikely]] return {};
    return { &this->m_Data.data()[0] + start , &this->m_Data.data()[0] + end };
}

CDBField& CDBM::GetField(const char* szName)
{
    static CDBField emptyField;

    if (this->m_kBox.empty() || !szName) [[unlikely]] return emptyField;

    for (this->m_kBoxIterator = 0; this->m_kBoxIterator < this->m_kBox.size(); this->m_kBoxIterator++)
        if (std::strcmp(this->m_kBox[this->m_kBoxIterator].GetName(), szName) == 0)
            return this->m_kBox[this->m_kBoxIterator];

    return emptyField;
}

CDBField& CDBM::GetField(uint32_t uiIndex)
{
    static CDBField emptyField;
    if (this->m_kBox.empty() || uiIndex == this->m_kBox.size()) [[unlikely]] return emptyField;
    return this->m_kBox[uiIndex];
}
std::vector<boost::unordered_flat_map<std::string, CDBField>>& CDBM::GetDataRows()
{
    return this->m_DataRows;
}

static void write_file(const std::string& filepath, const std::vector<uint8_t>& from)
{
    std::ofstream output(filepath.c_str(), std::ios::out | std::ios::binary);
    output.write(reinterpret_cast<const char*>(from.data()), static_cast<std::streamsize>(from.size()));
    output.flush();
    output.close();
}

bool CDBM::LoadCDB(std::span<uint8_t> contents)
{
    if (contents.size() < sizeof(uint32_t)) [[unlikely]] return false;
    uint32_t lastPos = 0;
    this->m_fileSize = static_cast<uint32_t>(contents.size());
    this->m_kHeader.m_iFieldCount = *reinterpret_cast<int32_t*>(&contents.data()[0]);
    lastPos += sizeof(int32_t);

    if (this->m_kHeader.m_iFieldCount == 0) [[unlikely]] return false;
    if (contents.size() < lastPos + static_cast<size_t>(this->m_kHeader.m_iFieldCount) * (iDB_FIELD_NAME_LEN + sizeof(uint32_t))) [[unlikely]]
        return false;

    std::vector<std::string> fieldNames;
    std::vector<uint32_t> fieldTypes;
    fieldNames.reserve(this->m_kHeader.m_iFieldCount);
    for (this->m_kBoxIterator = 0; this->m_kBoxIterator < this->m_kHeader.m_iFieldCount; this->m_kBoxIterator++)
    {
        fieldNames.emplace_back(&contents.data()[lastPos], &contents.data()[lastPos] + iDB_FIELD_NAME_LEN);
        lastPos += iDB_FIELD_NAME_LEN;
    }
    fieldTypes.reserve(this->m_kHeader.m_iFieldCount);
    for (this->m_kBoxIterator = 0; this->m_kBoxIterator < this->m_kHeader.m_iFieldCount; this->m_kBoxIterator++)
    {
        fieldTypes.push_back(*reinterpret_cast<uint32_t*>(&contents.data()[0] + lastPos));
        lastPos += sizeof(uint32_t);
    }

    this->m_kBox.clear();
    this->m_kBox.reserve(this->m_kHeader.m_iFieldCount);

    uint32_t offset = 0;
    for (uint32_t i = 0; i < this->m_kHeader.m_iFieldCount; ++i)
    {
        const uint32_t raw = fieldTypes[i];
        FIELD_TYPE fieldType;
        uint32_t size = 0;
        if (raw > 4) { fieldType = FIELD_STRING; size = raw; }
        else { fieldType = static_cast<FIELD_TYPE>(raw); size = static_cast<uint32_t>(GetSizeFromFieldType(fieldType)); }
        if (size == 0) [[unlikely]] return false;
        CDBField NewField(fieldType, size, offset);
        NewField.SetName(fieldNames[i].c_str());
        this->m_kBox.push_back(NewField);
        offset += size;
    }
    this->m_kHeader.m_iRowSize = offset;
    if (this->m_kHeader.m_iRowSize == 0) [[unlikely]] return false;
    if (contents.size() < lastPos) [[unlikely]] return false;
    m_DataStorage.assign(&contents.data()[0] + lastPos, &contents.data()[0] + contents.size());
    this->m_Data = { m_DataStorage.data(), m_DataStorage.size() };

    if (m_Data.size() % this->m_kHeader.m_iRowSize != 0) [[unlikely]] return false;

    this->m_kHeader.m_iRowCount = static_cast<uint32_t>(m_Data.size() / this->m_kHeader.m_iRowSize);

    m_DataRows.clear();
    m_DataRows.reserve(this->m_kHeader.m_iRowCount);

    for (m_TempDataRowIterator = 0; m_TempDataRowIterator < this->GetDataCount(); m_TempDataRowIterator++)
    {
        boost::unordered_flat_map<std::string, CDBField> m_TempDataRow;
        m_TempDataRow.reserve(this->m_kHeader.m_iFieldCount);
        for (m_TempFieldIterator = 0; m_TempFieldIterator < this->GetFieldCount(); m_TempFieldIterator++)
        {
            auto& field = this->m_kBox[m_TempFieldIterator];
            auto data = this->GetData(m_TempDataRowIterator, field.GetOffset(), field.GetSize());
            if (data.size() != field.GetSize()) [[unlikely]] return false; // invalid layout
            auto field_clone = CDBField(field);
            field_clone.SetFieldData(data);
            if (this->m_DataFieldNames.size() != this->GetFieldCount())
                this->m_DataFieldNames.push_back(field.GetName());

            m_TempDataRow[field.GetName()] = field_clone;
        }
        this->m_DataRows.push_back(std::move(m_TempDataRow));
    }
    return true;
}

bool CDBM::SaveCDB(const char* szPath)
{
    std::vector<uint8_t> outData;
    serialize(outData, this->m_kHeader.m_iFieldCount);
    for (this->m_kBoxIterator = 0; this->m_kBoxIterator < this->m_kHeader.m_iFieldCount; this->m_kBoxIterator++)
        serialize_string(outData, this->m_kBox[this->m_kBoxIterator].m_szName, iDB_FIELD_NAME_LEN);

    for (this->m_kBoxIterator = 0; this->m_kBoxIterator < this->m_kHeader.m_iFieldCount; this->m_kBoxIterator++)
        serialize(outData, this->m_kBox[this->m_kBoxIterator].GetSize());

    outData.insert(outData.end(), this->m_Data.begin(), this->m_Data.end());
    write_file(szPath, outData);

    return true;
}

static FIELD_TYPE parse_type(const std::string& t, bool& outIsFloat, uint32_t& outSize)
{
    outIsFloat = false;
    outSize = 0;
    std::string s;
    s.resize(t.size());
    std::transform(t.begin(), t.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (s == "u8" || s == "uint8" || s == "byte") { outSize = 1; return FIELD_UINT8_T; }
    if (s == "bool" || s == "boolean") { outSize = 1; return FIELD_BOOL; }
    if (s == "u16" || s == "uint16") { outSize = 2; return FIELD_UINT16_T; }
    if (s == "enum") { outSize = 4; return FIELD_ENUM; }
    if (s == "u32" || s == "uint32" || s == "int" || s == "integer") { outSize = 4; return FIELD_UINT32_T; }
    if (s == "float" || s == "f32") { outSize = 4; outIsFloat = true; return FIELD_UINT32_T; }
    if (s == "string" || s == "str") { return FIELD_STRING; }
    outSize = 4; return FIELD_UINT32_T;
}

static const char* type_to_string(FIELD_TYPE t, bool isFloat, uint32_t size)
{
    if (t == FIELD_STRING) return "string";
    if (size == 1 && t == FIELD_BOOL) return "bool";
    if (size == 1 && t == FIELD_UINT8_T) return "uint8";
    if (size == 2 && t == FIELD_UINT16_T) return "uint16";
    if (size == 4 && t == FIELD_ENUM) return "enum";
    if (size == 4) return isFloat ? "float" : "uint32";
    return "uint32";
}

static std::string trim_nulls(const std::string& s)
{
    auto pos = s.find('\0');
    if (pos == std::string::npos) return s;
    return s.substr(0, pos);
}

enum class JsonError {
    FileOpen,
    Parse,
    BadRoot,
    BadSchema,
    BadRows,
    FieldEntry,
    MissingFieldProperty,
    BadType,
    BadSize,
    BadRow,
    MissingValue
};

bool CDBM::LoadJSON(const char* szPath)
{
    using namespace rapidjson;

    auto expect = [&]() -> std::expected<void, std::string>
        {
            std::ifstream ifs(szPath);
            if (!ifs) [[unlikely]]
                return std::unexpected(std::string(magic_enum::enum_name(JsonError::FileOpen)) + ": cannot open file");

            IStreamWrapper isw(ifs);
            Document d; d.ParseStream(isw);
            if (d.HasParseError() || !d.IsObject()) [[unlikely]]
                return std::unexpected(std::string(magic_enum::enum_name(JsonError::Parse)) + ": parse error or not object");

            if (!d.HasMember("schema") || !d["schema"].IsObject()) [[unlikely]]
                return std::unexpected(std::string(magic_enum::enum_name(JsonError::BadSchema)) + ": missing/invalid 'schema'");
            if (!d.HasMember("rows") || !d["rows"].IsArray()) [[unlikely]]
                return std::unexpected(std::string(magic_enum::enum_name(JsonError::BadRows)) + ": missing/invalid 'rows'");

            const auto& schema = d["schema"];
            if (!schema.HasMember("fields") || !schema["fields"].IsArray()) [[unlikely]]
                return std::unexpected(std::string(magic_enum::enum_name(JsonError::BadSchema)) + ": missing/invalid 'fields'");

            Clear();

            const auto& fields = schema["fields"].GetArray();
            uint32_t offset = 0;
            m_kHeader.m_iFieldCount = static_cast<uint32_t>(fields.Size());
            m_kBox.reserve(m_kHeader.m_iFieldCount);

            for (rapidjson::SizeType i = 0; i < fields.Size(); ++i)
            {
                const auto& f = fields[i];
                if (!f.IsObject()) [[unlikely]]
                    return std::unexpected(std::string(magic_enum::enum_name(JsonError::FieldEntry)));
                if (!f.HasMember("name") || !f["name"].IsString()) [[unlikely]]
                    return std::unexpected(std::string(magic_enum::enum_name(JsonError::MissingFieldProperty)) + ": name");
                if (!f.HasMember("type") || !f["type"].IsString()) [[unlikely]]
                    return std::unexpected(std::string(magic_enum::enum_name(JsonError::MissingFieldProperty)) + ": type");

                std::string name = f["name"].GetString();
                std::string typeStr = f["type"].GetString();
                bool isFloat = false; uint32_t sz = 0;
                auto ft = parse_type(typeStr, isFloat, sz);
                if (ft == FIELD_STRING)
                {
                    if (!f.HasMember("size") || !f["size"].IsUint()) [[unlikely]]
                        return std::unexpected(std::string(magic_enum::enum_name(JsonError::MissingFieldProperty)) + ": size for string");
                    sz = f["size"].GetUint();
                }
                if (sz == 0) [[unlikely]]
                    return std::unexpected(std::string(magic_enum::enum_name(JsonError::BadSize)));

                CDBField field(ft, sz, offset);
                field.SetName(name.c_str());
                m_kBox.push_back(field);
                offset += sz;
            }
            m_kHeader.m_iRowSize = offset;

            const auto& rows = d["rows"].GetArray();
            m_kHeader.m_iRowCount = static_cast<uint32_t>(rows.Size());
            m_DataStorage.resize(static_cast<size_t>(m_kHeader.m_iRowCount) * m_kHeader.m_iRowSize);

            for (rapidjson::SizeType r = 0; r < rows.Size(); ++r)
            {
                const auto& row = rows[r];
                if (!row.IsObject()) [[unlikely]]
                    return std::unexpected(std::string(magic_enum::enum_name(JsonError::BadRow)));
                uint8_t* base = m_DataStorage.data() + static_cast<size_t>(r) * m_kHeader.m_iRowSize;
                for (size_t i = 0; i < m_kBox.size(); ++i)
                {
                    const auto& f = m_kBox[i];
                    auto it = row.FindMember(f.m_szName);
                    if (it == row.MemberEnd()) [[unlikely]]
                        return std::unexpected(std::string(magic_enum::enum_name(JsonError::MissingValue)) + ": " + f.m_szName);
                    const auto& v = it->value;
                    uint8_t* dst = base + f.GetOffset();
                    switch (f.m_eType)
                    {
                    case FIELD_UINT8_T:
                    {
                        if (!v.IsUint()) [[unlikely]] return std::unexpected("uint8 expected");
                        uint8_t x = static_cast<uint8_t>(v.GetUint());
                        std::memcpy(dst, &x, 1);
                        break;
                    }
                    case FIELD_BOOL:
                    {
                        if (!v.IsBool() && !v.IsUint()) [[unlikely]] return std::unexpected("bool/uint expected");
                        bool b = v.IsBool() ? v.GetBool() : (v.GetUint() != 0);
                        std::memcpy(dst, &b, 1);
                        break;
                    }
                    case FIELD_UINT16_T:
                    {
                        if (!v.IsUint()) [[unlikely]] return std::unexpected("uint16 expected");
                        uint16_t x = static_cast<uint16_t>(v.GetUint());
                        std::memcpy(dst, &x, 2);
                        break;
                    }
                    case FIELD_ENUM:
                    case FIELD_UINT32_T:
                    {
                        if (!v.IsUint()) [[unlikely]] return std::unexpected("uint32 expected");
                        uint32_t x = v.GetUint();
                        std::memcpy(dst, &x, 4);
                        break;
                    }
                    default:
                    {
                        if (!v.IsString()) [[unlikely]] return std::unexpected("string expected");
                        const char* s = v.GetString();
                        const size_t len = std::min<size_t>(std::strlen(s), f.GetSize());
                        std::memset(dst, 0, f.GetSize());
                        std::memcpy(dst, s, len);
                        break;
                    }
                    }
                }
            }

            m_Data = { m_DataStorage.data(), m_DataStorage.size() };
            m_DataRows.clear();
            m_DataRows.reserve(m_kHeader.m_iRowCount);
            m_DataFieldNames.clear();
            for (uint32_t r = 0; r < m_kHeader.m_iRowCount; ++r)
            {
                boost::unordered_flat_map<std::string, CDBField> rowMap;
                rowMap.reserve(m_kBox.size());
                for (size_t i = 0; i < m_kBox.size(); ++i)
                {
                    auto field = m_kBox[i];
                    auto data = GetData(r, field.GetOffset(), field.GetSize());
                    CDBField clone(field);
                    clone.SetFieldData(data);
                    rowMap[field.GetName()] = clone;
                    if (m_DataFieldNames.size() != m_kBox.size()) m_DataFieldNames.push_back(field.GetName());
                }
                m_DataRows.push_back(std::move(rowMap));
            }
            return {};
        }();

    if (!expect) [[unlikely]] {
        return false;
    }
    return true;
}

bool CDBM::SaveJSON(const char* szPath, bool pretty) const
{
    using namespace rapidjson;

    auto expect = [&]() -> std::expected<void, std::string>
        {
            Document d; d.SetObject();
            auto& a = d.GetAllocator();

            Value schema(kObjectType);
            Value fields(kArrayType);
            for (size_t i = 0; i < m_kBox.size(); ++i)
            {
                const auto& f = m_kBox[i];
                Value fobj(kObjectType);
                fobj.AddMember("name", Value(f.m_szName, a), a);
                const char* ts = type_to_string(f.m_eType, false, f.GetSize());
                fobj.AddMember("type", Value(ts, a), a);
                if (f.m_eType == FIELD_STRING)
                    fobj.AddMember("size", f.GetSize(), a);
                fields.PushBack(fobj, a);
            }
            schema.AddMember("fields", fields, a);
            d.AddMember("schema", schema, a);

            Value rows(kArrayType);
            for (uint32_t r = 0; r < m_kHeader.m_iRowCount; ++r)
            {
                Value row(kObjectType);
                for (size_t i = 0; i < m_kBox.size(); ++i)
                {
                    const auto& f = m_kBox[i];
                    auto data = const_cast<CDBM*>(this)->GetData(r, f.GetOffset(), f.GetSize());
                    Value key(f.m_szName, a);
                    switch (f.m_eType)
                    {
                    case FIELD_UINT8_T:
                    {
                        uint8_t x = *reinterpret_cast<uint8_t*>(data.data());
                        Value v; v.SetUint(static_cast<unsigned>(x));
                        row.AddMember(key, v, a);
                        break;
                    }
                    case FIELD_BOOL:
                    {
                        bool b = *reinterpret_cast<bool*>(data.data());
                        Value v; v.SetBool(b);
                        row.AddMember(key, v, a);
                        break;
                    }
                    case FIELD_UINT16_T:
                    {
                        uint16_t x = *reinterpret_cast<uint16_t*>(data.data());
                        Value v; v.SetUint(static_cast<unsigned>(x));
                        row.AddMember(key, v, a);
                        break;
                    }
                    case FIELD_ENUM:
                    case FIELD_UINT32_T:
                    {
                        uint32_t x = *reinterpret_cast<uint32_t*>(data.data());
                        Value v; v.SetUint(x);
                        row.AddMember(key, v, a);
                        break;
                    }
                    default:
                    {
                        std::string s(reinterpret_cast<char*>(data.data()), reinterpret_cast<char*>(data.data()) + data.size());
                        s = trim_nulls(s);
                        Value sval; sval.SetString(s.c_str(), static_cast<SizeType>(s.size()), a);
                        row.AddMember(key, sval, a);
                        break;
                    }
                    }
                }
                rows.PushBack(row, a);
            }
            d.AddMember("rows", rows, a);

            std::ofstream ofs(szPath);
            if (!ofs) [[unlikely]]
                return std::unexpected(std::string(magic_enum::enum_name(JsonError::FileOpen)) + ": cannot open for write");
            OStreamWrapper osw(ofs);
            if (pretty)
            {
                PrettyWriter<OStreamWrapper> w(osw); w.SetIndent('\t', 1); d.Accept(w);
            }
            else
            {
                Writer<OStreamWrapper> w(osw); d.Accept(w);
            }
            return {};
        }();

    if (!expect) [[unlikely]] {
        return false;
    }
    return true;
}


