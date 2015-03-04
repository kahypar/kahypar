#ifndef SRC_LIB_CORE_ITERATORPAIR_H_
#define SRC_LIB_CORE_ITERATORPAIR_H_

namespace core {
// based on:
// http://cplusplusmusings.wordpress.com/2013/04/14/range-based-for-loops-and-pairs-of-iterators/

template <typename Iterator>
class IteratorPair {
  public:
  IteratorPair(Iterator first, Iterator last) noexcept : _f(first), _l(last) { }
  IteratorPair(IteratorPair&& other) :
      _f(std::move(other._f)),
      _l(std::move(other._l)) { }
  Iterator begin() const { return _f; }
  Iterator end() const { return _l; }

  private:
  const Iterator _f;
  const Iterator _l;
};

template <typename Iterator>
IteratorPair<Iterator> makeIteratorPair(Iterator f, Iterator l) noexcept {
  return std::move(IteratorPair<Iterator>(f, l));
}

} // namespace core

#endif  // SRC_LIB_CORE_ITERATORPAIR_H_
