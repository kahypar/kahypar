// TypeList.hpp - definitions for typelists
// Project: TypeList

#include <type_traits>
#include <tuple>

#pragma once

namespace typelist
{
//-----------------------------------------------------------------------------
/// Helper to use with is_tlist, is_iseq (borrowed)

  template <typename T>
  struct type_to_type
  {
    using orig = T;
    type_to_type() {}
  };

/////////////////////////////////////////////////////////////////////////////
/// Helpers to work with integer_seqyense

//-----------------------------------------------------------------------------
/// Is the class integer-sequence

  template <typename T>
  struct is_iseq
  {
  private:
    using yes = char(&)[1];
    using no  = char(&)[2];

    template <typename U, U... Is>
    static yes Test(type_to_type<std::integer_sequence<U, Is...>>) {}

    static no Test(...) {}

  public:
    using type = std::integral_constant<bool, sizeof(Test(type_to_type<T>())) == 1>;
    using value_type = typename type::value_type;
    static constexpr value_type value = type::value;
  };

//-----------------------------------------------------------------------------
/// Get a number at the index 

  template <size_t idx, class ISeq>
  struct iseq_nmb_at_impl;

  template <typename U, U head, U... Is>  /// End of search, number is found
  struct iseq_nmb_at_impl < 0, std::integer_sequence<U, head, Is...> >
  {
    using type = std::integral_constant<U, head>;
  };

/// Recursion
  template <size_t idx, typename U, U head, U...Is>
  struct iseq_nmb_at_impl<idx, std::integer_sequence<U, head, Is...>>
  {
    using type = typename iseq_nmb_at_impl<idx - 1, std::integer_sequence<U, Is...>>::type;
  };

/// Wrapper; tests the index, has added 'value_type' and 'value' members
  template <size_t idx, class ISeg>
  struct iseq_nmb_at;

  template <size_t idx, typename U, U... Is>
  struct iseq_nmb_at<idx, std::integer_sequence<U, Is...>>
  {
  private:
    static_assert(std::integer_sequence<U, Is...>::size() > idx,
      "iseq_nmb_at: Index is out of sequence bounds");
  public:
    using type = typename iseq_nmb_at_impl<idx, std::integer_sequence<U, Is...>>::type; 
    using value_type = typename type::value_type;
    static constexpr value_type value = type::value;
  };

//-----------------------------------------------------------------------------
/// Get the index of a number

  template <size_t idx, typename U,U nmb, class ISeq>
  struct iseq_index_of_impl;

  template <size_t idx, typename U, U nmb>                  /// Number not the in sequence
  struct iseq_index_of_impl<idx, U, nmb, std::integer_sequence<U>>
  {
    using type = std::integral_constant<int, -1>;        /// Stop recursion
  };

  template <size_t idx, typename U, U nmb, U... Ns>        /// Number found
  struct iseq_index_of_impl<idx, U, nmb, std::integer_sequence<U, nmb, Ns...>>
  {
    using type = std::integral_constant<int, idx>;       
  };

  template <size_t idx, typename U, U nmb, U head, U... Ns>
  struct iseq_index_of_impl<idx, U, nmb, std::integer_sequence<U, head, Ns...>>
  {
    using type = typename iseq_index_of_impl<idx + 1, U, nmb, std::integer_sequence<U, Ns...>>::type;
  };

/// Wrapper to test arguments, supply starting index 0, and add members 'value_type' and 'value'

  template <typename U, U nmb, class ISeq>
  struct iseq_index_of
  {
  private:
    static_assert(is_iseq<ISeq>::value, 
      "iseq_index_of: The class supplied is not std::integer_sequence");
    using data_type = typename ISeq::value_type;       
    static_assert(std::is_same<U, data_type>::value, 
      "iseq_index_of: Number must be the same type as the sequence");
  public:
    using type = typename iseq_index_of_impl<0, data_type, nmb, ISeq>::type;
    using value_type = typename type::value_type;
    static constexpr value_type value = type::value;
  };

//-----------------------------------------------------------------------------
/// Remove some number from a sequence

  template <typename U, U nmb, class IntSeq1, class IntSeq2>
  struct iseq_remove_nmb_impl;

  template <typename U, U nmb, U... Ns>      /// Called on empty sequence or the number not found
  struct iseq_remove_nmb_impl<U, nmb, std::integer_sequence<U, Ns...>, std::integer_sequence<U>>
  {
    using type = std::integer_sequence<U, Ns...>;
  };

  template <typename U, U nmb, U... N1s, U... N2s>  // Number found and removed
  struct iseq_remove_nmb_impl<U, nmb, std::integer_sequence<U, N1s...>, std::integer_sequence<U, nmb, N2s...>>
  {
    using type = std::integer_sequence<U, N1s..., N2s...>;
  };

  template <typename U, U nmb, U head, U... N1s, U... N2s>
  struct iseq_remove_nmb_impl <U, nmb, std::integer_sequence<U, N1s...>, std::integer_sequence<U, head, N2s...>>
  {
    using type = typename iseq_remove_nmb_impl<U, nmb, std::integer_sequence<U, N1s..., head>, 
                                                                  std::integer_sequence<U, N2s...>>::type;
  };

/// Wrapper

  template <typename U, U nmb, class ISeq>
  struct iseq_remove_nmb
  {
  private:
    static_assert ( is_iseq<ISeq>::value, 
      "iseq_remove_nmb: Supplied class is not integral_sequence");
    using data_type = typename ISeq::value_type;
    static_assert(std::is_same<U, data_type>::value, 
      "iseq_remove_nmb: Number must be of the type of sequence numbers");
  public:
    using type =
      typename iseq_remove_nmb_impl<data_type, nmb, std::integer_sequence<data_type>, ISeq>::type;
  };

//-----------------------------------------------------------------------------
/// Push a number back:
  template <typename T, T nmb, typename U, class ISeg>
  struct iseq_push_back_impl;

  template <typename T, T nmb, typename U, U... Ns>
  struct iseq_push_back_impl<T, nmb, U, std::integer_sequence<U, Ns...>>
  {
    using type = std::integer_sequence<U, Ns..., nmb>;
  };

/// Wrapper to check template arguments
  template <typename T, T nmb, class ISeq>
  struct iseq_push_back
  {
  private:
    static_assert(is_iseq<ISeq>::value, 
      "iseq_push_back: Supplied class is not std::integer_sequence");
    using data_type = typename ISeq::value_type;
    static_assert(std::is_same<T, data_type>::value, 
      "iseq_push_back: Number must be of the type of sequence numbers");
  public:
    using type = typename iseq_push_back_impl<T, nmb, data_type, ISeq>::type;
  };

//----------------------------------------------------------------------------
/// Get the sum of std::integer_sequence members

  template <typename U, U sum, class IntSeq>
  struct iseq_sum_impl;

  template <typename U, U sum>                               /// End of recursion
  struct iseq_sum_impl <U, sum, std::integer_sequence< U>>
  {
    using type = typename std::integral_constant<U, sum>;
  };

