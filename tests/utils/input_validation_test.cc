/*******************************************************************************
 * MIT License
 *
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2023 Nikolai Maas <nikolai.maas@kit.edu>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ******************************************************************************/

#include "gmock/gmock.h"

#include <vector>

#include "kahypar/definitions.h"
#include "kahypar/utils/validate.h"

using ::testing::Eq;
using ::testing::Test;

namespace kahypar {
namespace validate {

struct ValidateInputTest : public Test {
  std::tuple<std::vector<InputError>, std::vector<HyperedgeID>, std::vector<size_t>> runValidation(
    const HypernodeID num_hypernodes, const HyperedgeID num_hyperedges,
    const std::vector<size_t>& hyperedge_indices, const std::vector<HypernodeID>& hyperedges,
    const std::vector<HyperedgeWeight>& hyperedge_weights = {}, const std::vector<HypernodeWeight>& vertex_weights = {}
  ) {
    std::vector<InputError> errors;
    std::vector<HyperedgeID> ignored_hes;
    std::vector<size_t> ignored_pins;
    validateInput(num_hypernodes, num_hyperedges, hyperedge_indices.data(), hyperedges.data(),
                  hyperedge_weights.empty() ? nullptr : hyperedge_weights.data(),
                  vertex_weights.empty() ? nullptr : vertex_weights.data(),
                  &errors, &ignored_hes, &ignored_pins);
    return {errors, ignored_hes, ignored_pins};
  }

  bool compareToExpected(const std::vector<InputError>& errors, const std::vector<InputError>& expected) {
    if (errors.size() != expected.size()) {
      return false;
    }
    for (size_t i = 0; i < errors.size(); ++i) {
      if (errors[i].error_type != expected[i].error_type || errors[i].element_index != expected[i].element_index) {
        return false;
      }
    }
    return true;
  }
};

TEST_F(ValidateInputTest, ReportsEachError) {
  std::vector<InputError> errors;
  std::vector<HyperedgeID> hes;
  std::vector<size_t> pins;

  std::tie(errors, hes, pins) = runValidation(1, 0, {1}, {});
  ASSERT_TRUE(compareToExpected(errors, { InputError{InputErrorType::FirstHyperedgeIndexNotZero, 0} }));

  std::tie(errors, hes, pins) = runValidation(2, 2, {0, 2, 1}, {0, 1});
  ASSERT_TRUE(compareToExpected(errors, { InputError{InputErrorType::HyperedgeIndexDescending, 1} }));

  std::tie(errors, hes, pins) = runValidation(1, 1, {0, 13}, {0});
  ASSERT_TRUE(compareToExpected(errors, { InputError{InputErrorType::HyperedgeOutOfBounds, 0} }));

  std::tie(errors, hes, pins) = runValidation(1, 2, {0, 0, 1}, {0});
  ASSERT_TRUE(compareToExpected(errors, { InputError{InputErrorType::HyperedgeSizeZero, 0} }));

  std::tie(errors, hes, pins) = runValidation(2, 1, {0, 2}, {0, 1}, {0});
  ASSERT_TRUE(compareToExpected(errors, { InputError{InputErrorType::HyperedgeWeightZero, 0} }));

  std::tie(errors, hes, pins) = runValidation(2, 1, {0, 2}, {0, 0});
  ASSERT_TRUE(compareToExpected(errors, { InputError{InputErrorType::HyperedgeDuplicatePin, 0} }));

  std::tie(errors, hes, pins) = runValidation(2, 1, {0, 2}, {0, 13});
  ASSERT_TRUE(compareToExpected(errors, { InputError{InputErrorType::HyperedgeInvalidPin, 0} }));

  std::tie(errors, hes, pins) = runValidation(1, 0, {0}, {}, {}, {0});
  ASSERT_TRUE(compareToExpected(errors, { InputError{InputErrorType::HypernodeWeightZero, 0} }));
}

TEST_F(ValidateInputTest, ReportsMultipleErrors) {
  auto [errors, hes, pins] = runValidation(4, 4, {0, 0, 1, 4, 13}, {13, 1, 3, 3, 0});
  ASSERT_TRUE(compareToExpected(errors, {
    InputError{InputErrorType::HyperedgeSizeZero, 0}, InputError{InputErrorType::HyperedgeInvalidPin, 1},
    InputError{InputErrorType::HyperedgeDuplicatePin, 2}, InputError{InputErrorType::HyperedgeOutOfBounds, 3}
  }));
}

TEST_F(ValidateInputTest, ReportsIgnoredHEsAndPins) {
  auto [errors, hes, pins] = runValidation(4, 5, {0, 0, 1, 4, 6, 10}, {0, 1, 1, 2, 0, 1, 2, 3, 2, 2}, {1, 1, 1, 0, 1});
  ASSERT_TRUE(compareToExpected(errors, {
    InputError{InputErrorType::HyperedgeSizeZero, 0}, InputError{InputErrorType::HyperedgeDuplicatePin, 2},
    InputError{InputErrorType::HyperedgeWeightZero, 3}, InputError{InputErrorType::HyperedgeDuplicatePin, 4}
  }));
  ASSERT_EQ(hes.size(), 3);
  ASSERT_EQ(hes[0], 0);
  ASSERT_EQ(hes[1], 1);
  ASSERT_EQ(hes[2], 3);
  ASSERT_EQ(pins.size(), 6);
  // note: pins of ignored hes are also ignored
  ASSERT_EQ(pins[0], 0);
  ASSERT_EQ(pins[1], 2);
  ASSERT_EQ(pins[2], 4);
  ASSERT_EQ(pins[3], 5);
  ASSERT_EQ(pins[4], 8);
  ASSERT_EQ(pins[5], 9);
}

}  // namespace math
}  // namespace kahypar
