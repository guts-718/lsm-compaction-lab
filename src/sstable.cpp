#include "sstable.h"
#include "memtable.h"
#include "platform_io.h"
#include <limits>

// entry in memtable - [key: [value | tombstone | seq_num]]
/*
struct SSTableEntry{
    std::string key;
    std::string value;
    bool tombstone;
    SequenceNumber seq;
};

struct SSTableMeta {
    std::string min_key;
    std::string max_key;
    uint64_t entry_count;
};

*/
SSTable SSTable::create_from_memtable(
    const std::string& filename,
    const MemTable& memtable){

        int fd = open_file_append(filename);
        SSTable table;
        table.filename_=filename;
        table.meta_.entry_count = 0;

        bool first=true;
        uint64_t offset = 0;
        size_t index_stride = 16;
        size_t i=0;

        for(const auto& [key, entry] : memtable.table_){
            
            if(i%index_stride==0){
                table.index_[key]=offset;
            }
            
            uint32_t key_len = key.size();
            uint32_t value_len = entry.value.size();
            uint8_t tombstone = entry.tombstone ? 1:0;
            uint64_t seq = entry.seq;

            write_file(fd, reinterpret_cast<char*>(&key_len), sizeof(key_len));
            write_file(fd, key.data(),key.size());
            write_file(fd, reinterpret_cast<char*>(&value_len), sizeof(value_len));
            write_file(fd, entry.value.data(),entry.value.size());
            write_file(fd, reinterpret_cast<char*>(&tombstone),sizeof(tombstone));  // & caused so much trouble here!!
            write_file(fd, reinterpret_cast<char*>(&seq), sizeof(seq)); // & caused so much trouble here!!
            
            offset+=sizeof(key_len)+key_len+sizeof(value_len)+value_len+sizeof(tombstone)+sizeof(seq);
            
            if(i==0)table.meta_.min_key=key;
            table.meta_.max_key=key;
            table.meta_.entry_count++;
            i++;
        }

        fsync_file(fd);
        close_file(fd);

        return table;

    }

bool SSTable::get(const std::string& key, std::string& value_out) const {
    // Range pruning
    if (!may_contain(key)) return false;

    int fd = open_file_readonly(filename_);
    if (fd < 0) return false;

    // Find nearest indexed offset <= key
    uint64_t start_offset = 0;
    for (const auto& [k, off] : index_) {
        if (k <= key && off > start_offset) {
            start_offset = off;
        }
    }

    seek_file(fd, start_offset);

    while (true) {
        uint32_t key_len;
        if (read_file(fd, reinterpret_cast<char*>(&key_len), sizeof(key_len)) != sizeof(key_len))
            break;

        std::string k(key_len, '\0');
        if (read_file(fd, k.data(), key_len) != key_len)
            break;

        uint32_t value_len;
        if (read_file(fd, reinterpret_cast<char*>(&value_len), sizeof(value_len)) != sizeof(value_len))
            break;

        std::string v;
        if (value_len > 0) {
            v.resize(value_len);
            if (read_file(fd, v.data(), value_len) != value_len)
                break;
        }

        uint8_t tombstone;
        if (read_file(fd, reinterpret_cast<char*>(&tombstone), sizeof(tombstone)) != sizeof(tombstone))
            break;

        uint64_t seq;
        if (read_file(fd, reinterpret_cast<char*>(&seq), sizeof(seq)) != sizeof(seq))
            break;

        // Since SSTable is sorted by key, we can early-exit
        if (k > key) {
            break;
        }

        if (k == key) {
            close_file(fd);
            if (tombstone) return false;
            value_out = v;
            return true;
        }
    }

    close_file(fd);
    return false;
}

bool SSTable::may_contain(const std::string& key) const {
    return key >= meta_.min_key && key <= meta_.max_key;
}

// std::vector<SSTableEntry> SSTable::read_all() const {
//     std::vector<SSTableEntry> entries;

//     int fd = open_file_readonly(filename_);
//     if (fd < 0) return entries;

//     while (true) {
//         uint32_t key_len;
//         if (read_file(fd, reinterpret_cast<char*>(&key_len), sizeof(key_len)) != sizeof(key_len))
//             break;

//         std::string key(key_len, '\0');
//         read_file(fd, key.data(), key_len);

//         uint32_t value_len;
//         read_file(fd, reinterpret_cast<char*>(&value_len), sizeof(value_len));

//         std::string value;
//         if (value_len > 0) {
//             value.resize(value_len);
//             read_file(fd, value.data(), value_len);
//         }


//         uint8_t tombstone;
//         read_file(fd, reinterpret_cast<char*>(&tombstone), sizeof(tombstone));

//         uint64_t seq;
//         read_file(fd, reinterpret_cast<char*>(&seq), sizeof(seq));

//         entries.push_back({
//             key,
//             value,
//             tombstone == 1,
//             seq
//         });
//     }

//     close_file(fd);
//     return entries;
// }



std::vector<SSTableEntry> SSTable::read_all() const {
    std::vector<SSTableEntry> entries;

    int fd = open_file_readonly(filename_);
    if (fd < 0) return entries;

    while (true) {
        uint32_t key_len;
        if (read_file(fd, reinterpret_cast<char*>(&key_len), sizeof(key_len)) != sizeof(key_len))
            break;

        std::string key(key_len, '\0');
        if (read_file(fd, key.data(), key_len) != key_len)
            break;

        uint32_t value_len;
        if (read_file(fd, reinterpret_cast<char*>(&value_len), sizeof(value_len)) != sizeof(value_len))
            break;

        std::string value;
        if (value_len > 0) {
            value.resize(value_len);
            if (read_file(fd, value.data(), value_len) != value_len)
                break;
        }

        uint8_t tombstone;
        if (read_file(fd, reinterpret_cast<char*>(&tombstone), sizeof(tombstone)) != sizeof(tombstone))
            break;

        uint64_t seq;
        if (read_file(fd, reinterpret_cast<char*>(&seq), sizeof(seq)) != sizeof(seq))
            break;

        entries.push_back({
            key,
            value,
            tombstone == 1,
            seq
        });
    }

    close_file(fd);
    return entries;
}



SSTable SSTable::create_from_entries(
    const std::string& filename,
    const std::map<std::string, MemEntry>& entries) {

    int fd = open_file_append(filename);

    SSTable table;
    table.filename_ = filename;
    table.meta_.entry_count = 0;

    bool first = true;
    uint64_t offset = 0;
    size_t index_stride = 16;
    size_t i = 0;

    for (const auto& [key, entry] : entries) {
        if (i % index_stride == 0) {
            table.index_[key] = offset;
        }

        uint32_t key_len = key.size();
        uint32_t value_len = entry.value.size();
        uint8_t tombstone = entry.tombstone ? 1 : 0;
        uint64_t seq = entry.seq;

        write_file(fd, reinterpret_cast<char*>(&key_len), sizeof(key_len));
        write_file(fd, key.data(), key.size());

        write_file(fd, reinterpret_cast<char*>(&value_len), sizeof(value_len));
        write_file(fd, entry.value.data(), entry.value.size());

        write_file(fd, reinterpret_cast<char*>(&tombstone), sizeof(tombstone));
        write_file(fd, reinterpret_cast<char*>(&seq), sizeof(seq));

        offset += sizeof(key_len) + key_len
                + sizeof(value_len) + value_len
                + sizeof(tombstone)
                + sizeof(seq);

        if (first) {
            table.meta_.min_key = key;
            first = false;
        }
        table.meta_.max_key = key;
        table.meta_.entry_count++;
        i++;
    }

    fsync_file(fd);
    close_file(fd);
    return table;
}