  template <typename U, U sum, U head, U... Ns>             /// Recursion
  struct iseq_sum_impl < U, sum, std::integer_sequence<U, head, Ns...>>
  {
    using type = typename iseq_sum_impl<U, sum + head, std::integer_sequence<U, Ns...>>::type;
  };

  /// Wrapper to check template arguments and supply initial sum = 0
  template <class ISeq>
  struct iseq_sum
  {
  private:
    static_assert(is_iseq<ISeq>::value, "iseq_sum: Supplied class is not inteter_sequence");
    using data_type = typename ISeq::value_type;
  public:
    using type = typename iseq_sum_impl<data_type, 0, ISeq>::type;
    using value_type = typename type::value_type;
    static constexpr value_type value = type::value;
  };

///////////////////////////////////////////////////////////////////////////////
/// NullType and Sizeof to assign size zero to NullType

/// NullType definition
  class NullType
  {};

/// Helper: assigns size 0 to NullType, calculates sizeof(T) for others types

  template <typename T>
  struct Sizeof
  {
    static constexpr size_t value = std::is_same<T, NullType>::value ? 0 : sizeof(T);
  };

//-----------------------------------------------------------------------------
/// TypeList definition; defines size similar to  C++ STL containers size

  template <typename... Ts>
  struct tlist 
  {
    using type = tlist;
    static constexpr size_t size() noexcept { return sizeof...(Ts); }
  };

  using tlist_emptylist = tlist<>;

//-----------------------------------------------------------------------------
/// Is the list empty? Has 'type', 'value_type' and 'value' members

  template <class List>
  struct is_tlist_empty
  {
    using type = std::false_type; // integral_constant<bool, false>;
    using value_type = typename type::value_type;
    static constexpr value_type value = type::value;
  };

/// Specialization for an empty list

  template <>
  struct is_tlist_empty<tlist<>>
  {
    using type = std::true_type; // integral_constant<bool, true>;
    using value_type = type::value_type;
    static constexpr value_type value = type::value;
  };

//-----------------------------------------------------------------------------
/// Is this type a tlist?

  template <typename T>
  struct is_tlist
  {
  private:
    using yes = char(&)[1];
    using no  = char(&)[2];

    template <typename... Ts>
    static yes Test(type_to_type<tlist<Ts...>>) {}

    static no Test(...) {}

  public:
    using type = std::integral_constant<bool, sizeof(Test(type_to_type<T>())) == 1>;
    using value_type = typename type::value_type;
    static constexpr value_type value = type::value;
  };

  //---------------------------------------------------------------------------
  ///Test whether all list's elements are TypeLists: used as a switch

  template <typename List>    // Is called if List is not tlist
  struct is_listoflists       // or on empty list
  {
    using type = std::integral_constant<bool, false>;
    using value_type = typename type::value_type;
    static constexpr value_type value = type::value;
  };

  template <typename H, typename... ListEls>
  struct is_listoflists<tlist<H, ListEls...>>
  {
  private:
    static constexpr bool bLst = is_tlist<H>::value;
  public:
    using type = typename std::conditional < bLst,
      typename is_listoflists<tlist<ListEls...>>::type, std::integral_constant<bool, false >> ::type;
    using value_type = typename type::value_type;            // Goes there only not tlist,stops
    static constexpr value_type value = type::value;
  };

  template <typename ListEl>                                 // Got to the end of original list
  struct is_listoflists<tlist<ListEl>>
  {
  private:
    static constexpr bool bLst = is_tlist<ListEl>::value;
  public:
    using type = std::integral_constant<bool, bLst>;
    using value_type = typename type::value_type;
    static constexpr value_type value = type::value;
  };

//-----------------------------------------------------------------------------
/// Access an element by an index

  template <size_t idx, class List>
  struct tlist_type_at_impl;

  template <typename T, typename... Ts>  ///> End of search, type is found
  struct tlist_type_at_impl < 0, tlist<T, Ts...> >
  {
    using type = T;
  };

/// Recursion
  template <size_t idx, typename T, typename...Ts>
  struct tlist_type_at_impl <idx, tlist<T, Ts...>>
  {
    using type = typename tlist_type_at_impl<idx - 1, tlist<Ts...>>::type;
  };

/// Wrapper
  template <size_t idx, class List>
  struct tlist_type_at;

  template <size_t idx, typename... Ts>
  struct tlist_type_at<idx, tlist<Ts...>>
  {
  private:
    static_assert(sizeof...(Ts) > idx, 
      "tlist_type_at: Index out of bounds or called on empty list");
  public:
    using type = typename tlist_type_at_impl<idx, tlist<Ts...>>::type;

  };

//-----------------------------------------------------------------------------
/// Search a typelist for a first occurrence of the type T

/// Implementation: has index as a template parameter
  template <size_t idx, typename T, class List>
  struct tlist_index_of_impl;

  template <size_t idx, typename T> /// The type T is not in the list
  struct tlist_index_of_impl <idx, T, tlist<>>
  {
    using type = std::integral_constant<int, -1>;
  };

  template <size_t idx, typename T, typename... Ts>    ///> The type is found
  struct tlist_index_of_impl <idx, T, tlist<T, Ts...>>
  {
    using type = std::integral_constant<int, idx>;
  };

  template <size_t idx, typename T, typename H, typename... Ts>  ///> Recursion
  struct tlist_index_of_impl <idx, T, tlist<H, Ts...>>
  {
    using type = typename tlist_index_of_impl<idx + 1, T, tlist<Ts...>>::type;
  };

/// Wrapping to supply initial index 0
  template <typename T, class List>
  struct tlist_index_of;

/// Specializing for idx >= 0
  template <typename T, typename... Ts>
  struct tlist_index_of<T, tlist<Ts...>>
  {
    using type = typename tlist_index_of_impl<0, T, tlist<Ts...>>::type;
    using value_type = typename type::value_type;
    static constexpr value_type value = type::value;
  };

//-----------------------------------------------------------------------------
/// Find the first element not equal to argument T

  template <size_t idx, typename T, class List>
  struct tlist_index_of_not_impl;

  template <size_t idx, typename T>             // All list's elements are T
  struct tlist_index_of_not_impl<idx, T, tlist<>>
  {
    using type = std::integral_constant<int, -1>;
  };

  template <size_t idx, typename T, typename H, typename... Ts>
  struct tlist_index_of_not_impl <idx, T, tlist<H, Ts...>>
  {
    using type = typename std::conditional < std::is_same<T, H>::value,
      typename tlist_index_of_not_impl<idx + 1, T, tlist<Ts...>>::type,
      std::integral_constant<int, idx >> ::type;
  };

/// Wrapper
  template <typename T, class List>
  struct tlist_index_of_not;

  template <typename T, typename... Ts>
  struct tlist_index_of_not<T, tlist<Ts...>>
  {
    using type = typename tlist_index_of_not_impl<0, T, tlist<Ts...>>::type;
    using value_type = typename type::value_type;
    static constexpr value_type value = type::value;
  };

//-----------------------------------------------------------------------------
/// Access to the front and the back of the list

