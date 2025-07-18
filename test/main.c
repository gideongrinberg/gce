#include "engine.h"
#include "tinytest.h"

TEST(test_zobrist_hashing) {
    const char *fens[] = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1",
        "rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 2",
        "rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 2",
        "rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3",
        "rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPPKPPP/RNBQ1BNR b kq - 0 3",
        "rnbq1bnr/ppp1pkpp/8/3pPp2/8/8/PPPPKPPP/RNBQ1BNR w - - 0 4",
        "rnbqkbnr/p1pppppp/8/8/PpP4P/8/1P1PPPP1/RNBQKBNR b KQkq c3 0 3",
        "rnbqkbnr/p1pppppp/8/8/P6P/R1p5/1P1PPPP1/1NBQKBNR b Kkq - 0 4"};

    const uint64_t keys[] = {
        0x463b96181691fc9cULL, 0x823c9b50fd114196ULL, 0x0756b94461c50fb0ULL,
        0x662fafb965db29d4ULL, 0x22a48b5a8e47ff78ULL, 0x652a607ca3f242c1ULL,
        0x00fdd303c946bdd9ULL, 0x3c8123ea7b067637ULL, 0x5c3f9b829b279560ULL};

    int num_cases = sizeof(keys) / sizeof(keys[0]);
    for (int i = 0; i < num_cases; i++) {
        Position *p = position_from_fen(fens[i]);
        ASSERT_EQ(keys[i], position_zobrist(p));
    }
}

int main(void) {
    tinytest_run_all();
    return 0;
}