/*
#include "wal.h"
#include<iostream>



int main(){
    WriteAheadLog wal("test.wal");

    for(int i=0;i<1000;i++){
        wal.append({
            RecordType::PUT,
            "key" + std::to_string(i),
            "value "+ std::to_string(i)
        });
    }
 
    wal.append({RecordType::DELETE, "key10",""});

    auto records=wal.replay();
    std::cout<<"Replayed records: "<<records.size()<<"\n";
    std::cout<<"WAL bytes written "<<wal.bytes_written()<<"\n";
}

*/
#include "kvstore.h"
#include<iostream>
#include <cassert>
void dump_sstable(const SSTable& sst, const std::string& name) {
    std::cout << "==== SSTable: " << name << " ====\n";
    auto entries = sst.read_all();

    for (const auto& e : entries) {
        std::cout
            << "key=" << e.key
            << " value=" << e.value
            << " seq=" << e.seq
            << " tombstone=" << e.tombstone
            << "\n";
    }
    std::cout << "============================\n";
}

int main() {
    KVStore store("test.wal");

    store.put("k1", "v1");
    store.put("k2", "v2");
    store.put("k3", "v3"); // triggers freeze
    store.put("k4", "v4");

    store.flush_immutables();

    // 🔍 DEBUG: inspect L0 SSTables
    int idx = 0;
    for (const auto& sst : store.l0_tables_) {
        dump_sstable(sst, "L0_" + std::to_string(idx++));
    }

    std::string v;
    if (store.get("k4", v))
        std::cout << "GET k4 = " << v << "\n";
    else
        std::cout << "GET k4 FAILED\n";

    if (store.get("k1", v))
        std::cout << "GET k1 = " << v << "\n";
    else
        std::cout << "GET k1 FAILED\n";
}