  template <class Lists>
  struct tlist_front; 

  template <typename... Ts>
  struct tlist_front<tlist<Ts...>>
  {
  private:
    static_assert(sizeof...(Ts) > 0, "tlist_front: Called on empty list");
  public:
    using type = typename tlist_type_at<0, tlist<Ts...>>::type;
  };

  template <class List>
  struct tlist_back;

  template <typename... Ts>
  struct tlist_back<tlist<Ts...>>
  {
  private:
    static_assert(sizeof...(Ts) > 0, "tlist_back: Called on empty list");
  public:
    using type = typename tlist_type_at< (sizeof...(Ts)) - 1, tlist<Ts...>>::type;
  };

//-----------------------------------------------------------------------------
/// Push front and push back

  template <typename H, class List>
  struct tlist_push_front; ;

  template <typename H, typename... Ts>
  struct tlist_push_front<H, tlist<Ts...>>
  {
    using type = tlist<H, Ts...>;
  };

  template <typename T, class List>
  struct tlist_push_back;

  template <typename T, typename... Ts>
  struct tlist_push_back<T, tlist<Ts...>>
  {
    using type = tlist<Ts..., T>;
  };

//-----------------------------------------------------------------------------
/// Erase an element from the list

  template <typename T, class List>
  struct tlist_erase_type; 

  template <typename T>
  struct tlist_erase_type<T, tlist<>>
  {
    using type = tlist<>;
  };

  template <typename T, typename... Ts>
  struct tlist_erase_type <T, tlist<T, Ts...>>
  {
    using type = tlist<Ts...>;
  };

  template <typename T, typename H, typename... Ts>
  struct tlist_erase_type<T, tlist<H, Ts...>>
  {
    using type =
      typename tlist_push_front<H, typename tlist_erase_type<T, tlist<Ts...>>::type>::type;
  };

//-----------------------------------------------------------------------------
/// Erase an element from a typelist by its index

  template <size_t idx, class List>
  struct tlist_erase_at_impl; 

  template <typename T, typename... Ts>
  struct tlist_erase_at_impl<0, tlist<T, Ts...>>
  {
    using type = tlist<Ts...>;
  };

  template <size_t idx, typename H, typename... Ts>
  struct tlist_erase_at_impl<idx, tlist<H, Ts...>>
  {
    using type =
      typename tlist_push_front < H, typename tlist_erase_at_impl<idx - 1, tlist<Ts...>>::type>::type;
  };

/// Wrapper to assert index is in range
  template <size_t idx, class List>
  struct tlist_erase_at;

  template <size_t idx, typename... Ts>
  struct tlist_erase_at<idx, tlist<Ts...>>
  {
  private:
    static_assert(sizeof...(Ts) > idx, "tlist_erase_at: Index out of bounds");
  public:
    using type = typename tlist_erase_at_impl<idx, tlist<Ts...>>::type;
  };

//-----------------------------------------------------------------------------
/// Pop front, this can be called on empty list

  template <class List>
  struct tlist_pop_front;

  template <typename... Ts>
  struct tlist_pop_front<tlist<Ts...>>
  {
    using type = typename std::conditional < sizeof...(Ts) == 0, tlist<>,
                   typename  tlist_erase_at<0, tlist<Ts...>>::type> ::type;
  };

//-----------------------------------------------------------------------------
/// Pop back, allows an empty list

  template <class List>
  struct tlist_pop_back;

  template <typename... Ts>
  struct tlist_pop_back<tlist<Ts...>>
  {
    using type = typename std::conditional<sizeof...(Ts) == 0, tlist<>,
             typename tlist_erase_at<sizeof...(Ts)-1, tlist<Ts...>>::type>::type;
  };

//-----------------------------------------------------------------------------
/// Erase all occurrence of type

  template <typename T, class List>
  struct tlist_erase_all;

  template <typename T>
  struct tlist_erase_all<T, tlist<>>
  {
    using type = tlist<>;
  };

  template <typename T, typename... Ts>
  struct tlist_erase_all<T, tlist<T, Ts...>>
  {
    using type = typename tlist_erase_all<T, tlist<Ts...>>::type;
  };

  template <typename T, typename H, typename... Ts>
  struct tlist_erase_all<T, tlist<H, Ts...>>
  {
    using type = typename tlist_push_front < H, typename tlist_erase_all<T, tlist<Ts...>>::type>::type;
  };

//-----------------------------------------------------------------------------
/// Erase all elements of given type from a list of lists
/// Never use it as a standalone meta function, use tlist_deeperase_all instead

  template <typename T, class List>
  struct tlist_erase_all_ll
  {
    using type = tlist<>;
  };

  template <typename T, typename... Hs, typename... Ts>
  struct tlist_erase_all_ll<T, tlist<tlist<Hs...>, Ts...>>
  {
  private:
    using el_cleaned = typename tlist_erase_all<T, tlist<Hs...>>::type;
  public:
    using type =
      typename tlist_push_front<el_cleaned, typename tlist_erase_all_ll<T, tlist<Ts...>>::type>::type;
  };

//-----------------------------------------------------------------------------
/// Wrapper for deep erasure (inside each of list's elements)

  template <typename T, class List>
  struct tlist_deeperase_all;

  template <typename T, typename... ListEls>
  struct tlist_deeperase_all<T, tlist<ListEls...>>
  {
  private:
    static constexpr bool bLL = is_listoflists <tlist<ListEls...>>::value;
  public:
    using type = typename std::conditional < bLL,
      typename tlist_erase_all_ll<T, tlist<ListEls...>>::type,
      typename tlist_erase_all<T, tlist<ListEls...>>::type>::type;
  };

//-----------------------------------------------------------------------------
/// Erase all duplicates from the list

  template <class List>
  struct tlist_erase_dupl; 

  template <>
  struct tlist_erase_dupl<tlist<>>
  {
    using type = tlist<>;
  };

  template <typename H, typename... Ts>
  struct tlist_erase_dupl<tlist<H, Ts...>>
  {
  private:
    using unique_t = typename tlist_erase_dupl<tlist<Ts...>>::type;
    using new_t = typename tlist_erase_type<H, unique_t>::type;
  public:
    using type = typename tlist_push_front<H, new_t>::type;
  };

//---------------------------------------------------------------------------
/// Erase duplicates in all elements of list of lists
/// Not use it as a standalone meta function; use tlist_deeperase_dupl instead

  template <class List>
  struct tlist_erase_dupl_ll
  {
    using type = tlist<>;
  };
  
  template <typename... Hs, typename... ListEls>
  struct tlist_erase_dupl_ll<tlist<tlist<Hs...>, ListEls...>>
  {
  private:
    using lst_clean = typename tlist_erase_dupl < tlist<Hs...>>::type;
  public:
    using type =
      typename tlist_push_front<lst_clean,
      typename tlist_erase_dupl_ll<tlist<ListEls...>>::type>::type;;
  };

//---------------------------------------------------------------------------
/// Wrapper for a deep erasure of duplicates

