#ifndef TOOLS_RANDOMFUNCTIONS_H_
#define TOOLS_RANDOMFUNCTIONS_H_

#include <ctime>

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>

namespace randomize {

static boost::random::mt19937 gen(time(0));
static boost::random::uniform_int_distribution<> dist(0,1);

bool flipCoin() {
  return static_cast<bool>(dist(gen));
}


} // namespace random

#endif  // TOOLS_RANDOMFUNCTIONS_H_
