"""
This code generates a header with precomputed tables for the chess engine. So far, it generates possible moves for
knights and kings on each of the 64 squares of the board, and magic bitboards for sliding pieces.
"""

import random
import time
from pathlib import Path


# Offset table generation


def generate_knight_moves():
    deltas = [(2, 1), (1, 2), (-1, 2), (-2, 1), (-2, -1), (-1, -2), (1, -2), (2, -1)]
    return generate_offset_moves("knight", deltas)


def generate_king_moves():
    deltas = [
        (-1, -1),
        (-1, 0),
        (-1, 1),
        (0, -1),
        (0, 1),
        (1, -1),
        (1, 0),
        (1, 1),
    ]

    return generate_offset_moves("king", deltas)


def generate_offset_moves(piece, deltas):
    all_moves = []
    for sq in range(64):
        moves = 0
        rank = sq // 8
        file = sq % 8
        for dr, df in deltas:
            r = rank + dr
            f = file + df
            if 0 <= r < 8 and 0 <= f < 8:
                to_sq = r * 8 + f
                moves |= 1 << to_sq
        all_moves.append(moves)
    return format_c_array(
        [f"0x{n:016X}ULL" for n in all_moves], f"{piece}_moves", "uint64_t"
    )


# Magic bitboard generation (sliding pieces)
def sparse_rand():
    return random.getrandbits(64) & random.getrandbits(64) & random.getrandbits(64)


class Slider:
    """Base class for sliding pieces used in magic bitboard generation"""

    def __init__(self):
        self.magic_list = None
        p = Path(__file__).parent / "magics" / f"{self.name()}.txt"
        if p.is_file():
            self.magic_list = list(p.read_text().splitlines())

    def name(self) -> str:
        raise NotImplementedError()

    def relevance_mask(self, sq: int) -> int:
        """Given a square, return a bitboard of relevant squares"""
        raise NotImplementedError()

    def attack_mask(self, sq: int, blockers: int) -> int:
        """Given a square and a bitboard of blocking pieces,
        return a mask of attacked squares."""
        raise NotImplementedError()

    def max_attacks(self) -> int:
        raise NotImplementedError()

    def generate_blockers(self, sq: int):
        """
        Generates each possible combination of blocker pieces
        based on a mask of relevant squares.
        """
        relevant_mask = self.relevance_mask(sq)
        relevant_bits = []
        for i in range(64):
            if (relevant_mask >> i) & 1:
                relevant_bits.append(i)

        blocker_configs = []
        n_configs = 1 << len(
            relevant_bits
        )  # the number of blocker configurations = number of bits^2
        for idx in range(n_configs):
            blockers = 0
            for j in range(len(relevant_bits)):
                if (idx >> j) & 1:
                    blockers |= 1 << relevant_bits[j]

            blocker_configs.append(blockers)

        return blocker_configs, len(relevant_bits)

    def test_magic(self, sq, magic) -> tuple[bool, dict[int, int]]:
        blockers_list, relevant_bits = self.generate_blockers(sq)
        table = {}
        collision = False
        for blockers_mask in blockers_list:
            product = (
                              blockers_mask * magic
                      ) & 0xFFFFFFFFFFFFFFFF  # truncate to 64 bit
            idx = product >> (64 - relevant_bits)
            attack_mask = self.attack_mask(sq, blockers_mask)
            if idx in table and table[idx] != attack_mask:
                collision = True
                break

            table[idx] = attack_mask

        if not collision:
            return True, table
        else:
            return False, {}

    def generate_magic(self, sq) -> tuple[int, dict[int, int]]:
        """Generates the magic number lookup table per square"""
        num_attempts = 1_000_000

        if self.magic_list is not None:
            magic = int(self.magic_list[sq], 16)
            collision, table = self.test_magic(sq, magic)
            if collision:
                print("Using precomputed magic")
                return magic, table
            else:
                print("Precomputed magic failed")
        for _ in range(num_attempts):
            magic = sparse_rand()
            collision, table = self.test_magic(sq, magic)
            if not collision:
                return magic, table

        raise RuntimeError(
            f"Exceeded {num_attempts} attempts while generating magic number for {self.name()} square {sq + 1}"
        )

    def generate_code(self):
        masks = []
        magics = []
        rel_bits = []
        attack_tables: list[dict[int, int]] = []

        for sq in range(64):
            t0 = time.time()
            print(f"Generating magic bitboards for {self.name()} square {sq + 1}")
            _, num_rel_bits = self.generate_blockers(sq)
            magic, table = self.generate_magic(sq)
            print(f"Finished in {time.time() - t0} seconds.")
            masks.append(self.relevance_mask(sq))
            rel_bits.append(num_rel_bits)
            magics.append(magic)
            attack_tables.append(table)

        code = format_c_array(
            [format_c_ull(n) for n in magics],
            f"{self.name()}_magic_numbers",
            "uint64_t",
        )
        code += "\n\n"

        code += format_c_array(
            [format_c_ull(n) for n in masks], f"{self.name()}_blocker_masks", "uint64_t"
        )
        code += "\n\n"
        code += format_c_array(
            [str(n) for n in rel_bits], f"{self.name()}_rel_bits", "uint64_t"
        )
        code += "\n\n"
        code += f"const uint64_t {self.name()}_attack_tables[64][{self.max_attacks()}] = {{\n"
        for i, table in enumerate(attack_tables):
            l = [0] * self.max_attacks()
            for k, v in table.items():
                l[k] = v

            l_formatted = "{" + (", ".join([format_c_ull(n) for n in l])) + "}"
            l_formatted = (
                    (" " * 4)
                    + l_formatted
                    + ("," if i != len(attack_tables) - 1 else "")
                    + "\n"
            )
            code += l_formatted
        code += "\n};"
        return code