  template <class List>
  struct tlist_deeperase_dupl;

  template <typename... ListEls>
  struct tlist_deeperase_dupl<tlist<ListEls...>>
  {
  private:
    static constexpr bool bLL = is_listoflists<tlist<ListEls...>>::value;
  public:
    using type = typename std::conditional < bLL,
      typename tlist_erase_dupl_ll<tlist<ListEls...>>::type,
      typename tlist_erase_dupl<tlist<ListEls...>>::type>::type;
  };

//---------------------------------------------------------------------------
/// Erase first k elements from the list

  template <size_t k, class List>
  struct tlist_erase_firsts; //{};

  template <size_t k>
  struct tlist_erase_firsts<k, tlist<>>
  {
    using type = tlist<>;
  };

  template <typename... Ts>
  struct tlist_erase_firsts < 1, tlist<Ts...>>
  {
    using type = typename tlist_pop_front < tlist<Ts...>>::type;
  };

  template <size_t k, typename... Ts>
  struct tlist_erase_firsts<k, tlist<Ts...>>
  {
  private:
    static_assert(k > 0, "Wrong number of items to erase (k must be > 0)");
  public:
    using type = typename tlist_erase_firsts<k - 1, typename tlist_pop_front<tlist<Ts...>>::type>::type;
  };

//-----------------------------------------------------------------------------
// Erase last k elements

  template <size_t k, class List>
  struct tlist_erase_lasts;

  template <size_t k>
  struct tlist_erase_lasts<k, tlist<>>
  {
    using type = tlist<>;
  };

  template <typename... Ts>
  struct tlist_erase_lasts<1, tlist<Ts...>>
  {
    using type = typename tlist_pop_back<tlist<Ts...>>::type;
  };

  template <size_t k, typename... Ts>
  struct tlist_erase_lasts<k, tlist<Ts...>>
  {
  private:
    static_assert(k > 0, "Wrong number of items to erase (k must be > 0)");
  public:
    using type =
      typename tlist_erase_lasts<k - 1, typename tlist_pop_back<tlist<Ts...>>::type>::type;
  };

//-----------------------------------------------------------------------------
/// Replace the first occurrence of the type in the typelist
/// R - replacement for the type T

  template <typename R, typename T, class List>
  struct tlist_replace_first; ;

  template <typename R, typename T>
  struct tlist_replace_first<R, T, tlist<>>
  {
    using type = tlist<>;
  };

  template <typename R, typename T, typename H, typename... Ts>
  struct tlist_replace_first<R, T, tlist<H, Ts...>>
  {
    using type = typename std::conditional<std::is_same<T, H>::value,
      tlist<R, Ts...>,
      typename tlist_push_front<H, typename tlist_replace_first<R, T, tlist<Ts...>>::type>::type>::type;
  };

//-----------------------------------------------------------------------------
/// Replace a type at index; R is a replacement

  template <size_t idx, typename R, class List>
  struct tlist_replace_at_impl;

  template <typename R, typename H, typename... Ts>   // Index reached, type replaced
  struct tlist_replace_at_impl<0, R, tlist<H, Ts...>>
  {
    using type = tlist<R, Ts...>;
  };

  template <size_t idx, typename R, typename H, typename... Ts>
  struct tlist_replace_at_impl<idx, R, tlist<H, Ts...>>  // Recursion
  {
    using type =
      typename tlist_push_front < H, typename tlist_replace_at_impl<idx - 1, R, tlist<Ts...>>::type>::type;
  };

/// Wrapper to check the index
  template <size_t idx, typename R, class List>
  struct tlist_replace_at;

  template <size_t idx, typename R, typename... Ts>
  struct tlist_replace_at<idx, R, tlist<Ts...>>
  {
  private:
    static_assert(sizeof...(Ts) > idx, "tlist_replace_at: Index is out of bounds");
  public:
    using type = typename tlist_replace_at_impl<idx, R, tlist<Ts...>>::type;
  };

//-----------------------------------------------------------------------------
/// Replace all occurrences of a given type in a tlist; R is a replacement for type T

  template <typename R, typename T, class List>
  struct tlist_replace_all;

  template <typename R, typename T>
  struct tlist_replace_all<R, T, tlist<>>
  {
    using type = tlist<>;
  };

  template <typename R, typename T, typename... Ts>
  struct tlist_replace_all<R, T, tlist<T, Ts...>>
  {
    using type = typename tlist_push_front<R, typename tlist_replace_all<R, T, tlist<Ts...>>::type>::type;
  };

  template <typename R, typename T, typename H, typename... Ts>
  struct tlist_replace_all<R, T, tlist<H, Ts...>>
  {
    using type = typename tlist_push_front<H, typename tlist_replace_all<R, T, tlist<Ts...>>::type>::type;
  };

//-----------------------------------------------------------------------------
/// Replace all occurrences of a type in a list of lists elements
/// Do not use it as a standalone meta function; use tlist_deepreplace_all instead

  template <typename R, typename T, class List>
  struct tlist_replace_all_ll
  {
    using type = tlist<>;
  };

  template <typename R, typename T, typename... Hs, typename... ListEls>
  struct tlist_replace_all_ll<R, T, tlist<tlist<Hs...>, ListEls...>>
  {
  private:
    using lst_replaced = typename tlist_replace_all<R, T, tlist<Hs...>>::type;
  public:
    using type =
      typename tlist_push_front < lst_replaced, 
      typename tlist_replace_all_ll<R, T, tlist<ListEls...>>::type>::type; 
  };

//-----------------------------------------------------------------------------
/// Wrapper to deep replacement

  template <typename R, typename T, class List>
  struct tlist_deepreplace_all;

  template <typename R, typename T, typename... ListEls>
  struct tlist_deepreplace_all<R, T, tlist<ListEls...>>
  {
  private:
    static constexpr bool bLL = is_listoflists <tlist<ListEls...>>::value;
  public:
    using type = typename std::conditional<bLL,
      typename tlist_replace_all_ll<R, T, tlist<ListEls...>>::type,
      typename tlist_replace_all<R, T, tlist<ListEls...>>::type>::type;
  };

//-----------------------------------------------------------------------------
/// Count a type in the list

  template <size_t k, typename T, class List>
  struct tlist_count_type_impl;

  template <size_t k, typename T >
  struct tlist_count_type_impl<k, T, tlist<>>
  {
    using type = std::integral_constant<size_t, k>;
  };

  template <size_t k, typename T, typename H, typename... Ts>
  struct tlist_count_type_impl<k, T, tlist<H, Ts...>>
  {
  private:
    static constexpr size_t count = std::is_same<T, H>::value ? k + 1 : k;
  public:
    using type = typename tlist_count_type_impl<count, T, tlist<Ts...>>::type;
  };

/// Wrapper
  template <typename T, class Loist>
  struct tlist_count_type;

