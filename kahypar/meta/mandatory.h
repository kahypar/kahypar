/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#pragma once

// http://clean-cpp.org/mandatory-template-arguments/

#include <type_traits>

namespace kahypar {
namespace meta {
struct unspecified_type;

template <typename SomeType>
class MandatoryTemplateArgument {
  struct private_type;

  static_assert(std::is_same<SomeType, private_type>::value,
                "You forgot to specify a mandatory template argument which cannot be deduced.");
};
}  // namespace meta
}  // namespace kahypar

using Mandatory = kahypar::meta::MandatoryTemplateArgument<kahypar::meta::unspecified_type>;

template <typename T>
using MandatoryTemplate = kahypar::meta::MandatoryTemplateArgument<T>;
