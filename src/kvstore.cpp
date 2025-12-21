#include "kvstore.h"

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


bool KVStore::get(const std::string& key, std::string& value_out){
    return memtable_.get(key,value_out);
}
