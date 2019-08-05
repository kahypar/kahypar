/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2019 Sebastian Schlag <sebastian.schlag@kit.edu>
 * Copyright (C) 2019 Tobias Heuer <tobias.heuer@kit.edu>
 *
 * KaHyPar is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KaHyPar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with KaHyPar.  If not, see <http://www.gnu.org/licenses/>.
 *
******************************************************************************/

#include "gmock/gmock.h"

#include "kahypar/utils/thread_pool.h"

using ::testing::Eq;
using ::testing::Test;

namespace kahypar {
namespace parallel {

class AThreadPool : public ::testing::TestWithParam<size_t> {
 public:
  AThreadPool() :
    pool(GetParam()) { }

  ThreadPool pool;
};

INSTANTIATE_TEST_CASE_P(NumThreads,
                        AThreadPool,
                        ::testing::Values(1, 2, 4, 8, 16, 32));

TEST_P(AThreadPool, TestsSimpleCount) {
  size_t num_threads = GetParam();
  ASSERT_THAT(pool.size(), Eq(num_threads));

  std::atomic<size_t> cnt(0);
  std::vector<std::future<size_t>> res;
  for (size_t i = 0; i < num_threads; ++i) {
    res.emplace_back(pool.enqueue([&cnt](const size_t num) {
      while ( cnt < num) { }
      return cnt++;
    }, i));
  }

  for ( size_t i = 0; i < num_threads; ++i ) {
    res[i].wait();
    ASSERT_THAT(res[i].get(), Eq(i));
  }
}

}  // namespace parallel
}  // namespace kahypar
