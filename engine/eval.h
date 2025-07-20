//
// Created by Gideon Grinberg on 7/18/25.
//

#ifndef EVAL_H
#define EVAL_H
#include "position.h"
extern const int piece_tables[6][64];
extern const int piece_values[6];
int eval_position(Position *pos);
#endif // EVAL_H
