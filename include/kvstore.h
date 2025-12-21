#pragma once
#include "wal.h"
#include "memtable.h"

class KVStore {
    public:
        explicit KVStore(const std::string& wal_path);

        void put(const std::string& key, const std::string& value);
        void del(const std::string &key);
        bool get(const std::string &key, std::string& value_out);

    private:
        WriteAheadLog wal_;
        MemTable memtable_;
        SequenceNumber next_seq_;
    
};
