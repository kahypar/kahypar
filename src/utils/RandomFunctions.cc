/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include "utils/RandomFunctions.h"
int Randomize::_seed = -1;
std::mt19937 Randomize::_gen;
std::uniform_int_distribution<int> Randomize::_bool_dist(0, 1);
std::uniform_int_distribution<int> Randomize::_int_dist(0, std::numeric_limits<int>::max());
std::uniform_real_distribution<float> Randomize::_float_dist(0, 1);
std::normal_distribution<float> Randomize::_norm_dist(0, 1);
