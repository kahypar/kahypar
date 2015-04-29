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
	std::vector<double> eps;
};

struct partitioning_stats {
	std::string benchmark;
	std::string mode;
	std::vector<HyperedgeWeight> cut;
	std::vector<double> imbalance;
	std::vector<double> time;
	int status;
};

bool exception = false;

partitioning_config config;

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

/*void printResults(std::vector<std::vector<partitioning_stats>>& stats,
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
 }*/

void printConfig(std::ofstream& latexStream) {
	latexStream << "\\subsection{Configuration}\n";
	latexStream << "\\begin{itemize}\n";
	latexStream << "\\item k = " << config.k << "\n";
	latexStream << "\\item epsilon = ";
	for (int i = 0; i < config.eps.size() - 1; i++) {
		latexStream << config.eps[i] << ", ";
	}
	latexStream << config.eps[config.eps.size() - 1] << "\n";
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
	for (int i = 0; i < config.eps.size(); i++) {
		latexStream << "|c|c|c";
	}
	latexStream << "|c|} \\hline \n";
	for (int i = 0; i < config.eps.size(); i++) {
		latexStream
				<< "& \\textbf{Cut} & \\textbf{Time} & \\textbf{Imbalance} ";
	}
	latexStream << "\\\\ \\hline \\hline \n";

	partitioning_stats sum;
	sum.imbalance.assign(config.eps.size(), 0);
	sum.cut.assign(config.eps.size(), 0);
	sum.time.assign(config.eps.size(), 0);
	for (int i = 0; i < stats.size(); i++) {
		if (stats[i][j].status == 0)
			latexStream << "\\textbf{" << benchmarks[i] << "}";
		else
			latexStream << "\\textcolor{red}{\\textbf{" << benchmarks[i]
					<< "}}";
		for (int k = 0; k < config.eps.size(); k++) {
			rounded(stats[i][j].time[k], 3);
			rounded(stats[i][j].imbalance[k], 3);
			sum.cut[k] += stats[i][j].cut[k];
			sum.time[k] += stats[i][j].time[k];
			sum.imbalance[k] += stats[i][j].imbalance[k];
			latexStream << " & " << stats[i][j].cut[k] << " & "
					<< stats[i][j].time[k] << " s & "
					<< stats[i][j].imbalance[k];
		}
		if (i != stats.size() - 1)
			latexStream << "\\\\ \\hline \n";
		else
			latexStream << "\\\\ \\hline \\hline \n";

	}
	latexStream << "\\textbf{Result}";
	for (int k = 0; k < config.eps.size(); k++) {
		latexStream << " & " << sum.cut[k] << " & " << sum.time[k] << " s & "
				<< (sum.imbalance[k] / static_cast<double>(benchmarks.size()));
	}
	latexStream << "\\\\ \\hline \n";
	latexStream << "\\end{tabular}\n";
}

