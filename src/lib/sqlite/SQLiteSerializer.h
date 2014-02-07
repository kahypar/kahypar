/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_LIB_SQLITE_SQLITESERIALIZER_H_
#define SRC_LIB_SQLITE_SQLITESERIALIZER_H_

#include <string>

#include "external/sqlite3pp/src/sqlite3pp.h"

#include "lib/GitRevision.h"
#include "lib/datastructure/Hypergraph.h"
#include "partition/Configuration.h"
#include "partition/Metrics.h"

using partition::CoarseningScheme;
using partition::StoppingRule;

namespace serializer {
class SQLiteBenchmarkSerializer {
  public:
  explicit SQLiteBenchmarkSerializer(const std::string& db_name) :
    _db_name(db_name),
    _db(_db_name.c_str()),
    _setup_dummy(setupDB(_db)),
    _insert_result_cmd(_db, "INSERT INTO experiments (graph, hypernodes, hyperedges, k,"
                            "epsilon, L_max, seed, initial_partitionings,"
                            "hyperedge_size_threshold, coarsening_scheme,"
                            "coarsening_node_weight_fraction, coarsening_node_weight_threshold,"
                            "coarsening_min_node_count, coarsening_rating,"
                            "twowayfm_stopping_rule, twowayfm_hyperedge_size_threshold, "
                            "twowayfm_fruitless_moves, twowayfm_alpha, twowayfm_beta, cut, part0,"
                            "part1, imbalance, gitrevision) VALUES (:graph, :hypernodes,"
                            ":hyperedges, :k, :epsilon, :L_max, :seed, :initial_partitionings,"
                            ":hyperedge_size_threshold, :coarsening_scheme, :coarsening_node_weight_fraction, "
                            ":coarsening_node_weight_threshold, :coarsening_min_node_count,"
                            ":coarsening_rating, :twowayfm_stopping_rule, :twowayfm_hyperedge_size_threshold,"
                            ":twowayfm_fruitless_moves, :twowayfm_alpha, :twowayfm_beta,:cut,"
                            ":part0, :part1, :imbalance, :gitrevision);") { }

  ~SQLiteBenchmarkSerializer() {
    sqlite3pp::command(_db, "COMMIT TRANSACTION;").execute();
  }

  char setupDB(sqlite3pp::database& db) {
    // Tweak configuration for more performance
    // See: http://blog.quibb.org/2010/08/fast-bulk-inserts-into-sqlite/
    sqlite3pp::command(db, "PRAGMA synchronous=OFF;").execute();
    sqlite3pp::command(db, "PRAGMA count_changes=OFF;").execute();
    sqlite3pp::command(db, "PRAGMA journal_mode=MEMORY;").execute();
    sqlite3pp::command(db, "PRAGMA temp_store=MEMORY;").execute();

    // Run everything in one transaction
    sqlite3pp::command(db, "BEGIN TRANSACTION;").execute();

    sqlite3pp::command(db,
                       "CREATE TABLE IF NOT EXISTS experiments ( "
                       "id INTEGER NOT NULL PRIMARY KEY ASC AUTOINCREMENT,"
                       "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,"
                       "graph TEXT NOT NULL,"
                       "hypernodes INTEGER NOT NULL,"
                       "hyperedges INTEGER NOT NULL,"
                       "k INTEGER NOT NULL,"
                       "epsilon REAL NOT NULL,"
                       "L_max INTEGER NOT NULL,"
                       "seed INTEGER NOT NULL,"
                       "initial_partitionings INTEGER NOT NULL,"
                       "hyperedge_size_threshold INTEGER NOT NULL,"    
                       "coarsening_scheme VARCHAR NOT NULL,"
                       "coarsening_node_weight_fraction REAL NOT NULL,"
                       "coarsening_node_weight_threshold INTEGER NOT NULL,"
                       "coarsening_min_node_count INTEGER NOT NULL,"
                       "coarsening_rating VARCHAR NOT NULL,"
                       "twowayfm_stopping_rule VARCHAR NOT NULL,"
                       "twowayfm_hyperedge_size_threshold INTEGER NOT NULL,"
                       "twowayfm_fruitless_moves INTEGER NOT NULL,"
                       "twowayfm_alpha REAL NOT NULL,"
                       "twowayfm_beta REAL NOT NULL,"
                       "cut INTEGER NOT NULL,"
                       "part0 INTEGER NOT NULL,"
                       "part1 INTEGER NOT NULL,"
                       "imbalance REAL NOT NULL,"
                       "gitrevision TEXT NOT NULL"
                       ");").execute();
    return 0;
  }

