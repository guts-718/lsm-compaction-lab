#pragma once
#include<string>
#include<vector>
#include<cstdint>
#include<unordered_map>
#include "memtable.h"

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

            bool get(const std::string& key, std::string &value_out) const;
            bool may_contain(const std::string& key) const;
    
    private:
        std::string filename_;
        SSTableMeta meta_;

        std::unordered_map<std::string,uint64_t>index_;
};