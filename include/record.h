#pragma once
#include<cstdint>
#include<string>

enum class RecordType: uint8_t{
    PUT=1,
    DELETE=2
};

struct LogRecord{
    RecordType type;
    std::string key;
    std::string value; // for DELETE record we will have this ""
};