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
        void compact_l0_to_l1();

        std::vector<SSTable>l0_tables_; // for debugging purpose made it public
        // const std::vector<SSTable>& KVStore::l0_tables() const {
        //     return l0_tables_;
        // }


        

    private:
        WriteAheadLog wal_;
        MemTable memtable_;
        SequenceNumber next_seq_;
        std::vector<MemTable>immutables_;
        //std::vector<SSTable>l0_tables_;
        size_t memtable_limit_=3;
        size_t l0_compaction_threshold_ = 4;
        std::vector<SSTable> l1_tables_;
        uint64_t next_sstable_id_ = 0;



    
};
