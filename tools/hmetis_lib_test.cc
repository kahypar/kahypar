/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
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
