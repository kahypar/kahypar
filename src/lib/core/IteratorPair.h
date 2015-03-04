#ifndef SRC_LIB_CORE_ITERATORPAIR_H_
#define SRC_LIB_CORE_ITERATORPAIR_H_

#include <utility>

namespace core {
// based on:
// http://cplusplusmusings.wordpress.com/2013/04/14/range-based-for-loops-and-pairs-of-iterators/

template <typename Iterator>
class IteratorPair {
  public:
  IteratorPair(Iterator&& first, Iterator&& last) noexcept :
    _f(std::move(first)),
    _l(std::move(last)) {
    std::cout << " IteratorPairConstructor" << std::endl;
  }

  IteratorPair(const IteratorPair& other) noexcept :
                                           _f(other._f),
                                           _l(other._l) {
  std::cout << " IteratorPairCopyConstructor" << std::endl;
  }

  IteratorPair(IteratorPair&& other) noexcept :
    _f(std::move(other._f)),
    _l(std::move(other._l)) {
      std::cout << "IteratorPairMoveConstructor" << std::endl;
  }

  IteratorPair& operator=(IteratorPair&& other) = default;

  const Iterator& begin() const noexcept { return _f; }
  const Iterator& end() const noexcept { return _l; }

  private:
  Iterator _f;
  Iterator _l;
};

template <typename Iterator>
IteratorPair<Iterator> makeIteratorPair(Iterator&& f, Iterator&& l) noexcept {
  std::cout << "calling makeIteratorPair" << std::endl;
  return std::move(IteratorPair<Iterator>(std::forward<Iterator>(f), std::forward<Iterator>(l)));
}

} // namespace core

#endif  // SRC_LIB_CORE_ITERATORPAIR_H_
