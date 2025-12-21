#include "wal.h"
#include "platform_io.h"
#include<sys/stat.h>
#include<cstring>
#include<stdexcept>

// chksum detects corrupted and partially wriitten records
static uint32_t compute_crc(const char* data, size_t len){
    uint32_t crc=0;

    for(size_t i=0;i<len;i++){
        crc=crc*31 + static_cast<uint8_t>(data[i]);
    }
    return crc;
}



WriteAheadLog::WriteAheadLog(const std::string& filename): filename_(filename),bytes_written_(0){
    fd_ = open_file_append(filename_);
    if(fd_ <0){
        throw std::runtime_error("Failed to open write ahead log");
    }
}

WriteAheadLog:: ~WriteAheadLog(){
    close_file(fd_);
}


void WriteAheadLog::append(const LogRecord& record){
    std::vector<char>payload;

    uint8_t type = static_cast<uint8_t>(record.type);
    uint32_t key_len=static_cast<uint32_t>(record.key.size());
    uint32_t value_len=static_cast<uint32_t>(record.value.size());

    payload.insert(payload.end(), reinterpret_cast<char*>(&type), reinterpret_cast<char*>(&type)+sizeof(type));
    payload.insert(payload.end(),reinterpret_cast<char*>(&key_len), reinterpret_cast<char*>(&key_len)+sizeof(key_len));
    payload.insert(payload.end(),reinterpret_cast<char*>(&value_len),reinterpret_cast<char*>(&value_len)+sizeof(value_len));
    payload.insert(payload.end(), record.key.begin(),record.key.end());
    payload.insert(payload.end(),record.value.begin(),record.value.end());

    uint32_t record_size= static_cast<uint32_t>(payload.size());
    uint32_t crc = compute_crc(payload.data(),payload.size());

    write_file(fd_, reinterpret_cast<char*>(&crc),sizeof(crc));
    write_file(fd_, reinterpret_cast<char*>(&record_size),sizeof(record_size));
    write_file(fd_, payload.data(),payload.size());

    fsync_file(fd_);

    bytes_written_+=sizeof(crc)+sizeof(record_size)+payload.size();

}


std::vector<LogRecord> WriteAheadLog::replay(){
    std::vector<LogRecord> records;
    int fd = open_file_readonly(filename_);
    if(fd<0)return records;

    while(true){
        uint32_t crc=0;
        uint32_t record_size=1;

        if(read_file(fd, reinterpret_cast<char*>(&crc),sizeof(crc))!=sizeof(crc))break;
        if(read_file(fd, reinterpret_cast<char*>(&record_size),sizeof(record_size))!=sizeof(record_size))break;

        std::vector<char>payload(record_size);
        if(read_file(fd,payload.data(),record_size)!=record_size)break;
        uint32_t actual_crc=compute_crc(payload.data(),payload.size());
        if(actual_crc!=crc)break;

        size_t offset=0;
        uint8_t type=payload[offset];
        offset+=sizeof(uint8_t);

        uint32_t key_len;
        std::memcpy(&key_len, &payload[offset], sizeof(key_len));
        offset+=sizeof(key_len);

        uint32_t value_len;
        std::memcpy(&value_len, &payload[offset], sizeof(value_len));
        offset+=sizeof(value_len);

        std::string key(payload.data()+offset, key_len);
        offset+=key_len;

        std::string value(payload.data()+offset, value_len);

        records.push_back({
            static_cast<RecordType>(type),
            key,
            value
        });

        
    }
    close_file(fd);
    return records;
}