  template <typename T, typename... Ts>
  struct tlist_count_type<T, tlist<Ts...>>
  {
    using type = typename tlist_count_type_impl<0, T, tlist<Ts...>>::type;
    using value_type = typename type::value_type;
    static constexpr value_type value = type::value;
  };

//---------------------------------------------------------------------------
/// Count types in the list of lists
/// Do not use it as a standalone meta function; use tlist_deepcount_type instead

  template <typename T, class ISeq, class List>
  struct tlist_count_type_ll
  {
    using type = std::integer_sequence<size_t>;
  };

  template <typename T, size_t... Rs, typename... Hs >
  struct tlist_count_type_ll<T, std::integer_sequence<size_t, Rs...>, tlist<tlist<Hs...>>>
  {
  private:
    static constexpr size_t res_cnt = tlist_count_type<T, tlist<Hs...>>::value;
  public:
    using type = std::integer_sequence<size_t, Rs..., res_cnt>;
  };

  template <typename T, size_t... Rs, typename... Hs, typename... Ls >
  struct tlist_count_type_ll<T, std::integer_sequence<size_t, Rs...>, tlist<tlist<Hs...>, Ls...>>
  {
  private:
    static constexpr size_t res_cnt = tlist_count_type<T, tlist<Hs...>>::value;
  public:
    using type = typename tlist_count_type_ll<T, std::integer_sequence<size_t, Rs..., res_cnt>, tlist<Ls...>>::type;
  };

//-----------------------------------------------------------------------------
/// Wrap it; result is std::integral_constant<size_t, count> for plain list
/// For the list of lists it is std::integer_sequence<size_t, counts...>

  template <typename T, class List>
  struct tlist_deepcount_type;

  template <typename T, typename... Ls>
  struct tlist_deepcount_type<T, tlist<Ls...>>
  {
/// Condition to select path
  private:
    static constexpr bool bLL = is_listoflists<tlist<Ls...>>::value;
  public:
    using type = typename std::conditional<bLL,
      typename tlist_count_type_ll<T, std::integer_sequence<size_t>, tlist<Ls...>>::type,
      typename tlist_count_type<T, tlist<Ls...>>::type>::type;
  };

//-----------------------------------------------------------------------------
/// Get all indexes of a type in a typelist; the result is a std::integer_sequence
/// Implementation

  template <size_t k, typename T, class List, class IntSeq>
  struct tlist_all_idx_impl;

  template <size_t k, typename T, size_t... Idx>                    // End of search
  struct tlist_all_idx_impl<k, T, tlist<>, std::integer_sequence<size_t, Idx...>>
  {
    using type = std::integer_sequence<size_t, Idx...>;
  };

  template <size_t k, typename T, typename... Ts, size_t... Idx>    // Type found at k
  struct tlist_all_idx_impl<k, T, tlist<T, Ts...>, std::integer_sequence<size_t, Idx...>>
  {
    using type =
      typename tlist_all_idx_impl<k + 1, T, tlist<Ts...>, std::integer_sequence<size_t, Idx..., k>>::type;
  };

  template <size_t k, typename T, typename H, typename... Ts, size_t... Idx> // Different type, continue
  struct tlist_all_idx_impl<k, T, tlist<H, Ts...>, std::integer_sequence<size_t, Idx...>>
  {
    using type =
      typename tlist_all_idx_impl<k + 1, T, tlist<Ts...>, std::integer_sequence<size_t, Idx...>>::type;
  };

/// Wrapper;; Supply the start index zero
  template <typename T, class List>
  struct tlist_all_idx;

  template <typename T, typename... Ts>
  struct tlist_all_idx<T, tlist<Ts...>>
  {
    using type = typename tlist_all_idx_impl<0, T, tlist<Ts...>, std::integer_sequence<size_t>>::type;
  };

//-----------------------------------------------------------------------------
/// Get all indexes of a type in each element of a list of lists
/// Do not use it as a standalone meta function; use tlist_all_deepidx instead

  template <typename T, class Res, class List>
  struct tlist_all_idx_ll
  {
    using type = tlist<std::integer_sequence<size_t>>;          // Will never called
  };

  template <typename T, typename... Rs, typename... Hs>         // End of search
  struct tlist_all_idx_ll<T, tlist<Rs...>, tlist<tlist<Hs...>>>
  {
  private:
    using iseq = typename tlist_all_idx_impl<0, T, tlist<Hs...>, std::integer_sequence<size_t>>::type;
  public:
    using type = tlist<Rs..., iseq>;
  };

  template <typename T, typename... Rs, typename... Hs, typename... ListEls>
  struct tlist_all_idx_ll<T, tlist<Rs...>, tlist<tlist<Hs...>, ListEls...>>
  {
  private:
    using iseq = typename tlist_all_idx_impl<0, T, tlist<Hs...>, std::integer_sequence<size_t>>::type;
  public:
    using type =
      typename tlist_all_idx_ll<T, tlist<Rs..., iseq>, tlist<ListEls...>>::type;
  };

//-----------------------------------------------------------------------------
/// Wrapper: find all indexes of the given type in a list or in elements of list of lists

  template <typename T, class List>
  struct tlist_all_deepidx;

  template <typename T, typename... ListEls>
  struct tlist_all_deepidx<T, tlist<ListEls...>>
  {
  private:
    static constexpr bool bLL = is_listoflists < tlist<ListEls...>>::value;
  public:
    using type = typename std::conditional < bLL,
      typename tlist_all_idx_ll<T, tlist<>, tlist<ListEls...>>::type,
      typename tlist_all_idx_impl<0, T, tlist<ListEls...>, std::integer_sequence<size_t>>::type>::type;
  };

//-----------------------------------------------------------------------------
/// Helper: Check whether all elements of a list are std::integer_sequences

  template <class List>
  struct is_listofiseqs                     // Called if the class is not a tlist
  {
    using type = std::integral_constant<bool, false>;
    using value_type = typename type::value_type;
    static constexpr value_type value = type::value;
  };

  template <>
  struct is_listofiseqs<tlist<>>
  {
    using type = std::integral_constant<bool, false>;
    using value_type = type::value_type;
    static constexpr value_type value = type::value;
  };

  template <typename H>
  struct is_listofiseqs<tlist<H>>
  {
  private:
    static constexpr bool bISeq = is_iseq<H>::value;
  public:
    using type = std::integral_constant<bool, bISeq>;
    using value_type = typename type::value_type;
    static constexpr value_type value = type::value;
  };
  
  template <typename H, typename... ISeqs>
  struct is_listofiseqs<tlist<H, ISeqs...>>
  {
  private:
    static constexpr bool bISeq = is_iseq<H>::value;
  public:
    using type = typename std::conditional < bISeq,
      typename is_listofiseqs<tlist<ISeqs...>>::type, std::integral_constant<bool, false>>::type;
    using value_type = typename type::value_type;
    static constexpr value_type value = type::value;
  };

//-----------------------------------------------------------------------------
/// Reverse order of members of the list implementation: needs empty list as destiny

  template <class Src, class Rsvr>
  struct tlist_reverse_impl;

  template <typename... Rs>
  struct tlist_reverse_impl<tlist<>, tlist<Rs...>>
  {
    using type = tlist<Rs...>;
  };

