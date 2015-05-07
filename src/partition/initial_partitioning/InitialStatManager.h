/*
 * InitialStatManager.h
 *
 *  Created on: May 7, 2015
 *      Author: theuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_INITIALSTATMANAGER_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_INITIALSTATMANAGER_H_

#define RESET   "\033[0m"
#define BOLD   "\033[1m"
#define BLUE    "\033[34m"

#include <map>
#include <string>
#include <vector>

namespace partition {

class InitialStatManager {

public:
	static InitialStatManager& getInstance() {
		static InitialStatManager stat;
		return stat;
	}

	void addStat(std::string group, std::string name, double stat, std::string style = "") {
		stats[group].push_back(std::make_pair(name, stat));
		styles[group].push_back(std::make_pair(name, style));
	}

	double getStat(std::string group, std::string name) {
		for (auto v : stats[group]) {
			if (v.first.compare(name) == 0) {
				return v.second;
			}
		}
		return 0.0;
	}

	void updateStat(std::string group, std::string name, double value) {
		for (int i = 0; i < stats[group].size(); i++) {
			if (stats[group][i].first.compare(name) == 0) {
				stats[group][i] = std::make_pair(name,value);
				return;
			}
		}
		addStat(group, name, value);
	}

	void printStats() {
		for (auto s : stats) {

			std::cout << BOLD << BLUE;
			for (int i = 0; i < s.first.size() + 18; i++)
				std::cout << "=";
			std::cout << "\n======== " << s.first << " ========" << std::endl;
			for (int i = 0; i < s.first.size() + 18; i++)
				std::cout << "=";
			std::cout << RESET <<  std::endl;

			for(int i = 0; i < s.second.size(); i++) {
				std::cout << styles[s.first][i].second << s.second[i].first << " = " << s.second[i].second << RESET<< std::endl;
			}
		}
	}

	std::map<std::string, std::vector<std::pair<std::string, double>>> stats;
	std::map<std::string, std::vector<std::pair<std::string, std::string>>> styles;

private:
	InitialStatManager() {};

	~InitialStatManager() {};

};

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_INITIALSTATMANAGER_H_ */
