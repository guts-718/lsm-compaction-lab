#pragma once
#include "wal.h"
#include "memtable.h"
#include "sstable.h"
class KVStore {
    public:
        explicit KVStore(const std::string& wal_path);

        void put(const std::string& key, const std::string& value);
        void del(const std::string &key);
        bool get(const std::string &key, std::string& value_out);
        void flush_immutables();
        

    private:
        WriteAheadLog wal_;
        MemTable memtable_;
        SequenceNumber next_seq_;
        std::vector<MemTable>immutables_;
        std::vector<SSTable>l0_tables_;
        size_t memtable_limit_=1000;
    
};
