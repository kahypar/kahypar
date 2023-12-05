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

#pragma once

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "kahypar/definitions.h"
#include "kahypar-resources/datastructure/fast_reset_flag_array.h"
#include "kahypar-resources/macros.h"

namespace kahypar {
namespace validate {

using ds::FastResetFlagArray;

/*!
  * Helper class for reading a line and reporting syntactical errors
  */
class CheckedIStream {
 public:
  explicit CheckedIStream(const char* begin, const char* end, size_t line_number = 0):
    _current(begin), _next(nullptr), _end(end), _line_number(line_number) {
      // whitespace at end of line is not considered an error
      while (_end > _current && *(_end - 1) == ' ') {
        --_end;
      }
    }

  explicit CheckedIStream(const std::string& line, size_t line_number = 0):
    CheckedIStream(line.c_str(), line.c_str() + line.size(), line_number) { }

  template<typename ResultT>
  bool operator>>(ResultT& result) {
    unsigned long long val = std::strtoull(_current, &_next, 10);
    if (val == 0 && _next == _current && _next != _end) {
      ERROR("Expected positive number", _line_number);
    }

    const ResultT max = std::numeric_limits<ResultT>::max();
    if (val > static_cast<unsigned long long>(max)) {
      ERROR("ID is out of range: " << val << ", but maximum is " << max, _line_number);
    }

    if (_current != _next) {
      _current = _next;
      result = static_cast<ResultT>(val);
      return true;
    }
    return false;
  }

  bool empty() const {
    char* p = nullptr;
    std::strtoull(_current, &p, 10);
    return p == _current && p == _end;
  }

