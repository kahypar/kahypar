/***************************************************************************
 *  Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include <cstddef>
#include <iostream>
#include <vector>

extern "C" void HMETIS_PartRecursive(int, int, int*, int*, int*, int*, int, int, int*, int*, int*);

int main(int argn, char** argv) {
  std::vector<int> a { 0, 2, 6, 9,  /*sentinel*/ 12 };
  std::vector<int> b { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 };
  int cut;
  int options[9] { 1, 1, 1, 1, 1, 0, 0, -1, 31 };
  std::vector<int> part(7, 0);
  HMETIS_PartRecursive(7, 4, NULL, a.data(), b.data(), NULL, 2, 2, options, part.data(), &cut);

  std::cout << cut << std::endl;
}
