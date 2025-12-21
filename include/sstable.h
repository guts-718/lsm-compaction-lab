#pragma once
#include<string>
#include<vector>
#include<cstdint>
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
    
    private:
        std::string filename_;
        SSTableMeta meta_;
};