#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <cstdint>

#include "crc32.hpp"

enum class OpType : uint8_t { INSERT=1, ERASE=2, REPLACE=3 };

struct Op {
    uint64_t seq = 0;
    OpType type;
    uint32_t pos = 0;   // byte offset (UTF-8 safe)
    uint32_t len = 0;   // for ERASE/REPLACE
    std::string text;   // for INSERT/REPLACE
    uint32_t doc_crc32 = 0; // CRC after applying
};

class Document {
    std::string content;
    uint64_t next_seq = 1;

public:
    Document() = default;

    const std::string& get() const { return content; }
    uint64_t get_seq() const { return next_seq-1; }

    Op apply(const Op& op_in);
    Op make_insert(uint32_t pos, const std::string& text);
    Op make_erase(uint32_t pos, uint32_t len);
    Op make_replace(uint32_t pos, uint32_t len, const std::string& text);

    static void append_to_oplog(const std::string& path, const Op& op);
    static std::vector<Op> load_oplog(const std::string& path);
    static Document replay_from_log(const std::string& path);
};
