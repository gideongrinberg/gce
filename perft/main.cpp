#include "engine.h"
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#define COLOR_RED "\x1b[31m"
#define COLOR_GREEN "\x1b[32m"
#define COLOR_RESET "\x1b[0m"
#define COLOR_BOLD "\x1b[1m"
#define COLOR_UNDERLINE "\x1b[4m"
#define SC(s) (s ? COLOR_GREEN : COLOR_RED)

int perft(Position *p, int depth) {
    if (depth == 0)
        return 1;

    int total = 0;
    Move moves[256];
    int count = generate_moves(p, moves);

    for (int i = 0; i < count; i++) {
        Position copy = *p;
        execute_move(&copy, moves[i]);
        total += perft(&copy, depth - 1);
    }

    return total;
}

int perftDivide(Position *p, int depth) {
    int total = 0;
    Move moves[256];
    int count = generate_moves(p, moves);

    for (int i = 0; i < count; i++) {
        Position copy = *p;
        execute_move(&copy, moves[i]);
        int nodes = perft(&copy, depth - 1);

        int from = MOVE_FROM(moves[i]);
        int to = MOVE_TO(moves[i]);

        std::string moveStr;
        moveStr += static_cast<char>('a' + (from % 8));
        moveStr += static_cast<char>('1' + (from / 8));
        moveStr += static_cast<char>('a' + (to % 8));
        moveStr += static_cast<char>('1' + (to / 8));

        int promo = MOVE_PROMO(moves[i]);
        if (promo != 0) {
            switch (promo) {
            case PIECE_QUEEN:
                moveStr += 'q';
                break;
            case PIECE_ROOK:
                moveStr += 'r';
                break;
            case PIECE_BISHOP:
                moveStr += 'b';
                break;
            case PIECE_KNIGHT:
                moveStr += 'n';
                break;
            }
        }

        printf("%s %d\n", moveStr.c_str(), nodes);
        total += nodes;
    }

    return total;
}

void runPerftree(int argc, char **argv) {
    int depth = atoi(argv[1]);
    const char *fen = argv[2];
    char *movesArg = argv[3];

    Position *p = position_from_fen(fen);

    if (strlen(movesArg) > 0) {
        char buffer[1024];
        strncpy(buffer, movesArg, sizeof(buffer));
        buffer[sizeof(buffer) - 1] = '\0';

        char *token = strtok(buffer, " ");
        while (token != NULL) {
            int fromFile = token[0] - 'a';
            int fromRank = token[1] - '1';
            int toFile = token[2] - 'a';
            int toRank = token[3] - '1';

            // int from = (7 - fromRank) * 8 + fromFile;
            // int to = (7 - toRank) * 8 + toFile;
            int from = (fromRank) * 8 + fromFile;
            int to = (toRank) * 8 + toFile;

            int promo = 0;
            if (strlen(token) == 5) {
                switch (token[4]) {
                case 'q':
                    promo = PIECE_QUEEN;
                    break;
                case 'r':
                    promo = PIECE_ROOK;
                    break;
                case 'b':
                    promo = PIECE_BISHOP;
                    break;
                case 'n':
                    promo = PIECE_KNIGHT;
                    break;
                default:
                    break;
                }
            }

            Move move = ENCODE_MOVE(from, to, promo);
            execute_move(p, move);
            token = strtok(NULL, " ");
        }
    }

    int total = perftDivide(p, depth);
    printf("\n%d", total);
    fflush(stdout);
}

class PerftTest {
  public:
    std::string name;
    std::string fen;
    std::vector<int> expected;

    PerftTest(std::string name, std::string fen, std::vector<int> expected)
        : name(std::move(name)), fen(std::move(fen)),
          expected(std::move(expected)) {}
};

class PerftResult {
  public:
    bool status;
    int totalNodes;
    double totalTime;
    std::string name;
    std::string fen;
    std::vector<int> expected;

    PerftResult(bool status, int totalNodes, double totalTime,
                const PerftTest &test)
        : status(status), totalNodes(totalNodes), totalTime(totalTime),
          name(test.name), fen(test.fen), expected(test.expected) {}
};

