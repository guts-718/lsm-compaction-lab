#pragma once
#include<cstdint>
#include "sequence.h"
#include<string>

enum class RecordType: uint8_t{
    PUT=1,
    DELETE=2
};

struct LogRecord{
    RecordType type;
    SequenceNumber seq;
    std::string key;
    std::string value; // for DELETE record we will have this ""
};

