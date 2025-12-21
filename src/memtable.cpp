#include "memtable.h"

void MemTable::put(const std::string& key, const std::string& value, SequenceNumber seq){
    table_[key]={seq, false, value};
}

void MemTable::del(const std::string& key, SequenceNumber seq){
    table_[key]={seq,true,""};
}

bool MemTable::get(const std::string& key, std::string& value_out) const {
    auto it=table_.find(key);
    if(it==table_.end())return false;
    if(it->second.tombstone)return false; //tombstone - true means deleted
    value_out=it->second.value;
    return true;
}

size_t MemTable::size() const {
    return table_.size();
}