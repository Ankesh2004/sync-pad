#include "document.hpp"
#include <sstream>
#include <stdexcept>

// --------- Apply operation ------------
Op Document::apply(const Op& op_in) {
    Op op = op_in; // copy (to set crc)
    if (op.seq == 0) op.seq = next_seq++;

    switch (op.type) {
        case OpType::INSERT:
            if (op.pos > content.size()) throw std::runtime_error("Insert out of bounds");
            content.insert(op.pos, op.text);
            break;

        case OpType::ERASE:
            if (op.pos + op.len > content.size()) throw std::runtime_error("Erase OOB");
            content.erase(op.pos, op.len);
            break;

        case OpType::REPLACE:
            if (op.pos + op.len > content.size()) throw std::runtime_error("Replace OOB");
            content.replace(op.pos, op.len, op.text);
            break;
    }
    op.doc_crc32 = crc32(content);
    if (op.seq >= next_seq) next_seq = op.seq + 1;
    return op;
}

// --------- Factory helpers ------------
Op Document::make_insert(uint32_t pos, const std::string& text) {
    Op op; op.type=OpType::INSERT; op.pos=pos; op.text=text;
    return apply(op);
}
Op Document::make_erase(uint32_t pos, uint32_t len) {
    Op op; op.type=OpType::ERASE; op.pos=pos; op.len=len;
    return apply(op);
}
Op Document::make_replace(uint32_t pos, uint32_t len, const std::string& text) {
    Op op; op.type=OpType::REPLACE; op.pos=pos; op.len=len; op.text=text;
    return apply(op);
}

// --------- Oplog persistence ------------
static std::string serialize(const Op& op) {
    std::ostringstream oss;
    oss << op.seq << "|" << int(op.type) << "|" 
        << op.pos << "|" << op.len << "|"
        << op.text << "|" << op.doc_crc32 << "\n";
    return oss.str();
}
static Op deserialize(const std::string& line) {
    std::stringstream ss(line);
    Op op; int t;
    std::string token;

    std::getline(ss, token, '|'); op.seq = std::stoull(token);
    std::getline(ss, token, '|'); t = std::stoi(token); op.type=(OpType)t;
    std::getline(ss, token, '|'); op.pos = std::stoul(token);
    std::getline(ss, token, '|'); op.len = std::stoul(token);
    std::getline(ss, token, '|'); op.text = token;
    std::getline(ss, token, '|'); op.doc_crc32 = std::stoul(token);

    return op;
}


void Document::append_to_oplog(const std::string& path, const Op& op) {
    std::ofstream f(path, std::ios::app);
    f << serialize(op);
}

std::vector<Op> Document::load_oplog(const std::string& path) {
    std::ifstream f(path);
    std::vector<Op> ops;
    std::string line;
    while (std::getline(f, line)) {
        if (!line.empty()) {
            ops.push_back(deserialize(line));
        }
    }
    return ops;
}

Document Document::replay_from_log(const std::string& path) {
    Document doc;
    for (auto& op : load_oplog(path)) {
        doc.apply(op);
    }
    return doc;
}