  template <typename Config, typename Hypergraph>
  void dumpPartitioningResult(const Config& config, const Hypergraph& hypergraph) {
    _insert_result_cmd.reset();
    std::string graph_name = config.partitioning.graph_filename.substr(
      config.partitioning.graph_filename.find_last_of("/") + 1);
    _insert_result_cmd.bind(":graph", graph_name.c_str());
    _insert_result_cmd.bind(":hypernodes", static_cast<int>(hypergraph.initialNumNodes()));
    _insert_result_cmd.bind(":hyperedges", static_cast<int>(hypergraph.initialNumEdges()));
    _insert_result_cmd.bind(":k", config.partitioning.k);
    _insert_result_cmd.bind(":epsilon", config.partitioning.epsilon);
    _insert_result_cmd.bind(":L_max",
                            static_cast<int>(config.partitioning.partition_size_upper_bound));
    _insert_result_cmd.bind(":seed", config.partitioning.seed);
    _insert_result_cmd.bind(":initial_partitionings",
                            config.partitioning.initial_partitioning_attempts);
    _insert_result_cmd.bind(":hyperedge_size_threshold",
                            static_cast<int>(config.partitioning.hyperedge_size_threshold));
    _insert_result_cmd.bind(":coarsening_scheme",
                            (config.coarsening.scheme == CoarseningScheme::HEAVY_EDGE_FULL ?
                             "heavy_full" : "heavy_heuristic"));
    _insert_result_cmd.bind(":coarsening_node_weight_fraction",
                            config.coarsening.hypernode_weight_fraction);
    _insert_result_cmd.bind(":coarsening_node_weight_threshold",
                            static_cast<int>(config.coarsening.threshold_node_weight));
    _insert_result_cmd.bind(":coarsening_min_node_count",
                            static_cast<int>(config.coarsening.minimal_node_count));
    _insert_result_cmd.bind(":coarsening_rating", "heavy_edge");
    _insert_result_cmd.bind(":twowayfm_stopping_rule",
                            (config.two_way_fm.stopping_rule == StoppingRule::SIMPLE ?
                             "simple" : "adaptive"));
    _insert_result_cmd.bind(":twowayfm_hyperedge_size_threshold",
                            static_cast<int>(config.two_way_fm.hyperedge_size_threshold));
    _insert_result_cmd.bind(":twowayfm_fruitless_moves",
                            config.two_way_fm.max_number_of_fruitless_moves);
    _insert_result_cmd.bind(":twowayfm_alpha", config.two_way_fm.alpha);
    _insert_result_cmd.bind(":twowayfm_beta", config.two_way_fm.beta);
    _insert_result_cmd.bind(":cut", static_cast<int>(metrics::hyperedgeCut(hypergraph)));

    int partition_size[2] = { 0, 0 };
    forall_hypernodes(hn, hypergraph) {
      ++partition_size[hypergraph.partitionIndex(*hn)];
    } endfor
    _insert_result_cmd.bind(":part0", partition_size[0]);
    _insert_result_cmd.bind(":part1", partition_size[1]);

    _insert_result_cmd.bind(":imbalance", metrics::imbalance(hypergraph));
    _insert_result_cmd.bind(":gitrevision", STR(KaHyPar_BUILD_VERSION));
    _insert_result_cmd.execute();
  }

  private:
  std::string _db_name;
  sqlite3pp::database _db;
  const char _setup_dummy;
  sqlite3pp::command _insert_result_cmd;
};
} // namespace serializer

#endif  // SRC_LIB_SQLITE_SQLITESERIALIZER_H_