  template <class H, class... Rs >
  struct tlist_reverse_impl<tlist<H>, tlist<Rs...>>
  {
    using type = tlist <H, Rs... >;
  };

  template <class H, class... Ts, class... Rs>
  struct tlist_reverse_impl<tlist<H, Ts...>, tlist<Rs...>>
  {
    using type = typename tlist_reverse_impl <  tlist<Ts...>, tlist<H, Rs...>>::type;
  };

/// Wrapper to hide starting empty resulting list inside the wrapper

  template <class List>
  struct tlist_reverse; 

  template <typename... Ts>
  struct tlist_reverse<tlist<Ts...>>
  {
    using type = typename tlist_reverse_impl<tlist<Ts...>, tlist<>>::type;
  };


//---------------------------------------------------------------------------
/// Concatenate typelists

/// Implementation
  template <class ResList, class OrigLists>
  struct tlist_concat_lists_impl;

  template <typename... Rs, typename... Ts>                     // Last OrigList 
  struct tlist_concat_lists_impl<tlist<Rs...>, tlist<Ts...>>
  {
    using type = tlist<Rs..., Ts...>;
  };

  template <typename... Rs, typename... Ts, typename... Rests>  // Recursion
  struct tlist_concat_lists_impl<tlist<Rs...>, tlist<tlist<Ts...>, Rests...>>
  {
    using type = typename tlist_concat_lists_impl<tlist<Rs..., Ts...>, tlist<Rests...>>::type;
  };

/// Flatten a list of lists (convert it to a plain list) - is used as standalone and in a wrapper
  template <class ListOfLists>
  struct tlist_flatten_list;

  template <typename... Lists>
  struct tlist_flatten_list<tlist<Lists...>>
  {
  private:
    static_assert(is_listoflists<tlist<Lists...>>::value, 
                                 "tlist_flatten: Argument is not a tlist of lists");
  public:
    using type = typename tlist_concat_lists_impl<tlist<>, tlist<Lists...>>::type;
  };

  template <typename... Lists>
  struct tlist_concat_lists
  { 
  private:
    using temp_list = tlist<Lists...>;
    static_assert(is_listoflists<temp_list>::value, "Not all arguments are lists");
  public:
    using type = typename tlist_flatten_list<temp_list>::type;
  };

//-----------------------------------------------------------------------------
/// Helper: Move first k elements from list L2 to the tail of the list T1
/// Type is the list of two lists, one with added elements, other with a rest

  template <size_t k, class Lst1, class Lst2>
  struct tlist_move_firsts;
  
  template <typename H, typename... T1s, typename... T2s>
  struct tlist_move_firsts<1, tlist<T1s...>, tlist<H, T2s...>>
  {
    using type = tlist < tlist<T1s..., H>, tlist<T2s... >>;
  };

  template <size_t k, typename H, typename... T1s, typename... T2s>
  struct tlist_move_firsts<k, tlist<T1s...>, tlist<H, T2s...>>
  {
    static_assert (k > 0, "Number of elements to transfer must be > 0");
    using type = typename tlist_move_firsts<k - 1, tlist<T1s..., H>, tlist<T2s...>>::type;
  };

//---------------------------------------------------------------------------
/// Split a TypeList into multiple lists; in tlist_split_impl numbers of elements 
//  in lists are in integer sequence

  template <class Idx, class List1, class List2>
  struct tlist_split_impl;

  template <size_t l, typename... Lists, typename... Tails>
  struct tlist_split_impl<std::integer_sequence<size_t, l>, tlist<Lists...>, tlist<Tails...>>
  {
  private:
    using lst_split_2 = typename tlist_move_firsts<l, tlist<>, tlist<Tails...>>::type;
    using first = typename tlist_type_at<0, lst_split_2>::type;
    using second = typename tlist_type_at<1, lst_split_2>::type;
  public:
    using type = tlist<Lists..., first, second>;
  };

  template <size_t l, size_t... Ls, typename ... Lists, typename... Tails>
  struct tlist_split_impl < std::integer_sequence<size_t, l, Ls...>, tlist<Lists...>, tlist<Tails...>>
  {
  private:
    using lst_split_2 = typename tlist_move_firsts<l, tlist<>, tlist<Tails...>>::type;
    using first = typename tlist_type_at<0, lst_split_2>::type;
    using second = typename tlist_type_at<1, lst_split_2>::type;
    using lst_res = tlist<Lists..., first>;
  public:
    using type = typename tlist_split_impl<std::integer_sequence<size_t, Ls...>, lst_res, second>::type;
  };

/// Wrapper to save the user from generating std::integer_sequence; will do it inside the structure

  template < class List, size_t... Ls>
  struct tlist_split;

  template <typename... Ts, size_t... Ls>
  struct tlist_split<tlist<Ts...>, Ls...>
  {
  private:
// Check the sum of component lengths; must be less than size of original list
    using lengths = std::integer_sequence<size_t, Ls...>;
    using lengths_sum = typename iseq_sum<lengths>::type;
    static_assert(lengths_sum::value < tlist<Ts...>::size(), 
      "tlist_split: Length of a last component must be greater than zero; check the sum of component sizes");
// No zero lengths are allowed
    static_assert(iseq_index_of<size_t, 0, lengths>::value == -1,
      "Multi_Split: Remove components with zero size from the sequence of sizes");
  public:   // Go
    using type = typename tlist_split_impl<std::integer_sequence<size_t, Ls...>, tlist<>, tlist<Ts...>>::type;
  };

//-----------------------------------------------------------------------------
/// tlist_max_type_impl gets first element of max size in a plain list

  template <typename T, class List>
  struct tlist_max_type_impl; 

  template <typename T>                   // End of recursion
  struct tlist_max_type_impl<T, tlist<>>
  {
    using max_type = T;
  };

  template <typename T, typename H, typename... Ts>
  struct tlist_max_type_impl<T, tlist<H, Ts...>>
  {
  private:
    static constexpr bool  bRes = Sizeof<T>::value >= Sizeof<H>::value ? true : false;
    using selected = typename std::conditional<bRes, T, H>::type;
  public:
    using max_type = typename tlist_max_type_impl<selected, tlist<Ts...>>::max_type;
  };

/// Wrapper to apply to a plain list (Starts from a NullType, size = 0
  template <class List>
  struct tlist_max_type_size; 

  template <typename... Ts>
  struct tlist_max_type_size<tlist<Ts...>>
  {
  private:
    using max_type = typename tlist_max_type_impl < NullType, tlist<Ts... >> ::max_type;
  public:
    using type = std::integral_constant<size_t, Sizeof<max_type>::value>;
    using value_type = typename type::value_type;
    static constexpr value_type value = type::value;
  };

//-----------------------------------------------------------------------------
/// Find max size of elements in each list of a list of lists

  template <class ISeq, class List>
  struct tlist_max_type_size_ll    // Never executed, but needed for std::std::conditional
  {
    using type = std::integer_sequence<size_t>;
  };

