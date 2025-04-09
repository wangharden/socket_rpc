#include "LightProto.h"

int SockProto::GetHeadDataLen(std::string_view data)
{
    std::string_view s(data.data() + TYPE_LEN, SIZE_LEN);
    return ByteToInt32(s);
}

std::string SockProto::GetHeadOne(const std::string& data)
{
    int size = GetHeadDataLen(data);
    return std::string(data.begin(), data.begin() + HEAD_LEN + size);
}

void SockProto::RemoveHeadOne(std::string& data)
{
    int size = GetHeadDataLen(data);
    data.erase(0, HEAD_LEN + size);
}

void SockProto::RemoveHeadOne(std::string_view& data)
{
    int len = GetHeadDataLen(data);
    data.remove_prefix(len + HEAD_LEN);
}

int SockProto::ByteToInt32(std::string_view data)
{
    if (data.size() != 4)
        return 0;
    int num = 0;
    num += ((unsigned char) data[0]) * 0x01000000;
    num += ((unsigned char) data[1]) * 0x00010000;
    num += ((unsigned char) data[2]) * 0x00000100;
    num += ((unsigned char) data[3]) * 0x00000001;
    return num;
}

std::string SockProto::Int32ToByte(int num)
{
    std::string data;
    data.resize(4);
    data[0] = (char)((num & 0xFF000000) >> 24);
    data[1] = (char)((num & 0x00FF0000) >> 16);
    data[2] = (char)((num & 0x0000FF00) >> 8);
    data[3] = (char)((num & 0x000000FF));
    return data;
}

int64_t SockProto::ByteToInt64(std::string_view data)
{
    int64_t ans = 0;
    int64_t multiplier = 0;
    const int64_t unit = 1;
    for (int i = 0; i < 8; ++i)
    {
        int digit = (7 - i) * 8;
        multiplier = unit << digit;
        ans += ((int64_t)data[i]) * multiplier;
    }
    return ans;
}

std::string SockProto::Int64ToByte(int64_t num)
{
    std::string data;
    data.resize(8);
    const int64_t d = 0xFF;
    for (int i = 0; i < 8; ++i)
    {
        int digit = (7 - i) * 8;
        int64_t k = d << digit;
        data[i] = (char)((num & k) >> digit);
    }
    return data;
}


std::string SockProto::Encode(const std::string& data)
{
    std::string s;
    s.push_back((char)STRING);
    s += Int32ToByte((int)data.size());
    s += data;
    return s;
}

std::string SockProto::Encode(const int& data)
{
    std::string s;
    s.push_back((char)INT32);
    s += Int32ToByte(sizeof(int));
    s += Int32ToByte(data);
    return s;
}

std::string SockProto::Encode(const int64_t& data)
{
    std::string s;
    s.push_back((char)INT64);
    s += Int32ToByte(sizeof(int64_t));
    s += Int64ToByte(data);
    return s;
}

std::string SockProto::Encode(const double& data)
{
    std::string s;
    s.push_back((char)DOUBLE);
    std::string val = std::to_string(data);
    s += Int32ToByte((int)val.size());
    s += val;
    return s;
}

std::string SockProto::Encode(const char* data)
{
    std::string s(data);
    return Encode(s);
}

std::string SockProto::Encode(const bool& data)
{
    std::string s;
    s.push_back((char)BOOLEAN);
    s += Int32ToByte(1);
    s.push_back((char)data);
    return s;
}

std::string SockProto::Encode(const float& data)
{
    std::string s;
    s.push_back((char)FLOAT);
    std::string val = std::to_string(data);
    s += Int32ToByte((int)val.size());
    s += val;
    return s;
}

bool SockProto::Decode(std::string_view& data, int& t)
{
    if (data.empty() || data[0] != INT32)
    {
        Log("Decode INT32 failed, type mismatch!\n");
        return false;
    }
    int size = GetHeadDataLen(data);
    t = ByteToInt32(std::string_view(data.data() + HEAD_LEN, size));
    return true;
}

bool SockProto::Decode(std::string_view& data, std::string& t)
{
    if (data.empty() || data[0] != STRING)
    {
        Log("Decode STRING failed, type mismatch!\n");
        return false;
    }
    int size = GetHeadDataLen(data);
    t.assign(data.begin() + HEAD_LEN, data.begin() + HEAD_LEN + size);
    return true;
}

bool SockProto::Decode(std::string_view& data, int64_t& t)
{
    if (data.empty() || data[0] != INT64)
    {
        Log("Decode INT64 failed, type mismatch!\n");
        return false;
    }
    int size = GetHeadDataLen(data);
    t = ByteToInt64(std::string_view(data.data() + HEAD_LEN, size));
    return true;
}

bool SockProto::Decode(std::string_view& data, double& t)
{
    if (data.empty() || data[0] != DOUBLE)
    {
        Log("Decode DOUBLE failed, type mismatch!\n");
        return false;
    }
    int size = GetHeadDataLen(data);
    std::string s(data.begin() + HEAD_LEN, data.begin() + HEAD_LEN + size);
    t = std::stod(s);
    return true;
}

