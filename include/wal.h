#pragma once
#include "record.h"
#include<string>
#include<vector>
#include<cstdint>

class WriteAheadLog{
    public:
    explicit WriteAheadLog(const std::string &filename);
    ~WriteAheadLog();
    
    void append(const LogRecord& record);

    std::vector<LogRecord>replay();

    uint64_t bytes_written() const { return bytes_written_; };

    private:
    std::string filename_;
    int fd_;
    uint64_t bytes_written_;
};