  template <size_t ... Ids, typename... Hs>             // End of recursion
  struct tlist_max_type_size_ll<std::integer_sequence<size_t, Ids...>, tlist<tlist<Hs...>>>
  {
  private:
    static constexpr size_t loc_max = tlist_max_type_size<tlist<Hs...>>::value;
  public:
    using type = std::integer_sequence<size_t, Ids..., loc_max>;
  };

  template <size_t... Ids, typename... Hs, typename... ListEls>
  struct tlist_max_type_size_ll<std::integer_sequence<size_t, Ids...>, tlist<tlist<Hs...>, ListEls...>>
  {
  private:
    static constexpr size_t loc_max = tlist_max_type_size<tlist<Hs...>>::value;
  public:
    using type =
      typename tlist_max_type_size_ll<std::integer_sequence<size_t, Ids..., loc_max>, tlist<ListEls...>>::type;
  };

//-----------------------------------------------------------------------------
/// Wrapper: find max size of elements in a plain list or max elements in
///a list of lists

  template <class List>
  struct tlist_max_type_deepsize;

  template <typename... ListEls>
  struct tlist_max_type_deepsize<tlist<ListEls...>>
  {
  private:
    static constexpr bool bLL = is_listoflists<tlist<ListEls...>>::value;
  public:
    using type = typename std::conditional < bLL,
      typename tlist_max_type_size_ll < std::integer_sequence<size_t>, tlist<ListEls...>>::type,
      typename tlist_max_type_size<tlist<ListEls...>>::type > ::type;
 };

//-----------------------------------------------------------------------------
/// Find the first element of given size in the list of mixed element types

  template <size_t k, class List>
  struct tlist_firsttype_of_size;

  template <size_t k>
  struct tlist_firsttype_of_size<k, tlist<>>
  {
    using type = std::false_type;
  };

  template <size_t k, typename H, typename... Ts>
  struct tlist_firsttype_of_size< k, tlist<H, Ts...>>
  {
    using type =
      typename std::conditional < Sizeof<H>::value == k, H,
      typename tlist_firsttype_of_size<k, tlist<Ts...>>::type>::type;
  };

//------------------------------------------------------------------------------
/// Find the first element of given size in each list of a list of lists; result is a tlist
 
    template <size_t k, class ResLst, class List> /// It is only to compile; 
    struct tlist_firstltype_of_size                /// a branch in the wrapper
    {                                             /// is never called
      using type = tlist<>;
    };
   

    template <size_t k, typename... Rs, typename... Hs>           // Stop recursion
    struct tlist_firstltype_of_size<k, tlist<Rs...>, tlist<tlist<Hs...>>>
    {
    private:
      using type_found = typename tlist_firsttype_of_size<k, tlist<Hs...>>::type;
    public:
      using type = tlist<Rs..., type_found>;
    };

    template <size_t k, typename... Rs, typename... Hs, typename... Ts>
    struct tlist_firstltype_of_size < k, tlist <Rs...>, tlist<tlist<Hs...>, Ts...>>
    {
    private:
      using type_found = typename tlist_firsttype_of_size < k, tlist<Hs...>>::type;
    public:
      using type = typename tlist_firstltype_of_size<k, tlist<Rs..., type_found>, tlist<Ts...>>::type;
    };

//---------------------------------------------------------------------------
/// Wrapper

  template <size_t k, class List>
  struct tlist_first_typedeep_of_size;
  
   template <size_t k, typename... ListEls>
   struct tlist_first_typedeep_of_size<k, tlist<ListEls...>>
   {
   private:
     static constexpr bool bLs = is_listoflists<tlist<ListEls...>>::value;
   public:
     using type = typename std::conditional < bLs, 
       typename tlist_firstltype_of_size < k, tlist<>, tlist<ListEls...>>::type,
       typename tlist_firsttype_of_size<k, tlist<ListEls...>>::type>::type;
   };
  
//-----------------------------------------------------------------------------
/// Get all types of given size from the plain list of mixed elements, no duplicates

  template <size_t k, class ListRes, class ListSoource>
  struct tlist_all_ttypes_of_size;

  template <size_t k, typename... Rs>
  struct tlist_all_ttypes_of_size<k, tlist<Rs...>, tlist<>>
  {
   using type = typename tlist_erase_dupl<tlist<Rs...>>::type;
  };

  template <size_t k, typename H, typename... Rs, typename... Ts>
  struct tlist_all_ttypes_of_size<k, tlist<Rs...>, tlist<H, Ts...>>
  {
  private:
    using res_lst = typename std::conditional < Sizeof<H>::value == k, tlist<Rs..., H>, tlist < Rs...>>::type;
  public:
    using type =
      typename tlist_all_ttypes_of_size<k, res_lst, tlist<Ts...>>::type;
  };

//---------------------------------------------------------------------------
/// Wrapper to use without looking inside the list's elements of tlist type 

  template <size_t k, class List>
  struct tlist_all_types_of_size;

  template <size_t k, typename... Ts>
  struct tlist_all_types_of_size<k, tlist<Ts...>>
  {
    using type = 
      typename tlist_all_ttypes_of_size<k, tlist<>, tlist<Ts...>>::type;
  };
//---------------------------------------------------------------------------
/// Get all types of size k from a list of lists

  template <size_t k, class ListRes, class ListSource>
  struct tlist_all_ltypes_of_size
  {
    using type = tlist<>;
  };

  template <size_t k, typename... RLs, typename... Hs>
  struct tlist_all_ltypes_of_size<k, tlist<RLs...>, tlist<tlist<Hs...>>>
  {
  private:
    using types_found = typename tlist_all_types_of_size < k, tlist<Hs...>>::type;
  public:
    using type = tlist<RLs..., types_found>;
  };

  template <size_t k, typename... RLs, typename... Hs, typename... Ts> 
  struct tlist_all_ltypes_of_size<k, tlist<RLs...>, tlist<tlist<Hs...>, Ts...>>
  {
  private:
    using types_found = typename tlist_all_types_of_size<k, tlist<Hs...>>::type;
  public:
    using type = 
      typename tlist_all_ltypes_of_size < k, tlist<RLs..., types_found>, tlist<Ts...>>::type;
  };

//---------------------------------------------------------------------------
/// Wrapper

  template <size_t k, class List>
  struct tlist_all_typesdeep_of_size;

  template <size_t k, typename... ListEls>
  struct tlist_all_typesdeep_of_size<k, tlist<ListEls...>>
  {
  private:
    static constexpr bool bLs = is_listoflists<tlist<ListEls...>>::value;
  public:
    using type = typename std::conditional < bLs, 
      typename tlist_all_ltypes_of_size < k, tlist<>, tlist<ListEls...>>::type,
      typename tlist_all_ttypes_of_size<k, tlist<>, tlist<ListEls...>>::type>::type;
  };

//-----------------------------------------------------------------------------
/// Get all types of greater or greater and equal (bEqual ==true) of given size 
///  from the list of mixed elements, no duplicates

