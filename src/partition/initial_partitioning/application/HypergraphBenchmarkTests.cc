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
#include <cmath>

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

struct partitioning_config {
	int part = 32;
	std::string k;
	std::vector<std::string> seed;
	double eps;
	std::string epsilon;
};

struct partitioning_stats {
	std::string benchmark;
	std::string mode;
	HyperedgeWeight cut;
	double imbalance;
	double time;
	int status;
};

void rounded(double& x, int nks) {
	int z = 0;

	if (!nks)
		z = 1;
	else
		z = pow(10, nks);   // nks = NachKommaStellen

	x = x * z + 0.5; // +.5 um die letzte Stelle richtig zu runden
	x = (int) x; //
	x /= z;
}

void printResults(std::vector<std::vector<partitioning_stats>>& stats,
		std::vector<std::string>& benchmarks) {
	for (int i = 0; i < stats.size(); i++) {
		std::cout << "************ " << benchmarks[i] << " ************"
				<< std::endl;
		for (int j = 0; j < stats[i].size(); j++) {
			std::cout << "Mode: " << stats[i][j].mode << std::endl;
			std::cout << "Cut: " << stats[i][j].cut << std::endl;
			std::cout << "Imbalance: " << stats[i][j].imbalance << std::endl;
			std::cout << "Time: " << stats[i][j].time << " s" << std::endl;
			std::cout << "Process Status: " << stats[i][j].status << std::endl;
			std::cout << "--------------------------------" << std::endl;
		}

	}
}

void printConfig(std::ofstream& latexStream, partitioning_config& config) {
	latexStream << "\\subsection{Configuration}\n";
	latexStream << "\\begin{itemize}\n";
	latexStream << "\\item k = " << config.k << "\n";
	latexStream << "\\item epsilon = " << config.epsilon << "\n";
	latexStream << "\\item Seeds = ";
	for (int i = 0; i < config.seed.size() - 1; i++)
		latexStream << config.seed[i] << ", ";
	latexStream << config.seed[config.seed.size() - 1] << "\n";
	latexStream << "\\end{itemize}\n";

}

void printTable(std::ofstream& latexStream,
		std::vector<std::vector<partitioning_stats>>& stats,
		std::vector<std::string>& benchmarks, std::vector<std::string>& modes,
		int j) {
	latexStream << "\\begin{tabular}[t]{";
	latexStream << "|c|c|c|c|} \\hline \n";
	latexStream << " & \\textbf{Cut} & \\textbf{Time} & \\textbf{Imbalance} ";
	latexStream << "\\\\ \\hline \\hline \n";

	partitioning_stats sum;
	sum.cut = 0;
	sum.imbalance = 0;
	double time = 0.0;
	for (int i = 0; i < stats.size(); i++) {
		if (stats[i][j].status == 0)
			latexStream << "\\textbf{" << benchmarks[i] << "}";
		else
			latexStream << "\\textcolor{red}{\\textbf{" << benchmarks[i]
					<< "}}";
		sum.cut += stats[i][j].cut;
		time += stats[i][j].time;
		sum.imbalance += stats[i][j].imbalance;
		latexStream << " & " << stats[i][j].cut << " & " << stats[i][j].time
				<< " s & " << stats[i][j].imbalance;
		if (i != stats.size() - 1)
			latexStream << "\\\\ \\hline \n";
		else
			latexStream << "\\\\ \\hline \\hline \n";

	}
	latexStream << "\\textbf{Result}";
	latexStream << " & " << sum.cut << " & " << time << " s & "
			<< (sum.imbalance / ((double) benchmarks.size()));
	latexStream << "\\\\ \\hline \n";
	latexStream << "\\end{tabular}\n";
}

void printCutTable(std::ofstream& latexStream,
		std::vector<std::vector<partitioning_stats>>& stats,
		std::vector<std::string>& benchmarks, std::vector<std::string>& modes) {

	latexStream << "\\begin{tabular}[t]{|c|";
	for (int i = 0; i < modes.size(); i++)
		latexStream << "c|";
	latexStream << "} \\hline \n";

	for (int i = 0; i < modes.size(); i++) {
		latexStream << " & \\textbf{" << modes[i] << "}";
	}
	latexStream << "\\\\ \\hline \\hline \n";

	std::vector<HyperedgeWeight> result(modes.size(), 0);
	for (int i = 0; i < benchmarks.size(); i++) {

		latexStream << "\\textbf{" << benchmarks[i] << "}";

		for (int j = 0; j < modes.size(); j++) {
			latexStream << " & " << stats[i][j].cut;
			result[j] += stats[i][j].cut;
		}
		if (i != stats.size() - 1)
			latexStream << "\\\\ \\hline \n";
		else
			latexStream << "\\\\ \\hline \\hline \n";
	}

	latexStream << "\\textbf{Result}";
	for (int i = 0; i < result.size(); i++) {
		latexStream << " & " << result[i];
	}
	latexStream << "\\\\ \\hline \n";
	latexStream << "\\end{tabular}\n";

}

