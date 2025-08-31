#include "../core/document.hpp"
#include <random>
#include <iostream>

int main() {
    std::string logpath = "oplog.log";
    std::ofstream clear(logpath, std::ios::trunc); // reset log

    Document doc;
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> coin(0, 4); // bias towards inserts

    for (int i = 0; i < 20; i++) {
        int choice = coin(rng);

        if (choice < 2 || doc.get().empty()) {
            // Insert at random position
            std::uniform_int_distribution<size_t> posdist(0, doc.get().size());
            size_t pos = posdist(rng);
            auto op = doc.make_insert(pos, "X");
            Document::append_to_oplog(logpath, op);
            std::cout << "Op " << i << ": INSERT @" << pos << " -> [" << doc.get() << "]\n";
        } 
        else if (choice == 2 && !doc.get().empty()) {
            // Erase at random position
            std::uniform_int_distribution<size_t> posdist(0, doc.get().size() - 1);
            size_t pos = posdist(rng);
            auto op = doc.make_erase(pos, 1);
            Document::append_to_oplog(logpath, op);
            std::cout << "Op " << i << ": ERASE @" << pos << " -> [" << doc.get() << "]\n";
        } 
        else if (choice > 2 && !doc.get().empty()) {
            // Replace at random position
            std::uniform_int_distribution<size_t> posdist(0, doc.get().size() - 1);
            size_t pos = posdist(rng);
            auto op = doc.make_replace(pos, 1, "Y");
            Document::append_to_oplog(logpath, op);
            std::cout << "Op " << i << ": REPLACE @" << pos << " -> [" << doc.get() << "]\n";
        }
    }

    std::cout << "Final doc: [" << doc.get() << "]\n";

    uint32_t crc1 = crc32(doc.get());

    // Replay from log
    Document doc2 = Document::replay_from_log(logpath);
    uint32_t crc2 = crc32(doc2.get());
    std::cout << "Replayed final doc: [" << doc2.get() << "]\n";

    if (crc1 == crc2) {
        std::cout << "Test passed! CRC=" << crc1 << "\n";
    } else {
        std::cerr << "Mismatch! " << crc1 << " vs " << crc2 << "\n";
        return 1;
    }
    return 0;
}
