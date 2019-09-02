/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2014-2016 Sebastian Schlag <sebastian.schlag@kit.edu>
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


#include "kahypar/application/command_line_options.h"
#include "kahypar/definitions.h"
#include "kahypar/io/hypergraph_io.h"
#include "kahypar/partitioner_facade.h"
#include "kahypar/utils/thread_pool.h"

int main(int argc, char* argv[]) {
  kahypar::Context context;

  kahypar::processCommandLineInput(context, argc, argv);

  //TODO(lars) we should let every part that uses the thread pool initialize and destroys this themselves. In community detection I currently just deactivate and reactivate the thread pool.
  context.shared_memory.pool = std::make_shared<kahypar::parallel::ThreadPool>(context);

  kahypar::Hypergraph hypergraph(
    kahypar::io::createHypergraphFromFile(context.partition.graph_filename,
                                          context.partition.k));

  if ( context.shared_memory.community_file != "" ) {
    std::vector<kahypar::PartitionID> communities;
    kahypar::io::readPartitionFile(context.shared_memory.community_file, communities);
    hypergraph.setCommunities(std::move(communities));
    hypergraph.setNumCommunities(128);
    context.preprocessing.enable_community_detection = false;
  }

  kahypar::PartitionerFacade().partition(hypergraph, context);


  return 0;
}
