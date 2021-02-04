/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2021 Nikolai Maas <nikolai.maas@student.kit.edu>
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

#include "kahypar/meta/registrar.h"
#include "kahypar/partition/bin_packing/bin_packer.h"
#include "kahypar/partition/context.h"
#include "kahypar/partition/factories.h"

#define REGISTER_BIN_PACKER(id, bin_packer)                               \
  static meta::Registrar<BinPackerFactory> register_ ## bin_packer(      \
    id,                                                                 \
    [](Hypergraph& hypergraph, const Context& context) -> IBinPacker* {  \
    return new bin_packer(hypergraph, context); \
  })

namespace kahypar {
using bin_packing::BinPacker;
using bin_packing::WorstFit;
using bin_packing::FirstFit;

using WorstFitBinPacker = BinPacker<WorstFit>;
using FirstFitBinPacker = BinPacker<FirstFit>;

REGISTER_BIN_PACKER(BinPackingAlgorithm::worst_fit, WorstFitBinPacker);
REGISTER_BIN_PACKER(BinPackingAlgorithm::first_fit, FirstFitBinPacker);
}  // namespace kahypar