class Rook(Slider):
    def name(self) -> str:
        return "rook"

    def max_attacks(self) -> int:
        return 4096

    def relevance_mask(self, sq: int) -> int:
        bb = 0
        rank = sq // 8
        file = sq % 8
        for i in range(file + 1, 7):
            bb |= 1 << (rank * 8 + i)

        for i in range(file - 1, 0, -1):
            bb |= 1 << (rank * 8 + i)

        for i in range(rank + 1, 7):
            bb |= 1 << (i * 8 + file)
        for i in range(rank - 1, 0, -1):
            bb |= 1 << (i * 8 + file)

        return bb

    def attack_mask(self, sq: int, blockers: int) -> int:
        attacks = 0
        rank = sq // 8
        file = sq % 8

        # Directions: north (+8), south (-8), east (+1), west (-1)
        for r in range(rank + 1, 8):
            sq = r * 8 + file
            attacks |= 1 << sq
            if blockers & (1 << sq):
                break

        for r in range(rank - 1, -1, -1):
            sq = r * 8 + file
            attacks |= 1 << sq
            if blockers & (1 << sq):
                break

        for f in range(file + 1, 8):
            sq = rank * 8 + f
            attacks |= 1 << sq
            if blockers & (1 << sq):
                break

        for f in range(file - 1, -1, -1):
            sq = rank * 8 + f
            attacks |= 1 << sq
            if blockers & (1 << sq):
                break

        return attacks


class Bishop(Slider):
    def name(self) -> str:
        return "bishop"

    def max_attacks(self) -> int:
        return 8192

    def relevance_mask(self, sq: int) -> int:
        bb = 0
        rank = sq // 8
        file = sq % 8

        r, f = rank + 1, file + 1
        while r < 7 and f < 7:
            bb |= 1 << (r * 8 + f)
            r += 1
            f += 1

        r, f = rank + 1, file - 1
        while r < 7 and f > 0:
            bb |= 1 << (r * 8 + f)
            r += 1
            f -= 1

        r, f = rank - 1, file + 1
        while r > 0 and f < 7:
            bb |= 1 << (r * 8 + f)
            r -= 1
            f += 1

        r, f = rank - 1, file - 1
        while r > 0 and f > 0:
            bb |= 1 << (r * 8 + f)
            r -= 1
            f -= 1

        return bb

    def attack_mask(self, sq, blockers):
        attacks = 0
        rank = sq // 8
        file = sq % 8

        # Diagonals: NE (-7), NW (-9), SE (+9), SW (+7)
        r, f = rank + 1, file + 1
        while r < 8 and f < 8:
            sq = r * 8 + f
            attacks |= 1 << sq
            if blockers & (1 << sq):
                break
            r += 1
            f += 1

        r, f = rank + 1, file - 1
        while r < 8 and f >= 0:
            sq = r * 8 + f
            attacks |= 1 << sq
            if blockers & (1 << sq):
                break
            r += 1
            f -= 1

        r, f = rank - 1, file + 1
        while r >= 0 and f < 8:
            sq = r * 8 + f
            attacks |= 1 << sq
            if blockers & (1 << sq):
                break
            r -= 1
            f += 1

        r, f = rank - 1, file - 1
        while r >= 0 and f >= 0:
            sq = r * 8 + f
            attacks |= 1 << sq
            if blockers & (1 << sq):
                break
            r -= 1
            f -= 1

        return attacks


