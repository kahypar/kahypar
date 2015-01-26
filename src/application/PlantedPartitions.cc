#include <boost/program_options.hpp>
#include <random>
#include <fstream>
#include <algorithm>
#include <ctime>
#include <iostream>

#include "tools/RandomFunctions.h"

namespace po = boost::program_options;

int main(int argc, char* argv[])
{
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help", "show help message")
    ("n", po::value<unsigned int>(), "number of hypernodes")
    ("m", po::value<unsigned int>(), "number of hyperedges")
    ("k", po::value<unsigned int>(), "number of planted partition blocks")
    ("p_inter", po::value<float>(), "probability for inter partition edges")
    ("deviation", po::value<float>(), "standart deviation fot the partition size")
    ("output", po::value<std::string>(), "output hypergraph path")
    ("seed", po::value<int>(), "random seed");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);
  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return 0;
  }

  if (!vm.count("n") ||
      !vm.count("m") ||
      !vm.count("k") ||
      !vm.count("p_inter") ||
      !vm.count("output"))
  {
    std::cout << "missing parameters!" << std::endl;
    std::cout << "deviation is the only optional and is in that case assumed to be 0" << std::endl;
    std::cout << desc << std::endl;
    return 0;
  }

  // parse the parameters
  unsigned int n = vm["n"].as<unsigned int>();
  unsigned int m = vm["m"].as<unsigned int>();
  unsigned int k = vm["k"].as<unsigned int>();
  float p_inter = vm["p_inter"].as<float>(); // inter partition egdes -> between partitions
  std::string result_file = vm["output"].as<std::string>();

  float deviation = 0;
  if (vm.count("deviation"))
  {
    deviation = vm["deviation"].as<float>();
  }

  int seed = std::random_device()();
  if (vm.count("seed") && vm["seed"].as<int>() != -1)
  {
    seed = vm["seed"].as<int>();
  }

  // main programm
  Randomize::setSeed(seed);
  // assign nodes to clusters
  std::vector<int> nodes(n);
  std::iota(nodes.begin(), nodes.end(), 1);

  Randomize::shuffleVector(nodes, n);
  // determine the size of each block
  float mean = static_cast<float>(n) / static_cast<float>(k);

  std::vector<int> block_size;
  int cur = 0;
  for (unsigned int i = 0; i < k; ++i)
  {
    int t = std::round(Randomize::getNormalDistributedFloat(mean, deviation));
    if (t + cur < n)
    {
      block_size.push_back(t);
    } else {
      t = n - cur;
      block_size.push_back(t);
    }
    cur += t;
  }

  std::vector<std::vector<int>> blocks;
  int last_pos = 0;
  for (unsigned int i = 0; i < k; ++i)
  {
    std::vector<int> temp_block;
    for (unsigned int j = 0; j < block_size.at(i); ++j)
    {
      temp_block.push_back(nodes.at(last_pos++));
    }
    blocks.push_back(std::move(temp_block));
  }

  std::ofstream ofs("temp.txt");
  ofs << "% n = " << n << " m = " << m <<
    " k = " << k << " p_inter = " << p_inter  <<  " standart_deviation_block_size = " << deviation<< std::endl;

  std::vector<std::vector<int>> clean_blocks;
  int t = 1;
  for (const auto blk : blocks)
  {
    if (blk.size() >= 2)
    {
      clean_blocks.push_back(blk);
      ofs << "% block" << t++ << " (size=" << blk.size() <<") : ";
      for (auto&& nds : blk)
      {
        ofs << nds << " ";
      }
      ofs << std::endl;
    }
  }

  unsigned int cut_weight = 0;

  std::vector<int> all_pins;
  for (auto&& blk : blocks)
  {
    for (auto pin : blk)
    {
      all_pins.push_back(pin);
    }
  }

  // the header of the hgr file
  ofs << n << " " << m << std::endl;
  for (unsigned int i = 0; i < m; ++i)
  {
    std::vector<int> *current_hyperedge;
    // determine wheter it is an anter or intra cluster edge
    if (Randomize::getRandomFloat(0.,1.) < p_inter)
    {
      // inter cluster edge (between different blocks)
      cut_weight++;
      current_hyperedge = &all_pins;
    } else {
      // intra cluster edge (inside a block)
      // determine the block oh this hyperedge
      int block_id = Randomize::getRandomInt(0, clean_blocks.size() -1);
      current_hyperedge = &clean_blocks.at(block_id);
    }

    int num_pins = Randomize::getRandomInt(2, current_hyperedge->size());
    Randomize::shuffleVector(*current_hyperedge, current_hyperedge->size());
    for (int j = 0; j < num_pins; j++)
    {
      ofs << current_hyperedge->at(j) << " ";
    }
    ofs << std::endl;
  }
  ofs.close();

  // ugly hack.. i want the planted cut to be in the comments in the top of the file
  std::ofstream result_ofs(result_file);
  std::ifstream tmp_istream("temp.txt");

  std::string tmp_string;

  int c = 0;
  while (std::getline(tmp_istream, tmp_string))
  {
    if (c==1)
    {
      result_ofs << "% cut = " << cut_weight << std::endl;
    }
    result_ofs << tmp_string << std::endl;
    ++c;
  }
  std::remove("temp.txt");
  result_ofs.close();
}
