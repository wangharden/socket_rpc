#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <cstdint>
#include "Logger.h"
#include "ReflectStatic.h"

#define TYPE_LEN 1                                      //data type byte
#define SIZE_LEN sizeof(int)                            //data len type byte
#define HEAD_LEN (TYPE_LEN + SIZE_LEN)                  //data len byte


void TestProto();

namespace SockProto
{
    //Encode functions register begin
    enum
    {
        EMPTY = 0,
        INT32 = 1,
        INT64 = 2,
        DOUBLE = 3,
        STRING = 4,
        BOOLEAN = 5,
        FLOAT = 6,
    };
    typedef std::tuple<
        void,
        int,
        int64_t, 
        double, 
        std::string, 
        bool, 
        float
    > ProtoTypeList;

    
    std::string Encode(const int& data);
    std::string Encode(const int64_t& data);
    std::string Encode(const double& data);
    std::string Encode(const std::string& data);
    std::string Encode(const char* data);
    std::string Encode(const bool& data);
    std::string Encode(const float& data);
    //Encode functions register end


    template<class T, class ...Args>
    std::string Encode(T t, Args ...args)
    {
        std::string d0 = Encode(t);
        if constexpr (sizeof...(args) > 0)
        {
            std::string d1 = Encode(args...);
            d0 += d1;
        }
        return d0;
    }

    int GetHeadDataLen(std::string_view data);
    std::string GetHeadOne(const std::string& data);
    void RemoveHeadOne(std::string& data);
    void RemoveHeadOne(std::string_view& data);

    int ByteToInt32(std::string_view data);
    std::string Int32ToByte(int num);
    int64_t ByteToInt64(std::string_view data);
    std::string Int64ToByte(int64_t num);

    //Decode function register begin
    bool Decode(std::string_view& data, int& t);

    bool Decode(std::string_view& data, std::string& t);

    bool Decode(std::string_view& data, int64_t& t);

    bool Decode(std::string_view& data, double& t);

    bool Decode(std::string_view& data, bool& t);

    bool Decode(std::string_view& data, float& t);

    bool Decode(std::string_view data, std::vector<SReflect::Any>& args);
    //Decode function register end
    bool Slice(const std::string_view& data, std::vector<std::string_view>& args);

    template<class T, class... Args>
    bool Decode(std::string_view data, T& t, Args& ...args)
    {
        bool first_success = Decode(data, t);
        bool rest_success = true;
        if constexpr (sizeof...(args) > 0)
        {
            RemoveHeadOne(data);
            rest_success = Decode(data, args...);
        }
        return first_success && rest_success;
    }
}