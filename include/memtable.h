#pragma once
#include<map>
#include<string>
#include "sequence.h"

struct MemEntry {
    SequenceNumber seq;
    bool tombstone;
    std::string value;
};

class MemTable {
    public:
        void put(const std::string& key, const std::string& value, SequenceNumber seq);

        void del(const std::string &key, SequenceNumber seq);

        bool get(const std::string &key, std::string& value_out) const;

        size_t size() const;
        // the map should be private....
        std::map<std::string, MemEntry> table_;
};