# Pin tables
def squares_between(sq1, sq2):
    if sq1 == sq2:
        return 0

    rank1, file1 = sq1 // 8, sq1 % 8
    rank2, file2 = sq2 // 8, sq2 % 8

    bb = 0

    if rank1 == rank2:
        direction = 1 if file2 > file1 else -1
        f = file1
        while f != file2:
            f += direction
            if f != file2:
                bb |= 1 << (rank1 * 8 + f)

    elif file1 == file2:
        direction = 1 if rank2 > rank1 else -1
        r = rank1
        while r != rank2:
            r += direction
            if r != rank2:
                bb |= 1 << (r * 8 + file1)

    elif (rank2 - rank1) == (file2 - file1):  # Main diagonal
        step = 9 if sq2 > sq1 else -9
        s = sq1 + step
        while s != sq2:
            bb |= 1 << s
            s += step

    elif (file1 + rank1) == (file2 + rank2):  # Anti-diagonal
        step = 7 if sq2 > sq1 else -7
        s = sq1 + step
        while s != sq2:
            bb |= 1 << s
            s += step

    return bb


def generate_pin_tables():
    tables = []
    for i in range(64):
        tables.append([])
        for j in range(64):
            tables[i].append(format_c_ull(squares_between(i, j)))

    arr = ["{" + (",".join(table)) + "}" for table in tables]
    code = "const uint64_t squares_between[64][64] = {"
    for i, item in enumerate(arr):
        code += f"\n{' ' * 4}{item}{',' if i != len(arr) - 1 else ''}"
    code += "\n};"
    return code


# Utility functions


def format_c_array(array, name, type, static=False, const=True):
    code = f"{'static ' if static else ''}{'const ' if const else ''}{type} {name}[{len(array)}] = {{"
    for i, item in enumerate(array):
        code += f"\n{' ' * 4}{item}{',' if i != len(array) - 1 else ''}"
    code += "\n};"

    return code


def format_c_ull(n) -> str:
    return f"0x{n:016X}ULL"


def print_bitboard(bb):
    for rank in range(8):
        for file in range(8):
            sq = rank * 8 + file
            print(f" {(bb >> sq) & 1} ", end="")
        print()


def u64(n):
    return n & 0xFFFFFFFFFFFFFFFF


def main():
    random.seed(42)
    print("Seeding random with value 42")
    code = f"""// Various precomputed tables generated by generate_tables.py
#include "tables.h"

{generate_knight_moves()}

{generate_king_moves()}

{generate_pin_tables()}

{Rook().generate_code()}

{Bishop().generate_code()}
"""

    with open((Path(__file__).parent / ".." / "engine" / "tables.c").as_posix(), "w") as f:
        f.write(code)

    # this should probably be part of the code at some point.
    header_start = """// Header of various precomputed tables generated by generate_tables.py
#ifndef TABLES_H
#define TABLES_H

#include <stdint.h>

extern const uint64_t knight_moves[64];
extern const uint64_t king_moves[64];
extern const uint64_t squares_between[64][64];

"""
    header_content = ""
    for piece in ["rook", "bishop"]:
        header_content += f"extern const uint64_t {piece}_magic_numbers[64];\n"
        header_content += f"extern const uint64_t {piece}_rel_bits[64];\n"
        header_content += f"extern const uint64_t {piece}_blocker_masks[64];\n"
        n = 4096 if piece == "rook" else 8192
        header_content += f"extern const uint64_t {piece}_attack_tables[64][{n}];\n"
        header_content += "\n"

    header_end = "#endif // TABLES_H"
    header = header_start + "\n" + header_content + "\n" + header_end

    with open((Path(__file__).parent / ".." / "engine" / "tables.h").as_posix(), "w") as f:
        f.write(header)


main()
