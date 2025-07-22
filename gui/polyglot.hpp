#ifndef POLYGLOT_HPP
#define POLYGLOT_HPP

#include "engine.h"
#include <cstdint>
#include <vector>

struct PolyglotEntry {
    uint64_t key;
    uint16_t move;
    uint16_t weight;
    uint32_t learn;

    [[nodiscard]] Move getMove() const;
};

class PolyglotBook {
  public:
    PolyglotBook(const uint8_t *data, size_t size);
    Move getMove(Position *p);

  private:
    std::vector<uint16_t> weights;
    std::vector<PolyglotEntry> entries;
};

#endif // POLYGLOT_HPP
