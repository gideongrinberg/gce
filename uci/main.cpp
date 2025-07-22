#include "engine.h"
#include "logger.hpp"
#include <cstdlib>
#include <deque>
#include <iostream>
#include <string>
#include <vector>

static auto logger = Logger(std::string(getenv("HOME")) + "/gce-uci.log");

void flush() { std::cout << std::flush; }
void sendMessage(const char *fmt, ...) {
    char buf[1024];

    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    std::cout << buf << "\n";
    logger.log("Sent message: ", buf);
}

std::vector<std::string> splitStr(const std::string &input) {
    std::istringstream stream(input);
    std::string word;
    std::vector<std::string> result;

    while (stream >> word) {
        result.push_back(word);
    }

    return result;
}

Move parseMove(std::string moveStr) {
    int fromFile = moveStr[0] - 'a', fromRank = moveStr[1] - '1',
        toFile = moveStr[2] - 'a', toRank = moveStr[3] - '1';
    int promo = 0;
    if (moveStr.size() > 4) {
        switch (moveStr[4]) {
        case 'n':
            promo = PIECE_KNIGHT;
            break;
        case 'b':
            promo = PIECE_BISHOP;
            break;
        case 'r':
            promo = PIECE_ROOK;
            break;
        case 'q':
            promo = PIECE_QUEEN;
            break;
        }
    }

    return ENCODE_MOVE(fromRank * 8 + fromFile, toRank * 8 + toFile, promo);
}

std::string formatMove(Move move) {
    std::stringstream ss;
    int moveFrom = MOVE_FROM(move), moveTo = MOVE_TO(move);
    int fromRank = moveFrom / 8, fromFile = moveFrom % 8;
    int toRank = moveTo / 8, toFile = moveTo % 8;

    // Correct conversion: file = 'a' + X, rank = '1' + Y
    ss << static_cast<char>('a' + fromFile) << static_cast<char>('1' + fromRank)
       << static_cast<char>('a' + toFile) << static_cast<char>('1' + toRank);

    switch (MOVE_PROMO(move)) {
    case PIECE_KNIGHT:
        ss << 'n';
        break;
    case PIECE_BISHOP:
        ss << 'b';
        break;
    case PIECE_ROOK:
        ss << 'r';
        break;
    case PIECE_QUEEN:
        ss << 'q';
        break;
    }

    return ss.str();
}

Position *parsePosition(std::string input) {
    Position *position = nullptr;
    std::vector<std::string> argv = splitStr(input);
    auto args = std::deque(argv.begin(), argv.end());
    args.pop_front();
    if (args.front() == "startpos") {
        position = position_from_fen(
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        args.pop_front();
    } else if (args.front() == "fen") {
        args.pop_front();
        std::string fen;
        for (int i = 0; i < 6; i++) {
            fen += " ";
            fen += args.front();
            args.pop_front();
        }

        position = position_from_fen(fen.c_str());
    } else {
        logger.log("Unrecognized command, expected startpos or fen.");
        return nullptr;
    }

    if (!args.empty() && args.front() == "moves") {
        args.pop_front();
        while (!args.empty() && !args.front().empty()) {
            execute_move(position, parseMove(args.front()));
            args.pop_front();
        }
    }

    return position;
}

int main() {
    logger.log("Started");
    std::string input;
    Position *position = nullptr;

    while (std::getline(std::cin, input)) {
        logger.log("Got command: ", input.c_str());
        if (input == "quit") {
            logger.log("Goodbye");
            exit(0);
        }

        if (input == "uci") {
            sendMessage("id name Gideon's Chess Engine");
            sendMessage("id author Gideon Grinberg");
            sendMessage("uciok");
            flush();
        }

        if (input == "isready") {
            sendMessage("readyok");
            flush();
        }
        if (input.starts_with("position")) {
            position = parsePosition(input);
            if (!position)
                return -1;
        }

        if (input.starts_with("go")) {
            Move move = get_best_move(position, 7);
            Position copy = *position;
            execute_move(&copy, move);
            sendMessage("info depth 10 score cp %d", eval_position(&copy));
            sendMessage("bestmove %s", formatMove(move).c_str());
            flush();
        }
    }
}