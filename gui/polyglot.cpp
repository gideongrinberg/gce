#include "polyglot.hpp"

#include <random>

uint16_t read_be16(const uint8_t *buf) { return (buf[0] << 8) | buf[1]; }

uint32_t read_be32(const uint8_t *buf) {
    return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
}

uint64_t read_be64(const uint8_t *buf) {
    return (static_cast<uint64_t>(buf[0]) << 56) |
           (static_cast<uint64_t>(buf[1]) << 48) |
           (static_cast<uint64_t>(buf[2]) << 40) |
           (static_cast<uint64_t>(buf[3]) << 32) |
           (static_cast<uint64_t>(buf[4]) << 24) |
           (static_cast<uint64_t>(buf[5]) << 16) |
           (static_cast<uint64_t>(buf[6]) << 8) |
           (static_cast<uint64_t>(buf[7]));
}

PolyglotBook::PolyglotBook(const uint8_t *data, size_t size) {
    size_t offset = 0;
    while (offset + 16 <= size) {
        entries.emplace_back(
            read_be64(data + offset), read_be16(data + offset + 8),
            read_be16(data + offset + 10), read_be32(data + offset + 12));

        offset += 16;
    }

    for (const auto &entry : entries) {
        weights.push_back(entry.weight);
    }
}

Move PolyglotBook::getMove(Position *p) {
    uint64_t hash = position_zobrist(p);
    int l = 0, r = entries.size() - 1;
    int index = -1;
    int end = 0;
    while (l <= r) {
        int m = l + (r - l) / 2;
        if (entries[m].key < hash) {
            l = m + 1;
        } else if (entries[m].key > hash) {
            r = m - 1;
        } else if (entries[m].key == hash) {
            index = m;
            while (index > 0 && entries[index - 1].key == hash) {
                index--;
            }
            break;
        }
    }

    while ((index + end) < entries.size() && entries[index + end].key == hash) {
        ++end;
    }

    if (index == -1 || end == 0)
        return 0;

    static std::mt19937 gen(std::random_device{}());
    std::discrete_distribution<> dist(weights.begin() + index,
                                      weights.begin() + index + end);

    int selected = index + dist(gen);
    PolyglotEntry e = entries[selected];
    return e.getMove();
}

Move PolyglotEntry::getMove() const {
    const int toFile = move & 0b111;
    const int toRank = (move >> 3) & 0b111;
    const int fromFile = (move >> 6) & 0b111;
    const int fromRank = (move >> 9) & 0b111;
    const int promo = (move >> 12) & 0b111;

    return ENCODE_MOVE(fromRank * 8 + fromFile, toRank * 8 + toFile, promo);
}
