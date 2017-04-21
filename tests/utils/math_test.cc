/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include "kahypar/utils/math.h"

using ::testing::Eq;

namespace kahypar {
namespace math {
TEST(NextPowerOfTwoCeiled, WorksAsExpected) {
  ASSERT_THAT(nextPowerOfTwoCeiled(static_cast<unsigned int>(1)), Eq(1));
  ASSERT_THAT(nextPowerOfTwoCeiled(static_cast<unsigned int>(2)), Eq(2));
  ASSERT_THAT(nextPowerOfTwoCeiled(static_cast<unsigned int>(26)), Eq(32));
  ASSERT_THAT(nextPowerOfTwoCeiled(static_cast<unsigned int>(65)), Eq(128));
}

TEST(NearestMultipleOf, WorksAsExpected) {
  ASSERT_THAT(nearestMultipleOf(2, 64), Eq(64));
  ASSERT_THAT(nearestMultipleOf(64, 64), Eq(64));
  ASSERT_THAT(nearestMultipleOf(65, 64), Eq(128));
}

TEST(Digits, ReturnsTheNumberOfDigitsOfInteger) {
  ASSERT_THAT(digits(0), Eq(1));
  ASSERT_THAT(digits(1), Eq(1));
  ASSERT_THAT(digits(65), Eq(2));
  ASSERT_THAT(digits(1234567), Eq(7));
  ASSERT_THAT(digits(100), Eq(3));
  ASSERT_THAT(digits(9999999999999999999ull), Eq(19));
}
}  // namespace math
}  // namespace kahypar
