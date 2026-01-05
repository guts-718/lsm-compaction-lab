#pragma once
#include<string>
#include<vector>
#include<cstdint>
#include<unordered_map>
#include "memtable.h"

struct SSTableEntry{
    std::string key;
    std::string value;
    bool tombstone;
    SequenceNumber seq;
};

struct SSTableMeta {
    std::string min_key;
    std::string max_key;
    uint64_t entry_count;
};

class SSTable {
    public:
            static SSTable create_from_memtable(
            const std::string& filename,
            const MemTable& memtable);

            static SSTable create_from_entries(
            const std::string& filename,
            const std::map<std::string, MemEntry>& entries);

            bool get(const std::string& key, std::string &value_out) const;
            bool may_contain(const std::string& key) const;
            std::vector<SSTableEntry> read_all() const;
            std::string filename_;
            SSTableMeta meta_; // this should be private btu for debugging sake made it public for now...

    
    private:
       
//        SSTableMeta meta_;

        std::unordered_map<std::string,uint64_t>index_;
};