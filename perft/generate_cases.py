import json

import pathlib

path = pathlib.Path(__file__).resolve().parent
cases = []
with open(path / "perftsuite.epd") as f:
    for i, case in enumerate(f.readlines()):
        case = case.split(";")
        fen = case[0]
        expected = [1]
        for j, num in enumerate(case[1:]):
            if j >= 4:
                break
            expected.append(int(num.split(" ")[1].strip()))

        case_dict = {
            "name": f"perftsuite #{i + 1}",
            "fen": fen,
            "depth": len(expected) - 1,
            "expected": expected,
        }

        cases.append(case_dict)

other_cases = [
    {
        "name": "kiwipete",
        "fen": "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - ",
        "depth": 6,
        "expected": [1, 48, 2039, 97862, 4085603, 193690690, 8031647685],
    },
    {
        "name": "CP Wiki #5",
        "fen": "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
        "depth": 5,
        "expected": [1, 48, 2039, 97862, 4085603],
    },
]

cases = other_cases + cases
print(json.dumps(cases))
