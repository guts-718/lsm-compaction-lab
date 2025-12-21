#include "sstable.h"
#include "memtable.h"
#include "platform_io.h"
#include<limits>


SSTable SSTable::create_from_memtable(
    const std::string& filename,
    const MemTable& memtable){

        int fd = open_file_append(filename);
        SSTable table;
        table.filename_=filename;
        table.meta_.entry_count = 0;

        bool first=true;
        uint64_t offset = 0;
        size_t index_stride = 16;
        size_t i=0;

        for(const auto& [key, entry] : memtable.table_){
            
            if(i%index_stride==0){
                table.index_[key]=offset;
            }
            
            uint32_t key_len = key.size();
            uint32_t value_len = entry.value.size();
            uint8_t tombstone = entry.tombstone ? 1:0;
            uint64_t seq = entry.seq;

            write_file(fd, reinterpret_cast<char*>(&key_len), sizeof(key_len));
            write_file(fd, key.data(),key.size());
            write_file(fd, reinterpret_cast<char*>(&value_len), sizeof(value_len));
            write_file(fd, entry.value.data(),entry.value.size());
            write_file(fd, reinterpret_cast<char*>(tombstone),sizeof(tombstone));
            write_file(fd, reinterpret_cast<char*>(seq), sizeof(seq));
            
            offset+=sizeof(key_len)+key_len+sizeof(value_len)+value_len+sizeof(tombstone)+sizeof(seq);
            
            if(i==0)table.meta_.min_key=key;
            table.meta_.max_key=key;
            table.meta_.entry_count++;
            i++;
        }

        fsync_file(fd);
        close_file(fd);

        return table;

    }


bool SSTable::get(const std::string& key, std::string& value_out) const {
    if(!may_contain(key))return false;
    int fd = open_file_readonly(filename_);
    if (fd < 0) return false;

    uint64_t start_offset=0;

    for(const auto& [k,off]: index_){
        if(k<= key && off > start_offset){
            start_offset=off;
        }
    }

    seek_file(fd, start_offset);


    while (true) {
        uint32_t key_len;
        if (read_file(fd, reinterpret_cast<char*>(&key_len), sizeof(key_len)) != sizeof(key_len))
            break;

        std::string k(key_len, '\0');
        read_file(fd, k.data(), key_len);

        uint32_t value_len;
        read_file(fd, reinterpret_cast<char*>(&value_len), sizeof(value_len));

        std::string v(value_len, '\0');
        read_file(fd, v.data(), value_len);

        uint8_t tombstone;
        read_file(fd, reinterpret_cast<char*>(&tombstone), sizeof(tombstone));

        uint64_t seq;
        read_file(fd, reinterpret_cast<char*>(&seq), sizeof(seq));

        if (k == key) {
            close_file(fd);
            if (tombstone) return false;
            value_out = v;
            return true;
        }
    }

    close_file(fd);
    return false;
}

bool SSTable::may_contain(const std::string& key) const {
    return key >= meta_.min_key && key <= meta_.max_key;
}