bool SockProto::Decode(std::string_view& data, bool& t)
{
    if (data.empty() || data[0] != BOOLEAN)
    {
        Log("Decode BOOLEAN failed, type mismatch!\n");
        return false;
    }
    t = (bool)data[HEAD_LEN];
    return true;
}

bool SockProto::Decode(std::string_view& data, float& t)
{
    if (data.empty() || data[0] != FLOAT)
    {
        Log("Decode FLOAT failed, type mismatch!\n");
        return false;
    }
    int size = GetHeadDataLen(data);
    std::string s(data.begin() + HEAD_LEN, data.begin() + HEAD_LEN + size);
    t = std::stof(s);
    return true;
}

template<class T>
SReflect::Any TDecode(std::string_view& data)
{
    T t;
    bool res = SockProto::Decode(data, t);
    if (res)
    {
        return t;
    }
    Log("TDecode failed\n");
    return SReflect::Any{};
}


SReflect::Any AnyDecode(std::string_view& data)
{
    using namespace SReflect;
    char t = data[0];
    switch (t)
    {
    case SockProto::EMPTY:
        break;
    case SockProto::INT32:
        return TDecode<std::tuple_element_t<SockProto::INT32, SockProto::ProtoTypeList>>(data);
        break;
    case SockProto::INT64:
        return TDecode<std::tuple_element_t<SockProto::INT64, SockProto::ProtoTypeList>>(data);
        break;
    case SockProto::DOUBLE:
        return TDecode<std::tuple_element_t<SockProto::DOUBLE, SockProto::ProtoTypeList>>(data);
        break;
    case SockProto::STRING:
        return TDecode<std::tuple_element_t<SockProto::STRING, SockProto::ProtoTypeList>>(data);
        break;
    case SockProto::BOOLEAN:
        return TDecode<std::tuple_element_t<SockProto::BOOLEAN, SockProto::ProtoTypeList>>(data);
        break;
    case SockProto::FLOAT:
        return TDecode<std::tuple_element_t<SockProto::FLOAT, SockProto::ProtoTypeList>>(data);
    default:
        break;
    }
    return Any{};
}


bool SockProto::Decode(std::string_view data, std::vector<SReflect::Any>& args)
{
    std::vector<std::string_view> segments;
    Slice(data, segments);
    bool ans = true;
    for (auto& val : segments)
    {
        args.emplace_back(AnyDecode(val));
    }
    return ans;
}

bool SockProto::Slice(const std::string_view& data, std::vector<std::string_view>& args)
{
    int len = (int)data.size();
    if (len == 0)
    {
        return false;
    }
    int i = 0;
    int pieceLen = 0;
    while (i < len)
    {
        int j = i;
        j += TYPE_LEN;
        if (j + SIZE_LEN > len) 
        {
            return false;
        }
        pieceLen = ByteToInt32(std::string_view(data.data() + j, SIZE_LEN));
        j += SIZE_LEN;
        j += pieceLen;
        if (j > len)
        {
            return false;
        }
        args.push_back(std::string_view(data.data() + i, pieceLen + HEAD_LEN));
        i = j;
    }
    return true;
}

void TestProto()
{
    using namespace std;
    auto printMsg = [](const string& msg) {
        printf("After encode, it is ");
        for (auto ch : msg) {
            printf("%c", ch);
        }
        printf("\n");
    };
    string msg;
    
    int IntValue = -1223;
    printf("Test int32 encode: encode %d\n", IntValue);
    msg = SockProto::Encode(IntValue);
    printMsg(msg);

    int DecodeInt = 0;
    SockProto::Decode(msg, DecodeInt);
    printf("Test int32 decode: decode value is %d\n", DecodeInt);

    int64_t Int64Value = 0x132456446546465;
    printf("Test int64 encode: encode %ld\n", Int64Value);
    msg = SockProto::Encode(Int64Value);
    printMsg(msg);

    int64_t DecodeInt64 = 0;
    SockProto::Decode(msg, DecodeInt64);
    printf("Test int64 decode: decode value is %ld\n", DecodeInt64);

    double DoubleValue = 0.123546;
    printf("Test double encode: encode %f\n", DoubleValue);
    msg = SockProto::Encode(DoubleValue);
    printMsg(msg);

    double DecodeDouble = 0.;
    SockProto::Decode(msg, DecodeDouble);
    printf("Test double decode: decode value is %f\n", DecodeDouble);

    string StringValue = "hello light proto";
    printf("Test string encode: encode %s\n", StringValue.c_str());
    msg = SockProto::Encode(StringValue);
    printMsg(msg);

    string DecodeString;
    SockProto::Decode(msg, DecodeString);
    printf("Test string decode: decode value is %s\n", DecodeString.c_str());

}