// some utilities for testing
extern "C" {
double getTime(clockid_t clk_id) {
    struct timespec ts;
    clock_gettime(clk_id, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}
}

std::string trim(const std::string &s) {
    size_t start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos)
        return "";

    size_t end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

PerftResult runTest(PerftTest *test) {
    bool success = true;
    double totalWall = 0;
    int totalNodes = 0;
    int maxDepth = test->expected.size();

    Position *p = position_from_fen(test->fen.c_str());
    std::cout << COLOR_BOLD << COLOR_UNDERLINE << "Test " << test->name << ":"
              << COLOR_RESET << std::endl;
    std::cout << COLOR_BOLD << "FEN:" << COLOR_RESET << " " << test->fen
              << std::endl;
    std::cout << COLOR_BOLD << "Max depth: " << COLOR_RESET << maxDepth
              << std::endl;

    for (int i = 0; i < test->expected.size(); i++) {
        int depth = i + 1;
        double wallStart = getTime(CLOCK_MONOTONIC);
        int result = perft(p, depth);
        double wallEnd = getTime(CLOCK_MONOTONIC);
        double wallTime = wallEnd - wallStart;
        totalWall += wallTime;

        const bool ts = result == test->expected[i];
        totalNodes += test->expected[i];
        success &= ts;
        std::string nodes = result > 1 ? "nodes" : "node";
        std::cout << SC(ts) << "Depth " << i << ": found " << result << " "
                  << nodes << ", expected " << test->expected[i]
                  << " (wall: " << (wallEnd - wallStart) << "s)" << COLOR_RESET
                  << std::endl;
    }

    std::string finalStatus = success ? "succeeded" : "failed";
    std::cout << "\n," << COLOR_BOLD << SC(success) << "Test " << test->name
              << " " << finalStatus << " in " << totalWall << " seconds"
              << COLOR_RESET << "\n"
              << std::endl;

    return PerftResult(success, totalNodes, totalWall, *test);
}

int runSuite(std::vector<std::string> args) {
    std::string path = args[2];
    std::ofstream failing;
    if (args.size() >= 5) {
        failing = std::ofstream(args[4], std::ios::out);
        if (!failing.is_open()) {
            std::cerr << COLOR_BOLD << COLOR_UNDERLINE << "Failed to open file"
                      << COLOR_RESET << "\n";
        }
    }

    std::cout << std::fixed << std::setprecision(4);
    std::ifstream file(path);
    if (!file) {
        std::cerr << "Failed to open file.\n";
        return 1;
    }

    std::vector<PerftTest> suite;
    std::string line;
    int tests = 1;
    while (std::getline(file, line)) {
        std::string fen;
        std::vector<int> expected;
        expected.reserve(5);

        std::istringstream iss(line);
        std::string chunk;
        std::getline(iss, fen, ';');
        while (std::getline(iss, chunk, ';')) {
            std::string moves = chunk.substr(3);
            moves = trim(moves);
            expected.push_back(std::stoi(moves));
        }

        suite.emplace_back(std::to_string(tests++), fen, expected);
    }

    std::cout << "Found " << suite.size() << " tests.\n\n";
    std::vector<PerftResult> results;
    results.reserve(suite.size());
    for (auto &t : suite) {
        auto result = runTest(&t);
        if (!result.status && failing.is_open()) {
            failing << t.fen << std::endl;
        }

        results.push_back(result);
    }

    double totalTime = 0;
    double totalNodes = 0;
    bool success = true;
    int numSuccess = 0;
    int totalTests = 0;
    for (auto &r : results) {
        totalTime += r.totalTime;
        totalNodes += r.totalNodes;
        success &= r.status;
        if (r.status) {
            numSuccess++;
        }
        totalTests++;
    }

    std::cout << SC(success) << numSuccess << "/" << totalTests
              << " tests passed." << "\n";
    std::cout << COLOR_RESET << "Average NPS: " << COLOR_RESET
              << totalNodes / totalTime << ".\n";

    return success ? 0 : -1;
}

int main(int argc, char **argv) {
    if (argc >= 3 && strcmp(argv[1], "--suite") == 0) {
        return runSuite(std::vector<std::string>(argv, argv + argc));
    }

    if (argc == 4) {
        runPerftree(argc, argv);
        return 0;
    }

    if (argc == 3) {
        char *newArgv[4] = {argv[0], argv[1], argv[2], const_cast<char *>("")};
        runPerftree(argc, newArgv);
    }

    return 0;
}