void printTimeTable(std::ofstream& latexStream,
		std::vector<std::vector<partitioning_stats>>& stats,
		std::vector<std::string>& benchmarks, std::vector<std::string>& modes) {

	latexStream << "\\begin{tabular}[t]{|c|";
	for (int i = 0; i < modes.size(); i++)
		latexStream << "c|";
	latexStream << "} \\hline \n";

	for (int i = 0; i < modes.size(); i++) {
		latexStream << " & \\textbf{" << modes[i] << "}";
	}
	latexStream << "\\\\ \\hline \\hline \n";

	std::vector<double> result(modes.size(), 0);
	for (int i = 0; i < benchmarks.size(); i++) {

		latexStream << "\\textbf{" << benchmarks[i] << "}";

		for (int j = 0; j < modes.size(); j++) {
			rounded(stats[i][j].time, 2);
			latexStream << " & " << stats[i][j].time << " s";
			result[j] += stats[i][j].time;
		}
		if (i != stats.size() - 1)
			latexStream << "\\\\ \\hline \n";
		else
			latexStream << "\\\\ \\hline \\hline \n";
	}

	latexStream << "\\textbf{Result}";
	for (int i = 0; i < result.size(); i++) {
		latexStream << " & " << result[i] << " s";
	}
	latexStream << "\\\\ \\hline \n";
	latexStream << "\\end{tabular}\n";

}

void printImbalanceTable(std::ofstream& latexStream,
		std::vector<std::vector<partitioning_stats>>& stats,
		std::vector<std::string>& benchmarks, std::vector<std::string>& modes) {

	latexStream << "\\begin{tabular}[t]{|c|";
	for (int i = 0; i < modes.size(); i++)
		latexStream << "c|";
	latexStream << "} \\hline \n";

	for (int i = 0; i < modes.size(); i++) {
		latexStream << " & \\textbf{" << modes[i] << "}";
	}
	latexStream << "\\\\ \\hline \\hline \n";

	std::vector<double> result(modes.size(), 0);
	for (int i = 0; i < benchmarks.size(); i++) {

		latexStream << "\\textbf{" << benchmarks[i] << "}";

		for (int j = 0; j < modes.size(); j++) {
			rounded(stats[i][j].imbalance, 3);
			latexStream << " & " << stats[i][j].imbalance;
			result[j] += stats[i][j].imbalance;
		}
		if (i != stats.size() - 1)
			latexStream << "\\\\ \\hline \n";
		else
			latexStream << "\\\\ \\hline \\hline \n";
	}

	latexStream << "\\textbf{Result}";
	for (int i = 0; i < result.size(); i++) {
		result[i] /= benchmarks.size();
		rounded(result[i], 3);
		latexStream << " & " << (result[i]);
	}
	latexStream << "\\\\ \\hline \n";
	latexStream << "\\end{tabular}\n";

}

void printExceptions(std::ofstream& latexStream,
		std::vector<std::vector<partitioning_stats>>& stats) {

	latexStream << "\\begin{itemize} \n";
	for (int i = 0; i < stats.size(); i++) {

		for (int j = 0; j < stats[i].size(); j++) {
			if (stats[i][j].status != 0) {
				latexStream << "\\item " << stats[i][j].benchmark << ", "
						<< stats[i][j].mode << " \n";
			}
		}

	}

	latexStream << "\\end{itemize} \n";

}

void printLatexFile(std::vector<std::vector<partitioning_stats>>& stats,
		partitioning_config& config, std::vector<std::string>& benchmarks,
		std::vector<std::string>& modes) {
	std::ofstream latexStream;
	latexStream.open("result.tex");
	latexStream << "\\documentclass[landscape]{article}\n";
	latexStream << "\\usepackage{colortab}\n";
	latexStream << "\\usepackage{color}\n";
	latexStream << "\\usepackage{pifont}\n";
	latexStream
			<< "\\usepackage[a4paper,left=0.5cm,right=0.5cm,top=1.5cm,bottom=1.5cm,bindingoffset=5mm]{geometry}\n";
	latexStream << "\\begin{document}\n";

	latexStream << "\\section{Hypergraph-Benchmark-Results}\n";

	printConfig(latexStream, config);

	latexStream << "\\subsection{Hyperedge-Cut} \n";
	latexStream << "\\begin{center}\n";
	printCutTable(latexStream, stats, benchmarks, modes);
	latexStream << "\\end{center}\n";
	latexStream << "\n";

	latexStream << "\\subsection{Time} \n";
	latexStream << "\\begin{center}\n";
	printTimeTable(latexStream, stats, benchmarks, modes);
	latexStream << "\\end{center}\n";
	latexStream << "\n";

	latexStream << "\\subsection{Imbalance} \n";
	latexStream << "\\begin{center}\n";
	printImbalanceTable(latexStream, stats, benchmarks, modes);
	latexStream << "\\end{center}\n";
	latexStream << "\n";


	latexStream << "\\end{document}\n";
	latexStream.close();

	std::string pdflatex = "pdflatex result.tex";
	std::system(pdflatex.c_str());

	std::string clean = "rm -f *.aux *.log";
	std::system(clean.c_str());

}

