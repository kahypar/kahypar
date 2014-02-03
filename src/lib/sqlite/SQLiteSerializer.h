#ifndef LIB_SQLITE_SQLITE_SERIALIZER_H_
#define LIB_SQLITE_SQLITE_SERIALIZER_H_

#include "external/sqlite3pp/src/sqlite3pp.h"

#include "partition/Configuration.h"

#include <string>

using partition::CoarseningScheme;
using partition::StoppingRule;

namespace serializer {
class SQLiteBenchmarkSerializer {
  public:
  SQLiteBenchmarkSerializer(const std::string& db_name) :
    _db_name(db_name),
    _db(_db_name.c_str()),
    _setup_dummy(setupDB(_db)),
    _insert_result_cmd(_db, "INSERT INTO experiments (graph, hypernodes, hyperedges, k, \
epsilon, L_max, seed, initial_partitionings, coarsening_scheme, coarsening_node_weight_fraction, \
coarsening_node_weight_threshold, coarsening_min_node_count, coarsening_rating, twowayfm_stopping_rule, \
twowayfm_fruitless_moves, twowayfm_alpha, twowayfm_beta) VALUES (:graph, :hypernodes, :hyperedges, :k, \
:epsilon, :L_max, :seed, :initial_partitionings, :coarsening_scheme, :coarsening_node_weight_fraction, \
 :coarsening_node_weight_threshold, :coarsening_min_node_count, :coarsening_rating, :twowayfm_stopping_rule, \
:twowayfm_fruitless_moves, :twowayfm_alpha, :twowayfm_beta);")
  { }

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
                       "graph VARCHAR NOT NULL,"
                       "hypernodes INTEGER NOT NULL,"
                       "hyperedges INTEGER NOT NULL,"
                       "k INTEGER NOT NULL,"
                       "epsilon REAL NOT NULL,"
                       "L_max INTEGER NOT NULL,"
                       "seed INTEGER NOT NULL,"
                       "initial_partitionings INTEGER NOT NULL,"
                       "coarsening_scheme VARCHAR NOT NULL,"
                       "coarsening_node_weight_fraction REAL NOT NULL,"
                       "coarsening_node_weight_threshold INTEGER NOT NULL,"
                       "coarsening_min_node_count INTEGER NOT NULL,"
                       "coarsening_rating VARCHAR NOT NULL,"
                       "twowayfm_stopping_rule VARCHAR NOT NULL,"
                       "twowayfm_fruitless_moves INTEGER NOT NULL,"
                       "twowayfm_alpha REAL NOT NULL,"
                       "twowayfm_beta REAL NOT NULL"
                       ");").execute();
    return 0;
  }

  template <typename Config, typename Hypergraph>
  void dumpPartitioningResult(const Config& config, const Hypergraph& hypergraph) {
    _insert_result_cmd.reset();
    _insert_result_cmd.bind(":graph",
                            config.partitioning.graph_filename.substr(
                              config.partitioning.graph_filename.find_last_of("/") + 1).c_str());
    _insert_result_cmd.bind(":hypernodes", static_cast<int>(hypergraph.initialNumNodes()));
    _insert_result_cmd.bind(":hyperedges", static_cast<int>(hypergraph.initialNumEdges()));
    _insert_result_cmd.bind(":k", config.partitioning.k);
    _insert_result_cmd.bind(":epsilon", config.partitioning.epsilon);
    _insert_result_cmd.bind(":L_max",
                            static_cast<int>(config.partitioning.partition_size_upper_bound));
    _insert_result_cmd.bind(":seed", config.partitioning.seed);
    _insert_result_cmd.bind(":initial_partitionings",
                            config.partitioning.initial_partitioning_attempts);
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
    _insert_result_cmd.bind(":twowayfm_fruitless_moves",
                            config.two_way_fm.max_number_of_fruitless_moves);
    _insert_result_cmd.bind(":twowayfm_alpha", config.two_way_fm.alpha);
    _insert_result_cmd.bind(":twowayfm_beta", config.two_way_fm.beta);
    _insert_result_cmd.execute();
  }

  private:
  std::string _db_name;
  sqlite3pp::database _db;
  const char _setup_dummy;
  sqlite3pp::command _insert_result_cmd;
};
} // namespace serializer

#endif  // LIB_SQLITE_SQLITE_SERIALIZER_H_
