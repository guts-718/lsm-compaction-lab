#include "kvstore.h"
#include "sstable.h"

KVStore::KVStore(const std::string& wal_path):wal_(wal_path),next_seq_(1){

    auto records=wal_.replay();
    for(const auto& r:records){
        next_seq_=std::max(next_seq_,r.seq+1);

        if(r.type == RecordType::PUT){
            memtable_.put(r.key, r.value, r.seq);
        }else{
            memtable_.del(r.key,r.seq);
        }
    }
}



void KVStore::put(const std::string &key, const std::string& value){
    LogRecord r{
        RecordType::PUT,
        next_seq_,
        key,
        value
    };

    wal_.append(r);
    memtable_.put(key,value,next_seq_);
    next_seq_++;

    if(memtable_.size() >= memtable_limit_){
        immutables_.push_back(memtable_);
        memtable_=MemTable{};
    }
}

void KVStore::del(const std::string& key){
    LogRecord r{
        RecordType::DELETE,
        next_seq_,
        key,
        ""
    };
    wal_.append(r);
    memtable_.del(key,next_seq_);
    next_seq_++;
}

// immutable is in memory, readable, not accepting writes and waiting to be flushed to an sstable
bool KVStore::get(const std::string& key, std::string& value_out) {
    // from active memtable
    if (memtable_.get(key, value_out)) return true;
    // from immutable memtables (recently frozen but not yet flushed)
    for (auto it = immutables_.rbegin(); it != immutables_.rend(); ++it) {
        if (it->get(key, value_out)) return true;
    }
    // sstable
    for (auto it = l0_tables_.rbegin(); it != l0_tables_.rend(); ++it) {
        if (!it->may_contain(key)) continue;
        if (it->get(key, value_out)) return true;
    }



    return false;
}



void KVStore::flush_immutables(){
    for(auto &mt: immutables_){
        auto sst = SSTable::create_from_memtable(
            "sst_l0_" + std::to_string(l0_tables_.size())+ ".db",mt
        );
        l0_tables_.push_back(sst);
    }
    immutables_.clear();
}