void readHmetisResultFile(std::vector<std::vector<partitioning_stats>>& stats,
		std::vector<std::string>& benchmarks, std::vector<std::string>& modes) {

	modes.push_back("hMetis");

	std::string benchmark;
	HyperedgeWeight cut;
	double time;
	double imbalance;
	while (std::cin >> benchmark >> cut >> time >> imbalance) {
		int pos = -1;
		for (int i = 0; i < benchmarks.size(); i++) {
			if (benchmarks[i].compare(benchmark) == 0) {
				pos = i;
				break;
			}
		}

		if (pos != -1) {
			partitioning_stats stat;
			stat.benchmark = benchmark;
			stat.mode = "hMetis";
			stat.cut = cut;
			stat.time = time;
			stat.imbalance = imbalance;
			stat.status = 0;
			stats[pos].push_back(stat);
		}

	}

}

int main(int argc, char* argv[]) {

	std::vector<std::string> benchmarks { "ibm01", "ibm02", "ibm03", "ibm04",
			"ibm05", "ibm06", "ibm07", "vibrobox","avqsmall",
			"avqlarge", "bcsstk29", "bcsstk31", "bcsstk32", "s3rmq4m1",
			"s15850", "s35932", "s38584" };
	std::vector<std::string> modes { "random", "recursive-random", "bfs",
			"recursive-bfs", "greedy-maxpin", "greedy-maxnet", "greedy",
			"recursive-greedy", "sa", "ils" };

	/*std::vector<std::string> benchmarks {
	 "ibm03", "ibm04", "ibm05"};
	 std::vector<std::string> modes {  "random", "recursive-random", "bfs",
	 "recursive-bfs", "greedy-maxpin", "greedy-maxnet", "greedy",
	 "recursive-greedy", "ils"};*/

	partitioning_config config;

	config.part = 32;
	config.k = "32";
	config.seed.resize(4);
	config.seed[0] = "1";
	config.seed[1] = "42";
	config.seed[2] = "143";
	config.seed[3] = "201";
	config.eps = 0.05;
	config.epsilon = "0.05";

	std::vector<std::vector<partitioning_stats>> stats(benchmarks.size());

	int i = 0;
	for (std::string benchmark : benchmarks) {

		std::string hgr = "test_instances/" + benchmark + ".hgr";

		for (std::string mode : modes) {

			partitioning_stats statistic;
			statistic.benchmark = benchmark;
			statistic.mode = mode;
			statistic.imbalance = 0;
			statistic.cut = 0;
			statistic.status = 0;

			for (std::string seed : config.seed) {

				//Read Hypergraph-File
				HypernodeID num_hypernodes;
				HyperedgeID num_hyperedges;
				HyperedgeIndexVector index_vector;
				HyperedgeVector edge_vector;
				HyperedgeWeightVector hyperedge_weights;
				HypernodeWeightVector hypernode_weights;

				io::readHypergraphFile(hgr, num_hypernodes, num_hyperedges,
						index_vector, edge_vector, &hyperedge_weights,
						&hypernode_weights);
				Hypergraph hypergraph(num_hypernodes, num_hyperedges,
						index_vector, edge_vector, config.part,
						&hyperedge_weights, &hypernode_weights);

				HypernodeWeight total_graph_weight = 0;
				for (HypernodeID hn : hypergraph.nodes())
					total_graph_weight += hypergraph.nodeWeight(hn);

				std::string initial_partitioner_call =
						"./InitialPartitioningKaHyPar --hgr=" + hgr + " --k="
								+ config.k + " --seed=" + seed + " --epsilon="
								+ config.epsilon + " --mode=" + mode
								+ " --output=output/" + benchmark + ".part."
								+ config.k;

				HighResClockTimepoint start;
				HighResClockTimepoint end;

				if (statistic.status == 0) {
					start = std::chrono::high_resolution_clock::now();
					statistic.status = std::system(
							initial_partitioner_call.c_str());
					end = std::chrono::high_resolution_clock::now();
				}

				std::chrono::duration<double> elapsed_seconds = end - start;

				std::vector<HypernodeID> mapping(hypergraph.numNodes(), 0);
				for (HypernodeID hn : hypergraph.nodes())
					mapping[hn] = hn;

				std::vector<PartitionID> partitioning;
				io::readPartitionFile(
						"output/" + benchmark + ".part." + config.k,
						partitioning);
				HyperedgeWeight current_cut = metrics::hyperedgeCut(hypergraph,
						mapping, partitioning);
				double imbalance = metrics::imbalance(hypergraph, mapping,
						partitioning);
				statistic.cut += current_cut;
				statistic.imbalance += imbalance;
				statistic.time += ((double) elapsed_seconds.count());

			}

			statistic.cut /= config.seed.size();
			statistic.imbalance /= config.seed.size();
			statistic.time /= config.seed.size();

			stats[i].push_back(statistic);
		}
		i++;

	}

	readHmetisResultFile(stats, benchmarks, modes);

	printResults(stats, benchmarks);
	printLatexFile(stats, config, benchmarks, modes);
}

