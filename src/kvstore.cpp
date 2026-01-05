#include "kvstore.h"
#include "sstable.h"
#include<iostream>

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
        std::cout << "L0 range: [" 
            << it->meta_.min_key << ", "
            << it->meta_.max_key << "]\n";

        if (!it->may_contain(key)) continue;
        if (it->get(key, value_out)) return true;
    }

    for (auto& t : l1_tables_) {
        if (!t.may_contain(key)) continue;
        if (t.get(key, value_out)) return true;
    }


    return false;
}



void KVStore::flush_immutables(){
    for(auto &mt: immutables_){
        auto sst = SSTable::create_from_memtable(
            "sst_l0_" + std::to_string(next_sstable_id_++) + ".db",mt
        );
        l0_tables_.push_back(sst);
    }
    if (l0_tables_.size() >= l0_compaction_threshold_) {
    compact_l0_to_l1();
    }   

    immutables_.clear();
}

void KVStore::compact_l0_to_l1() {
    // Merge newest → oldest
    std::map<std::string, MemEntry> merged;

    for (auto it = l0_tables_.rbegin(); it != l0_tables_.rend(); ++it) {
        auto entries = it->read_all();

        for (const auto& e : entries) {
            auto m = merged.find(e.key);
            if (m == merged.end() || e.seq > m->second.seq) {
                merged[e.key] = {
                    e.seq,
                    e.tombstone,
                    e.value
                };
            }
        }
    }

    // Drop tombstones
    for (auto it = merged.begin(); it != merged.end(); ) {
        if (it->second.tombstone)
            it = merged.erase(it);
        else
            ++it;
    }

    std::string filename =
        "sst_l1_" + std::to_string(l1_tables_.size()) + ".db";

    SSTable new_l1 =
        SSTable::create_from_entries(filename, merged);

    // Delete old L0 files
    for (auto& t : l0_tables_) {
        std::remove(t.filename_.c_str());
    }

    l0_tables_.clear();
    l1_tables_.push_back(new_l1);
}
