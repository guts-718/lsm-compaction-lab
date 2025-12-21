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

int main(){
    KVStore store("test.wal");
   
    for (int i = 0; i < 3000; i++) {
        store.put("k" + std::to_string(i), "v" + std::to_string(i));
    }
    
    store.flush_immutables();
    std::string v;
    std::cout << store.get("k42", v) << " " << v << "\n";
}