void printCutTable(std::ofstream& latexStream,
		std::vector<std::vector<partitioning_stats>>& stats,
		std::vector<std::string>& benchmarks, std::vector<std::string>& modes, int k) {

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
			latexStream << " & " << stats[i][j].cut[k];
			result[j] += stats[i][j].cut[k];
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

/*void printTimeTable(std::ofstream& latexStream,
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

 if (exception) {
 latexStream << "\\subsection{Exceptions} \n";
 latexStream << "\\begin{center}\n";
 printExceptions(latexStream, stats);
 latexStream << "\\end{center}\n";
 latexStream << "\n";
 }

 latexStream << "\\end{document}\n";
 latexStream.close();

 std::string pdflatex = "pdflatex result.tex";
 std::system(pdflatex.c_str());

 std::string clean = "rm -f *.aux *.log";
 std::system(clean.c_str());

 }*/

void printLatexFile2(std::vector<std::vector<partitioning_stats>>& stats,
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

	printConfig(latexStream);

	for (int i = 0; i < modes.size(); i++) {
		latexStream << "\\subsection{" << modes[i] << "} \n";
		latexStream << "\\begin{center}\n";
		printTable(latexStream, stats, benchmarks, modes, i);
		latexStream << "\\end{center}\n";
		latexStream << "\n";
	}

	for(int i = 0; i < config.eps.size(); i++) {
		latexStream << "\\subsection{$\\epsilon = " << config.eps[i] << "$} \n";
		latexStream << "\\begin{center}\n";
		printCutTable(latexStream, stats, benchmarks, modes, i);
		latexStream << "\\end{center}\n";
		latexStream << "\n";
	}

	latexStream << "\\end{document}\n";
	latexStream.close();

	std::string pdflatex = "pdflatex result.tex";
	std::system(pdflatex.c_str());

	std::string clean = "rm -f *.aux *.log";
	std::system(clean.c_str());

}

/*void readHmetisResultFile(std::vector<std::vector<partitioning_stats>>& stats,
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

 }*/

int main(int argc, char* argv[]) {

	std::vector<std::string> benchmarks { "ibm01", "ibm02", "ibm03", "ibm04",
			"ibm05", "ibm06", "ibm07", "vibrobox", "avqsmall", "avqlarge",
			"bcsstk29", "bcsstk31", "bcsstk32", "s3rmq4m1", "s15850", "s35932",
			"s38584" };
	std::vector<std::string> modes { "greedy --refinement=false --rollback=false", "greedy --refinement=false --rollback=true", "greedy --refinement=true --rollback=false", "greedy --refinement=true --rollback=true" };

	/*std::vector<std::string> benchmarks {
	 "ibm03", "ibm04", "ibm05"};
	 std::vector<std::string> modes {  "random", "recursive-random", "bfs",
	 "recursive-bfs", "greedy-maxpin", "greedy-maxnet", "greedy",
	 "recursive-greedy", "ils"};*/

	config.part = 2;
	config.k = "2";
	config.seed.resize(4);
	config.seed[0] = "1";
	config.seed[1] = "42";
	config.seed[2] = "143";
	config.seed[3] = "201";
	config.eps.resize(4);
	config.eps[0] = 0.01;
	config.eps[1] = 0.03;
	config.eps[2] = 0.05;
	config.eps[3] = 0.1;

	std::vector<std::vector<partitioning_stats>> stats(benchmarks.size());

	int i = 0;
	for (std::string benchmark : benchmarks) {

		std::string hgr = "test_instances/" + benchmark + ".hgr";

		for (std::string mode : modes) {

			partitioning_stats statistic;
			statistic.benchmark = benchmark;
			statistic.mode = mode;
			statistic.imbalance.assign(config.eps.size(), 0);
			statistic.cut.assign(config.eps.size(), 0);
			statistic.time.assign(config.eps.size(), 0);
			statistic.status = 0;

			for (int j = 0; j < config.eps.size(); j++) {

				for (std::string seed : config.seed) {

					std::cout << benchmark << ", " << mode << ", " << seed
							<< std::endl;

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
							"./InitialPartitioningKaHyPar --hgr=" + hgr
									+ " --k=" + config.k + " --seed=" + seed
									+ " --epsilon="
									+ std::to_string(config.eps[j]) + " --mode="
									+ mode
									+ " --output=output/"
									+ benchmark + ".part." + config.k;

					HighResClockTimepoint start;
					HighResClockTimepoint end;

					if (statistic.status == 0) {
						start = std::chrono::high_resolution_clock::now();
						statistic.status = std::system(
								initial_partitioner_call.c_str());
						end = std::chrono::high_resolution_clock::now();
					} else {
						exception = true;
					}

					std::chrono::duration<double> elapsed_seconds = end - start;

					std::vector<HypernodeID> mapping(hypergraph.numNodes(), 0);
					for (HypernodeID hn : hypergraph.nodes())
						mapping[hn] = hn;

					std::vector<PartitionID> partitioning;
					io::readPartitionFile(
							"output/" + benchmark + ".part." + config.k,
							partitioning);
					HyperedgeWeight current_cut = metrics::hyperedgeCut(
							hypergraph, mapping, partitioning);
					double imbalance = metrics::imbalance(hypergraph, mapping,
							partitioning);
					statistic.cut[j] += current_cut;
					statistic.imbalance[j] += imbalance;
					statistic.time[j] +=
							static_cast<double>(elapsed_seconds.count());

				}

				statistic.cut[j] /= config.seed.size();
				statistic.imbalance[j] /= config.seed.size();
				statistic.time[j] /= config.seed.size();
			}

			stats[i].push_back(statistic);
		}
		i++;

	}

	//readHmetisResultFile(stats, benchmarks, modes);

	//printResults(stats, benchmarks);
	printLatexFile2(stats, config, benchmarks, modes);
}

