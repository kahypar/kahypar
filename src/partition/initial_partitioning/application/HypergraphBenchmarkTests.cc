/*
 * HypergraphBenchmarkTests.cc
 *
 *  Created on: Apr 22, 2015
 *      Author: theuer
 */

#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <fstream>

#include "lib/definitions.h"
#include "lib/io/HypergraphIO.h"
#include "partition/Metrics.h"

using defs::Hypergraph;
using defs::HypernodeID;
using defs::HypernodeWeight;
using defs::HyperedgeID;
using defs::HyperedgeIndexVector;
using defs::HyperedgeVector;
using defs::HyperedgeWeightVector;
using defs::HypernodeWeightVector;
using defs::PartitionID;
using defs::HighResClockTimepoint;

struct partitioning_stats {
	std::string benchmark;
	std::string mode;
	HyperedgeWeight cut;
	double imbalance;
	std::chrono::duration<double> time;
};

void printResults(std::vector<std::vector<partitioning_stats>>& stats, std::vector<std::string>& benchmarks) {
	for(int i = 0; i < stats.size(); i++) {
		std::cout << "************ " << benchmarks[i] << " ************" << std::endl;
		for(int j = 0; j < stats[i].size(); j++) {
			std::cout << "Mode: " << stats[i][j].mode << std::endl;
			std::cout << "Cut: " << stats[i][j].cut << std::endl;
			std::cout << "Imbalance: " << stats[i][j].imbalance << std::endl;
			std::cout << "Time: " << stats[i][j].time.count() << " s"<< std::endl;
			std::cout << "--------------------------------" << std::endl;
		}

	}
}

void printTable(std::ofstream& latexStream, std::vector<std::vector<partitioning_stats>>& stats, std::vector<std::string>& benchmarks, std::vector<std::string>& modes, int j) {
	  latexStream << "\\begin{tabular}[t]{";
	  latexStream << "|c|c|c|c|} \\hline \n";
	  latexStream << " & \\textbf{Cut} & \\textbf{Time} & \\textbf{Imbalance} ";
	  latexStream << "\\\\ \\hline \\hline \n";


	  partitioning_stats sum;
	  sum.cut = 0;
	  sum.imbalance = 0;
	  double time = 0.0;
	  for(int i = 0; i < stats.size(); i++) {
		  latexStream << "\\textbf{"<<benchmarks[i]<<"}";
		  sum.cut += stats[i][j].cut;
		  time += stats[i][j].time.count();
		  sum.imbalance += stats[i][j].imbalance;
		  latexStream << " & " << stats[i][j].cut << " & " << stats[i][j].time.count() << " s & " << stats[i][j].imbalance;
		  if(i != stats.size()-1)
			  latexStream << "\\\\ \\hline \n";
		  else
			  latexStream << "\\\\ \\hline \\hline \n";

	  }
	  latexStream << "\\textbf{Result}";
	  latexStream << " & " << sum.cut << " & " << time << " s & " << (sum.imbalance/((double) benchmarks.size()));
	  latexStream << "\\\\ \\hline \n";
	  latexStream << "\\end{tabular}\n";
}

void printLatexFile(std::vector<std::vector<partitioning_stats>>& stats, std::vector<std::string>& benchmarks, std::vector<std::string>& modes) {
	  std::ofstream latexStream;
	  latexStream.open ("result.tex");
	  latexStream << "\\documentclass{article}\n";
	  latexStream << "\\usepackage{colortab}\n";
	  latexStream << "\\usepackage{pifont}\n";
	  latexStream << "\\begin{document}\n";

	  latexStream << "\\section{Hypergraph-Benchmark-Results}\n";

	  for(int i = 0; i < modes.size(); i++) {
		  latexStream << "\\subsection{ " <<  modes[i] << "} \n";
		  latexStream << "\\begin{center}\n";
		  printTable(latexStream,stats,benchmarks,modes,i);
		  latexStream << "\\end{center}\n";
		  latexStream << "\n";
	  }

	  latexStream << "\\end{document}\n";
	  latexStream.close();

	  std::string pdflatex = "pdflatex result.tex";
      std::system(pdflatex.c_str());

}

int main(int argc, char* argv[]) {

	std::vector<std::string> benchmarks {"avqlarge", "avqsmall", "cs4","ibm03","ibm04","ibm05","s3rmq4m1","s15850","s35932","s38584","memplus","vibrobox"};
	std::vector<std::string> modes {"random","recursive-random","bfs","recursive-bfs","greedy","recursive-greedy","ils"};

	int part = 32;
	std::string k = "32";
	std::string seed = "1";
	std::string epsilon = "0.05";

	std::vector<std::vector<partitioning_stats>> stats(benchmarks.size());

	int i = 0;
	for(std::string benchmark : benchmarks) {

		std::string hgr = "test_instances/" + benchmark + ".hgr";

		for(std::string mode : modes) {

			partitioning_stats statistic;
			statistic.benchmark = benchmark;
			statistic.mode = mode;

			//Read Hypergraph-File
			HypernodeID num_hypernodes;
			HyperedgeID num_hyperedges;
			HyperedgeIndexVector index_vector;
			HyperedgeVector edge_vector;
			HyperedgeWeightVector hyperedge_weights;
			HypernodeWeightVector hypernode_weights;

			io::readHypergraphFile(hgr,
					num_hypernodes, num_hyperedges, index_vector, edge_vector,
					&hyperedge_weights, &hypernode_weights);
			Hypergraph hypergraph(num_hypernodes, num_hyperedges, index_vector,
					edge_vector, part, &hyperedge_weights,
					&hypernode_weights);

	        std::string initial_partitioner_call = "./InitialPartitioningKaHyPar --hgr="
	                                   + hgr
	                                   + " --k=" + k
	                                   + " --seed=" + seed
	                                   + " --epsilon=" + epsilon
	                                   + " --mode=" + mode
	                                   + " --output=output/" + benchmark + ".part." + k;

	    	HighResClockTimepoint start;
	    	HighResClockTimepoint end;

	    	start = std::chrono::high_resolution_clock::now();
	        std::system(initial_partitioner_call.c_str());
	    	end = std::chrono::high_resolution_clock::now();

	    	std::chrono::duration<double> elapsed_seconds = end - start;

	        std::vector<HypernodeID> mapping(hypergraph.numNodes(),0);
	        for(HypernodeID hn : hypergraph.nodes())
	        	mapping[hn] = hn;

	        std::vector<PartitionID> partitioning;
	        io::readPartitionFile("output/" + benchmark + ".part." + k, partitioning);
	        HyperedgeWeight current_cut = metrics::hyperedgeCut(hypergraph, mapping, partitioning);
	        double imbalance =  metrics::imbalance(hypergraph, mapping, partitioning);
	        statistic.cut = current_cut;
	        statistic.imbalance = imbalance;
	        statistic.time = elapsed_seconds;
	        stats[i].push_back(statistic);
		}
		i++;

	}

	printResults(stats, benchmarks);
	printLatexFile(stats, benchmarks, modes);
}