    template <bool bEqual, size_t k, class ListRes, class ListSoource>
    struct tlist_all_ttypes_greater;

    template <bool bEqual, size_t k, typename... Rs>
    struct tlist_all_ttypes_greater<bEqual, k, tlist<Rs...>, tlist<>>
    {
      using type = typename tlist_erase_dupl<tlist<Rs...>>::type;
    };

    template <bool bEqual, size_t k, typename H, typename... Rs, typename... Ts>
    struct tlist_all_ttypes_greater<bEqual, k, tlist<Rs...>, tlist<H, Ts...>>
    {
    private:
      static constexpr bool bComp = bEqual ? (Sizeof<H>::value) >= k : (Sizeof<H>::value) > k;
      using res_lst =
        typename std::conditional < bComp, tlist<Rs..., H>, tlist < Rs...>>::type;
    public:
      using type =
        typename tlist_all_ttypes_greater<bEqual, k, res_lst, tlist<Ts...>>::type;
    };

/// Wrapper to provide initial empty list for result
    template <bool bEqual, size_t k, class List>
    struct tlist_all_types_greater;

    template <bool bEqual, size_t k, typename... Ts>
    struct tlist_all_types_greater<bEqual, k, tlist<Ts...>>
    {
      using type = typename tlist_all_ttypes_greater<bEqual, k, tlist<>, tlist<Ts...>>::type;
    };
 
//-----------------------------------------------------------------------------
/// All types greater or equal of size k in a list of lists

  template <bool bEqual, size_t k, class ListRes, class ListSource>
  struct tlist_all_ltypes_greater
  {
    using type = tlist<>;
  };

  template <bool bEqual, size_t k, typename... RLs, typename... Hs>
  struct tlist_all_ltypes_greater<bEqual, k, tlist<RLs...>, tlist<tlist<Hs...>>>
  {
  private:
    using types_found = typename tlist_all_types_greater<bEqual, k, tlist<Hs...>>::type;
  public:
    using type = tlist<RLs..., types_found>; ;
  };

  template <bool bEqual, size_t k, typename... RLs, typename... Hs, typename... Ts>
  struct tlist_all_ltypes_greater<bEqual, k, tlist<RLs...>, tlist<tlist<Hs...>, Ts...>>
  {
  private:
    using types_found = typename tlist_all_types_greater<bEqual, k, tlist<Hs...>>::type;
  public:
    using type =
      typename tlist_all_ltypes_greater <bEqual, k, tlist<RLs..., types_found>, tlist<Ts...>>::type;
  };

//-----------------------------------------------------------------------------
/// Wrapper for types greater than size k

  template <bool bEqual, size_t k, class ListSources>
  struct tlist_all_typesdeep_greater;

  template <bool bEqual, size_t k, typename... ListEls>
  struct tlist_all_typesdeep_greater<bEqual, k, tlist<ListEls...>>
  {
  private:
    static constexpr bool bLs = is_listoflists<tlist<ListEls...>>::value;
  public:
    using type = typename std::conditional < bLs,
      typename tlist_all_ltypes_greater <bEqual, k, tlist<>, tlist<ListEls...>>::type,
      typename tlist_all_ttypes_greater<bEqual, k, tlist<>, tlist<ListEls...>>::type>::type;
  };

//-----------------------------------------------------------------------------
/// All types less or equal the given size (supply true to include equal)
/// for a list of mixed elements

  template <bool bEqual, size_t k, class ListRes, class ListSoource>
  struct tlist_all_ttypes_less;

  template <bool bEqual, size_t k, typename... Rs>
  struct tlist_all_ttypes_less<bEqual, k, tlist<Rs...>, tlist<>>
  {
    using type = typename tlist_erase_dupl<tlist<Rs...>>::type;
  };

  template <bool bEqual, size_t k, typename H, typename... Rs, typename... Ts>
  struct tlist_all_ttypes_less<bEqual, k, tlist<Rs...>, tlist<H, Ts...>>
  {
  private:
    static constexpr bool bComp = bEqual ? (Sizeof<H>::value) <= k : (Sizeof<H>::value) < k;
    using res_lst =
      typename std::conditional < bComp, tlist<Rs..., H>, tlist < Rs...>>::type;
  public:
    using type =
      typename tlist_all_ttypes_less<bEqual, k, res_lst, tlist<Ts...>>::type;
  };

// Initial result list wrapped
  template <bool bEqual, size_t k, class List>
  struct tlist_all_types_less;

  template <bool bEqual, size_t k, typename... Ts>
  struct tlist_all_types_less<bEqual, k, tlist<Ts...>>
  {
    using type = typename tlist_all_ttypes_less<bEqual, k, tlist<>, tlist<Ts...>>::type;
  };

//-----------------------------------------------------------------------------
/// All types less or equal size k in a list of lists

  template <bool bEqual, size_t k, class ListRes, class ListSource>
  struct tlist_all_ltypes_less
  {
    using type = tlist<>;
  };

  template <bool bEqual, size_t k, typename... RLs, typename... Hs>
  struct tlist_all_ltypes_less<bEqual, k, tlist<RLs...>, tlist<tlist<Hs...>>>
  {
  private:
    using types_found = typename tlist_all_types_less<bEqual, k, tlist<Hs...>>::type;  
  public:
    using type = tlist<RLs..., types_found>; ;
  };

  template <bool bEqual, size_t k, typename... RLs, typename... Ts, typename... Hs>
  struct tlist_all_ltypes_less<bEqual, k, tlist<RLs...>, tlist<tlist<Hs...>, Ts...>>
  {
  private:
    using types_found = typename tlist_all_ttypes_less<bEqual, k, tlist<>, tlist<Hs...>>::type;
  public:
    using type =
      typename tlist_all_ltypes_less <bEqual, k, tlist<RLs..., types_found>, tlist<Ts...>>::type;
  };

//-----------------------------------------------------------------------------
/// Wrapper for types lesser or equal than size k

  template <bool bEqual, size_t k, class ListSources>
  struct tlist_all_typesdeep_less;

  template <bool bEqual, size_t k, typename... ListEls>
  struct tlist_all_typesdeep_less<bEqual, k, tlist<ListEls...>>
  {
  private:
    static constexpr bool bLs = is_listoflists<tlist<ListEls...>>::value;
  public:
    using type = typename std::conditional < bLs,
      typename tlist_all_ltypes_less <bEqual, k, tlist<>, tlist<ListEls...>>::type,
      typename tlist_all_ttypes_less<bEqual, k, tlist<>, tlist<ListEls...>>::type>::type;
  };

//-------------------------------------------------------------------------------
/// Convert tlist to tuple and back (From Peter Dimov)

  template <class Src, template <typename...> class Trg>
  struct convert_impl;

  template <template <typename... > class Src, typename... Ts, template <typename...> class Trg>
  struct convert_impl<Src<Ts...>, Trg>
  {
    using type = Trg<Ts...>;
  };

  template <class Src, template <typename...> class Trg>
  using convert = typename convert_impl<Src, Trg>::type;

}// End of namespace TypeList

