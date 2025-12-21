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