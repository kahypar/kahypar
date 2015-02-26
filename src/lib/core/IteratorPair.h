#ifndef SRC_LIB_CORE_ITERATORPAIR_H_
#define SRC_LIB_CORE_ITERATORPAIR_H_

namespace core {
// based on:
// http://cplusplusmusings.wordpress.com/2013/04/14/range-based-for-loops-and-pairs-of-iterators/

template <typename Iterator>
class IteratorPair {
  public:
  IteratorPair(Iterator first, Iterator last) : _f(first), _l(last) { }
  Iterator begin() const { return _f; }
  Iterator end() const { return _l; }

  private:
  const Iterator _f;
  const Iterator _l;
};

template <typename Iterator>
IteratorPair<Iterator> makeIteratorPair(Iterator f, Iterator l) {
  // copy elision
  // see http://stackoverflow.com/questions/18849958/why-is-this-move-constructor-not-working
  return IteratorPair<Iterator>(f, l);
}

} // namespace core

#endif  // SRC_LIB_CORE_ITERATORPAIR_H_