 private:
  const char* _current;
  char* _next;
  const char* _end;
  size_t _line_number;
};

/*!
  * Types of semantic errors in hypergraph input
  */
enum class InputErrorType : uint8_t {
  FirstHyperedgeIndexNotZero,   // first entry in index array not zero
  HyperedgeIndexDescending,     // index array is not in ascending order
  HyperedgeOutOfBounds,         // size of hyperedge (according to index array) is larger than number of nodes
  HyperedgeSizeZero,            // size of hyperedge is zero
  HyperedgeWeightZero,          // weight of hyperedge is zero
  HyperedgeDuplicatePin,        // hyperedge contains duplicate pins
  HyperedgeInvalidPin,          // pin index is not a valid hypernode ID
  HypernodeWeightZero           // weight of hypernode is zero
};

bool isFatal(const InputErrorType& type) {
  switch (type) {
    case InputErrorType::FirstHyperedgeIndexNotZero:  return true;
    case InputErrorType::HyperedgeIndexDescending:    return true;
    case InputErrorType::HyperedgeOutOfBounds:        return true;
    case InputErrorType::HyperedgeSizeZero:           return false;
    case InputErrorType::HyperedgeWeightZero:         return false;
    case InputErrorType::HyperedgeDuplicatePin:       return false;
    case InputErrorType::HyperedgeInvalidPin:         return true;
    case InputErrorType::HypernodeWeightZero:         return true;
  }
  return true;
}

static std::ostream& operator<< (std::ostream& os, const InputErrorType& type) {
  switch (type) {
    case InputErrorType::FirstHyperedgeIndexNotZero:
      return os << "First entry in hyperedge indices must be 0";
    case InputErrorType::HyperedgeIndexDescending:
      return os << "Hyperedge indices must be in ascending order";
    case InputErrorType::HyperedgeOutOfBounds:
      return os << "Hyperedge has too many entries";
    case InputErrorType::HyperedgeSizeZero:
      return os << "Hyperedge with size 0";
    case InputErrorType::HyperedgeWeightZero:
      return os << "Hyperedge with weight 0";
    case InputErrorType::HyperedgeDuplicatePin:
      return os << "Hyperedge has duplicate pins";
    case InputErrorType::HyperedgeInvalidPin:
      return os << "Hyperedge has invalid pin. Each pin must be a valid vertex ID";
    case InputErrorType::HypernodeWeightZero:
      return os << "Vertices with weight 0 are not supported. The minimum allowed vertex weight is 1";
  }
  return os << static_cast<uint8_t>(type);
}


struct InputError {
  InputErrorType error_type;   // type of the error
  size_t element_index;        // index of erronous hyperedge/hypernode (zero if not available)
};


/*!
* Checks the provided hypergraph input for semantic errors
* (e.g. invalid hyperedge indices, invalid pins).
* Returns true if at least one error was found.
*
* \param num_hypernodes Number of hypernodes |V|
* \param num_hyperedges Number of hyperedges |E|
* \param hyperedge_indices For each hyperedge e, hyperedge_indices stores the index of the
*                          first pin in edge_vector that belongs to e.
* \param hyperedges Stores the pins of all hyperedges.
* \param hyperedge_weights Weight for each hyperedge
* \param hypernode_weights Weight for each hypernode
*
* \param errors If not null, all found errors are accumulated here
* \param ignored_hes If not null, hyperedges that should be ignored are accumulated here
* \param ignored_pins If not null, pins that should be ignored are accumulated here
*/
bool validateInput(const HypernodeID num_hypernodes, const HyperedgeID num_hyperedges,
                   const size_t* hyperedge_indices, const HypernodeID* hyperedges,
                   const HyperedgeWeight* hyperedge_weights, const HypernodeWeight* vertex_weights,
                   std::vector<InputError>* errors = nullptr,
                   std::vector<HyperedgeID>* ignored_hes = nullptr,
                   std::vector<size_t>* ignored_pins = nullptr) {
  bool found_error = false;
  auto report_error = [&](const InputErrorType type, const size_t index) {
    if (errors != nullptr) {
      errors->push_back(InputError{type, index});
    }
    found_error = true;
  };

  if (hyperedge_indices[0] != 0) {
    report_error(InputErrorType::FirstHyperedgeIndexNotZero, 0);
  }

  FastResetFlagArray<HyperedgeID> pin_set(num_hypernodes);
  // iterate through the input and report all errors
  for (HyperedgeID he = 0; he < num_hyperedges; ++he) {
    // check hyperedge_indices
    bool ignore_hyperedge = false;
    const size_t hyperedge_size = hyperedge_indices[he + 1] - hyperedge_indices[he];
    if (hyperedge_indices[he + 1] < hyperedge_indices[he]) {
      report_error(InputErrorType::HyperedgeIndexDescending, he);
      continue;  // no hyperedge here in any meaningful sense
    } else if (hyperedge_size == 0) {
      report_error(InputErrorType::HyperedgeSizeZero, he);
      ignore_hyperedge = true;
    } else if (hyperedge_size == 1) {
      // remove the hyperedge for efficiency reasons
      ignore_hyperedge = true;
    } else if (hyperedge_size > num_hypernodes) {
      // this check should hopefully prevent an overly long running time in case the indices are garbage
      report_error(InputErrorType::HyperedgeOutOfBounds, he);
      continue;
    }
    // check hyperedge_weight
    if (hyperedge_weights != nullptr && hyperedge_weights[he] == 0) {
      report_error(InputErrorType::HyperedgeWeightZero, he);
      ignore_hyperedge = true;
    }
    if (ignore_hyperedge && ignored_hes != nullptr) {
      ignored_hes->push_back(he);
    }

    // check pins
    pin_set.reset();
    bool invalid_pin = false;
    bool duplicate_pin = false;
    for (size_t i = hyperedge_indices[he]; i < hyperedge_indices[he + 1]; ++i) {
      const HypernodeID hn = hyperedges[i];
      bool ignore_pin = ignore_hyperedge; // all pins of an ignored hyperedge are also ignored
      if (hn >= num_hypernodes) {
        invalid_pin = true;
      } else if (pin_set[hn]) {
        duplicate_pin = true;
        ignore_pin = true; // ignore duplicate pins
      } else {
        pin_set.set(hn);
      }
      if (ignore_pin && ignored_pins != nullptr) {
        ignored_pins->push_back(i);
      }
    }
    // Note: We report invalid/duplicate pins once per hyperedge to avoid emitting
    // multiple errors for one line of the input file
    if (invalid_pin) {
      report_error(InputErrorType::HyperedgeInvalidPin, he);
    }
    if (duplicate_pin) {
      report_error(InputErrorType::HyperedgeDuplicatePin, he);
    }
  }
  if (vertex_weights != nullptr) {
    for (HypernodeID hn = 0; hn < num_hypernodes; ++hn) {
      if (vertex_weights[hn] == 0) {
        report_error(InputErrorType::HypernodeWeightZero, hn);
      }
    }
  }
  return found_error;
}


/*!
* Prints the list of errors, including line numbers if provided.
*
* \param num_hyperedges Number of hyperedges |E|
* \param errors List of errors from call to validateInput
* \param line_numbers Mapping of hyperedge/vertex IDs to line of input file.
*                     Must either be empty or have size num_hyperedges + num_hypernodes
* \param promote_warnings_to_errors If true, non-fatal errors are reported as error instead of warning
* \param out Output stream (default: stderr)
*/
void printErrors(const HyperedgeID num_hyperedges,
                 const std::vector<InputError>& errors,
                 const std::vector<size_t>& line_numbers = {},
                 bool promote_warnings_to_errors = false,
                 std::ostream& out = std::cerr) {
  ASSERT(line_numbers.empty() || line_numbers.size() >= static_cast<size_t>(num_hyperedges));

  for (const InputError& e: errors) {
    bool print_as_warning = !promote_warnings_to_errors && !isFatal(e.error_type);
    bool references_hypernode = (e.error_type == InputErrorType::HypernodeWeightZero);

    out << (print_as_warning ? "Warning: " : "Error: ") << e.error_type;
    if (line_numbers.empty()) {
      out << (references_hypernode ? " (Vertex " : " (Hyperedge ") << e.element_index << ")" << std::endl;
    } else {
      size_t line = references_hypernode ? num_hyperedges + e.element_index : e.element_index;
      out << " (line " << line_numbers[line] << ")" << std::endl;
    }
    if (e.element_index + 1 == num_hyperedges
        && (e.error_type == InputErrorType::HyperedgeIndexDescending
            || e.error_type == InputErrorType::HyperedgeOutOfBounds)) {
      // output additional help if the likely cause for the error is a missing sentinel element
      out << "Help: The last element of 'hyperedge_indices' must be a sentinel with value equal to the number of pins"
          << " (hyperedge_indices has one more element than the number of hyperedges)" << std::endl;
    }
  }
}

/*!
* Returns whether the list of errors contains a fatal error (so that partitioning must be aborted).
*
* \param errors List of errors from call to validateInput
* \param promote_warnings_to_errors If true, all errors are considered fatal
*/
bool containsFatalError(const std::vector<InputError>& errors, bool promote_warnings_to_errors = false) {
  for (const InputError& e: errors) {
    if (promote_warnings_to_errors || isFatal(e.error_type)) {
      return true;
    }
  }
  return false;
}

}  // namespace validate
}  // namespace kahypar
