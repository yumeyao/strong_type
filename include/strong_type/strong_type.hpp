#ifndef SAFE_TYPE_SAFE_TYPE_HPP
#define SAFE_TYPE_SAFE_TYPE_HPP

#include <utility>
#include <type_traits>
#include <functional>
#include <concepts>
#include <iosfwd>

namespace strong
{

template <typename M, typename T>
using modifier = typename M::template modifier<T>;

template <typename T, typename Tag, typename ... Ms>
class type;

struct default_constructible
{
  template <typename T>
  class modifier;

  template <typename T, typename Tag, typename ... Ms>
  requires(std::is_default_constructible_v<T>)
  class modifier<type<T, Tag, Ms...>>
  {
  };
};

namespace impl {
  template<typename T>
  constexpr bool
  is_default_constructible_test(const typename default_constructible::template modifier<T> *) { return true; }

  constexpr bool is_default_constructible_test(...) { return false; }

  template<typename T>
  inline constexpr bool is_default_constructible =
    impl::is_default_constructible_test((const T *) nullptr);
}

struct uninitialized_t {};
inline constexpr uninitialized_t uninitialized{};

template <typename T, typename Tag, typename ... M>
class type : public modifier<M, type<T, Tag, M...>>...
{
public:
  constexpr
  type()
    noexcept(std::is_nothrow_default_constructible_v<T>)
    requires (std::is_default_constructible_v<T> &&
      impl::is_default_constructible<type>)
  : val{}
  {
  }

  explicit
  type(uninitialized_t)
    noexcept
  requires(std::is_trivially_constructible_v<T>)
  {
  }
  template <typename U>
  constexpr
  explicit
  type(
    std::initializer_list<U> us
  )
    noexcept(noexcept(T{us}))
    requires(std::is_constructible_v<T, std::initializer_list<U>>)
  : val{us}
  {
  }
  template <typename ... U>
  constexpr
  explicit
  type(
    U&& ... u)
  noexcept(std::is_nothrow_constructible<T, U...>::value)
  requires(std::is_constructible_v<T, U...> && sizeof...(U) > 0)
  : val(std::forward<U>(u)...)
  {}


  friend void swap(type& a, type& b) noexcept(
                                        std::is_nothrow_move_constructible<type>::value &&
                                        std::is_nothrow_move_assignable<type>::value
                                      )
  requires(std::swappable<T>)
  {
    using std::swap;
    swap(a.val, b.val);
  }

  friend constexpr T& value_of(type& p) noexcept { return p.val;}
  friend constexpr T&& value_of(type&& p) noexcept { return std::move(p).val;}
  friend constexpr const T& value_of(const type& p) noexcept { return p.val;}
  friend constexpr const T&& value_of(const type&& p) noexcept { return std::move(p).val;}

private:
  T val;
};
template <typename T>
T underlying_type_func(T);
template <typename T, typename Tag, typename ... Ms>
T underlying_type_func(type<T, Tag, Ms...>);

template <typename T>
using underlying_type_t = decltype(underlying_type_func(std::declval<T>()));

namespace impl {
  template <typename ... Ts>
  constexpr bool is_strong_type_func(const strong::type<Ts...>*) { return true;}
  constexpr bool is_strong_type_func(...) { return false;}
  template <typename T>
  concept strong_type = is_strong_type_func(static_cast<std::remove_cvref_t<T>*>(nullptr));

  template <typename T>
  constexpr auto&& access(T&& t)
  {
    if constexpr (strong_type<T>)
    {
      return value_of(std::forward<T>(t));
    }
    else
      return std::forward<T>(t);
  }

#if 0
  template<strong_type T>
  constexpr auto&& access(T&& t)
  {
    return value_of(std::forward<T>(t));
  }
  template <typename T>
  constexpr auto&& access(T&& t) requires (!strong_type<T>) { return std::forward<T>(t);}
#endif
}
struct equality
{
  template <typename>
  class modifier;
  template <typename T, typename Tag, typename ... Ms>
  class modifier<::strong::type<T, Tag, Ms...>>
  {
    using type = ::strong::type<T, Tag, Ms...>;
    friend constexpr bool operator==(const type& lh, const type& rh)
    noexcept(noexcept(value_of(lh) == value_of(rh)))
    {
      return value_of(lh) == value_of(rh);
    }
  };
};




struct semiregular
{
  template <typename>
  struct modifier;
};

template <typename T>
struct semiregular::modifier{};
template <std::semiregular T, typename Tag, typename ... M>
struct semiregular::modifier<::strong::type<T, Tag, M...>>
  : public default_constructible::modifier<::strong::type<T, Tag, M...>>
{
};

struct regular
{
  template <typename T>
  class modifier
    : public semiregular::modifier<T>
    , public equality::modifier<T>
  {
  };
};

struct ordered
{
  template <typename T>
  class modifier;
};

template <std::totally_ordered T, typename Tag, typename ... M>
class ordered::modifier<::strong::type<T, Tag, M...>>
{
  using type = ::strong::type<T, Tag, M...>;
public:
  [[nodiscard]] friend constexpr auto operator<(const type& lh, const type& rh)
  noexcept(noexcept(value_of(lh) < value_of(rh)))
  -> decltype(std::declval<const T&>() < std::declval<const T&>())
  {
    return value_of(lh) < value_of(rh);
  }
  [[nodiscard]] friend constexpr auto operator<=(const type& lh, const type& rh)
  noexcept(noexcept(value_of(lh) <= value_of(rh)))
  -> decltype(std::declval<const T&>() <= std::declval<const T&>())
  {
    return value_of(lh) <= value_of(rh);
  }
  [[nodiscard]] friend constexpr auto operator>(const type& lh, const type& rh)
  noexcept(noexcept(value_of(lh) > value_of(rh)))
  -> decltype(std::declval<const T&>() > std::declval<const T&>())
  {
    return value_of(lh) > value_of(rh);
  }
  [[nodiscard]] friend constexpr auto operator>=(const type& lh, const type& rh)
  noexcept(noexcept(value_of(lh) >= value_of(rh)))
  -> decltype(std::declval<const T&>() >= std::declval<const T&>())
  {
    return value_of(lh) >= value_of(rh);
  }

};

struct ostreamable
{
  template <typename T>
  class modifier
  {
  public:
    friend
    std::ostream&
    operator<<(
      std::ostream &os,
      const T& t)
    {
      return os << value_of(t);
    }
  };
};

struct istreamable
{
  template <typename T>
  class modifier
  {
  public:
    friend
    std::istream&
    operator>>(
      std::istream &is,
      T &t)
    {
      return is >> value_of(t);
    }
  };
};

struct iostreamable
{
  template <typename T>
  class modifier
    : public ostreamable::modifier<T>
    , public istreamable::modifier<T>
  {
  };
};

struct incrementable
{
  template <typename T>
  class modifier
  {
  public:
    constexpr
    T&
    operator++()
    noexcept(noexcept(++value_of(std::declval<T&>())))
    {
      auto &self = static_cast<T&>(*this);
      ++value_of(self);
      return self;
    }

    constexpr
    T
    operator++(int)
    {
      auto rv{static_cast<T&>(*this)};
      ++*this;
      return rv;
    }
  };
};

struct decrementable
{
  template <typename T>
  class modifier
  {
  public:
    constexpr
    T&
    operator--()
    noexcept(noexcept(--value_of(std::declval<T&>())))
    {
      auto &self = static_cast<T&>(*this);
      --value_of(self);
      return self;
    }

    constexpr
    T
    operator--(int)
    {
      T rv{static_cast<T&>(*this)};
      --*this;
      return rv;
    }
  };
};

struct bicrementable
{
  template <typename T>
  class modifier
    : public incrementable::modifier<T>
    , public decrementable::modifier<T>
  {
  };
};

struct boolean
{
  template <typename T>
  class modifier
  {
  public:
    explicit constexpr operator bool() const
    noexcept(noexcept(static_cast<bool>(value_of(std::declval<const T&>()))))
    {
      const auto& self = static_cast<const T&>(*this);
      return static_cast<bool>(value_of(self));
    }
  };
};

struct hashable
{
  template <typename T>
  class modifier{};
};

struct difference
{
  template <typename T>
  class modifier;
};

template <typename T, typename Tag, typename ... M>
class difference::modifier<::strong::type<T, Tag, M...>>
: public ordered::modifier<::strong::type<T, Tag, M...>>
{
  using type = ::strong::type<T, Tag, M...>;
public:
  type& operator+=(const type& t)
  noexcept(noexcept(std::declval<T&>() += value_of(t)))
  {
    auto& self = static_cast<type&>(*this);
    value_of(self) += value_of(t);
    return self;
  }

  type& operator-=(const type& t)
    noexcept(noexcept(std::declval<T&>() -= value_of(t)))
  {
    auto& self = static_cast<type&>(*this);
    value_of(self) -= value_of(t);
    return self;
  }

  type& operator*=(const T& t)
  noexcept(noexcept(std::declval<T&>() *= t))
  {
    auto& self = static_cast<type&>(*this);
    value_of(self) *= t;
    return self;
  }

  type& operator/=(const T& t)
    noexcept(noexcept(std::declval<T&>() /= t))
  {
    auto& self = static_cast<type&>(*this);
    value_of(self) /= t;
    return self;
  }

  friend
  type operator+(type lh, const type& rh)
  {
    lh += rh;
    return lh;
  }

  friend
  type operator-(type lh, const type& rh)
  {
    lh -= rh;
    return lh;
  }

  friend
  type operator*(type lh, const T& rh)
  {
    lh *= rh;
    return lh;
  }

  friend
  type operator*(const T& lh, type rh)
  {
    rh *= lh;
    return rh;
  }

  friend
  type operator/(type lh, const T& rh)
  {
    lh /= rh;
    return lh;
  }

  friend
  T operator/(const type& lh, const type& rh)
  {
    return value_of(lh) / value_of(rh);
  }
};


namespace impl
{
  template <typename T>
  concept subtractable = std::invocable<std::minus<>, const T&, const T&>;

  template <typename T>
  using diff_type = decltype(std::declval<const T&>() - std::declval<const T&>());
}

template <typename D>
struct affine_point
{
  template <typename T>
  class modifier;
};

template <typename D> template <typename>
class affine_point<D>::modifier {};

template <typename D>
template <typename T, typename Tag, typename ... M>
requires(std::constructible_from<D, impl::diff_type<T>>)
class affine_point<D>::modifier<::strong::type<T, Tag, M...>>
{
  using type = ::strong::type<T, Tag, M...>;
  using diff_type = decltype(std::declval<const T&>() - std::declval<const T&>());
public:
  [[nodiscard]]
  friend
  constexpr
  D
  operator-(
    const type& lh,
    const type& rh)
  {
    return D(value_of(lh) - value_of(rh));
  }

  type&
  operator+=(
    const D& d)
  noexcept(noexcept(std::declval<T&>() += value_of(d)))
  {
    auto& self = static_cast<type&>(*this);
    value_of(self) += value_of(d);
    return self;
  }

  type&
  operator-=(
    const D& d)
  noexcept(noexcept(std::declval<T&>() -= value_of(d)))
  {
    auto& self = static_cast<type&>(*this);
    value_of(self) -= value_of(d);
    return self;
  }

  [[nodiscard]]
  friend
  type
  operator+(
    const type& lh,
    const D& d)
  {
    return type(value_of(lh) + impl::access(d));
  }

  [[nodiscard]]
  friend
  type
  operator+(
    const D& d,
    const type& rh)
  {
    return type(impl::access(d) + value_of(rh));
  }

  [[nodiscard]]
  friend
  type
  operator-(
    const type& lh,
    const D& d)
  {
    return type(value_of(lh) - impl::access(d));
  }
};


struct pointer
{
  template <typename T>
  class modifier;
};

template <typename T, typename Tag, typename ... M>
class pointer::modifier<::strong::type<T, Tag, M...>>
{
  using type = strong::type<T, Tag, M...>;
public:
  template <typename TT = T>
  [[nodiscard]]
  friend
  constexpr
  auto
  operator==(
    const type& t,
    std::nullptr_t)
  noexcept(noexcept(std::declval<const TT&>() == nullptr))
  -> decltype(std::declval<const TT&>() == nullptr)
  {
    return value_of(t) == nullptr;
  }

  [[nodiscard]]
  constexpr
  auto&&
  operator*()
  const
  {
    const auto& self = static_cast<const type&>(*this);
    return *value_of(self);
  }

  constexpr auto* operator->() const { return std::addressof(operator*());}
};

struct arithmetic
{
  template <typename T>
  class modifier
  {
  public:
    [[nodiscard]]
    friend
    constexpr
    T
    operator-(
      const T &lh)
    {
      return T{-value_of(lh)};
    }

    friend
    constexpr
    T&
    operator+=(
      T &lh,
      const T &rh)
    noexcept(noexcept(value_of(lh) += value_of(rh)))
    {
      value_of(lh) += value_of(rh);
      return lh;
    }

    friend
    constexpr
    T&
    operator-=(
      T &lh,
      const T &rh)
    noexcept(noexcept(value_of(lh) -= value_of(rh)))
    {
      value_of(lh) -= value_of(rh);
      return lh;
    }

    friend
    constexpr
    T&
    operator*=(
      T &lh,
      const T &rh)
    noexcept(noexcept(value_of(lh) *= value_of(rh)))
    {
      value_of(lh) *= value_of(rh);
      return lh;
    }

    friend
    constexpr
    T&
    operator/=(
      T &lh,
      const T &rh)
    noexcept(noexcept(value_of(lh) /= value_of(rh)))
    {
      value_of(lh) /= value_of(rh);
      return lh;
    }

    [[nodiscard]]
    friend
    constexpr
    T
    operator+(
      T lh,
      const T &rh)
    {
      lh += rh;
      return lh;
    }

    [[nodiscard]]
    friend
    constexpr
    T
    operator-(
      T lh,
      const T &rh)
    {
      lh -= rh;
      return lh;
    }

    [[nodiscard]]
    friend
    constexpr
    T
    operator*(
      T lh,
      const T &rh)
    {
      lh *= rh;
      return lh;
    }

    [[nodiscard]]
    friend
    constexpr
    T
    operator/(
      T lh,
      const T &rh)
    {
      lh /= rh;
      return lh;
    }
  };
};


struct bitarithmetic
{
  template <typename T>
  class modifier
  {
  public:
    friend
    constexpr
    T&
    operator&=(
      T &lh,
      const T &rh)
    noexcept(noexcept(value_of(lh) &= value_of(rh)))
    {
      value_of(lh) &= value_of(rh);
      return lh;
    }

    friend
    constexpr
    T&
    operator|=(
      T &lh,
      const T &rh)
    noexcept(noexcept(value_of(lh) |= value_of(rh)))
    {
      value_of(lh) |= value_of(rh);
      return lh;
    }

    friend
    constexpr
    T&
    operator^=(
      T &lh,
      const T &rh)
    noexcept(noexcept(value_of(lh) ^= value_of(rh)))
    {
      value_of(lh) ^= value_of(rh);
      return lh;
    }

    template <typename C>
    friend
    constexpr
    T&
    operator<<=(
      T &lh,
      C c)
    noexcept(noexcept(value_of(lh) <<= c))
    {
      value_of(lh) <<= c;
      return lh;
    }

    template <typename C>
    friend
    constexpr
    T&
    operator>>=(
      T &lh,
      C c)
    noexcept(noexcept(value_of(lh) >>= c))
    {
      value_of(lh) >>= c;
      return lh;
    }

    [[nodiscard]]
    friend
    constexpr
    T
    operator~(
      const T &lh)
    {
      auto v = value_of(lh);
      v = ~v;
      return T(v);
    }

    [[nodiscard]]
    friend
    constexpr
    T
    operator&(
      T lh,
      const T &rh)
    {
      lh &= rh;
      return lh;
    }

    [[nodiscard]]
    friend
    constexpr
    T
    operator|(
      T lh,
      const T &rh)
    {
      lh |= rh;
      return lh;
    }

    [[nodiscard]]
    friend
    constexpr
    T
    operator^(
      T lh,
      const T &rh)
    {
      lh ^= rh;
      return lh;
    }

    template <typename C>
    [[nodiscard]]
    friend
    constexpr
    T
    operator<<(
      T lh,
      C c)
    {
      lh <<= c;
      return lh;
    }

    template <typename C>
    [[nodiscard]]
    friend
    constexpr
    T
    operator>>(
      T lh,
      C c)
    {
      lh >>= c;
      return lh;
    }
  };
};
template <typename I = void>
struct indexed
{
  template <typename>
  class modifier;
};


template <typename I> template <typename>
class indexed<I>::modifier {};

template <typename I>
template <typename T, typename Tag, typename ... M>
class indexed<I>::modifier<::strong::type<T, Tag, M...>>
{
  template <typename C, typename II>
  using at_type = decltype(std::declval<C>().at(std::declval<II>()));
  using type = ::strong::type<T, Tag, M...>;
public:
  [[nodiscard]]
  auto
  operator[](
    const I& i)
  const &
  noexcept(noexcept(std::declval<const T&>()[impl::access(i)]))
  -> decltype(std::declval<const T&>()[impl::access(i)])
  {
    auto& self = static_cast<const type&>(*this);
    return value_of(self)[impl::access(i)];
  }

  [[nodiscard]]
  auto
  operator[](
    const I& i)
  &
  noexcept(noexcept(std::declval<T&>()[impl::access(i)]))
  -> decltype(std::declval<T&>()[impl::access(i)])
  {
    auto& self = static_cast<type&>(*this);
    return value_of(self)[impl::access(i)];
  }

  [[nodiscard]]
  auto
  operator[](
    const I& i)
  &&
  noexcept(noexcept(std::declval<T&&>()[impl::access(i)]))
  -> decltype(std::declval<T&&>()[impl::access(i)])
  {
    auto& self = static_cast<type&>(*this);
    return value_of(std::move(self))[impl::access(i)];
  }

  template <typename TT = T>
  [[nodiscard]]
  auto
  at(
    const I& i)
  const &
  -> decltype(std::declval<const TT&>().at(impl::access(i)))
  {
    auto& self = static_cast<const type&>(*this);
    return value_of(self).at(impl::access(i));
  }

  template <typename TT = T>
  [[nodiscard]]
  auto
  at(
    const I& i)
  &
  -> decltype(at_access(std::declval<TT&>(), impl::access(i)))
  {
    auto& self = static_cast<type&>(*this);
    return value_of(self).at(impl::access(i));
  }

  template <typename TT = T>
  [[nodiscard]]
  auto
  at(
    const I& i)
  &&
  -> decltype(at_access(std::declval<TT&&>(), impl::access(i)))
  {
    auto& self = static_cast<type&>(*this);
    return value_of(std::move(self)).at(impl::access(i));
  }
};

template <> template <typename V>
class indexed<void>::modifier {};

template <>
template <typename T, typename Tag, typename ... Ms>
class indexed<void>::modifier<::strong::type<T, Tag, Ms...>>
{
  static_assert(!std::is_same_v<T, void>);
  static_assert(!std::is_same_v<T, const void>);

  using type = ::strong::type<T, Tag, Ms...>;
  using ref = T&;
  using cref = const T&;
  using rref = T&&;
public:
  template <typename I>
  [[nodiscard]]
  auto
  operator[](
    const I& i)
  const &
  noexcept(noexcept(std::declval<cref>()[impl::access(i)]))
  -> decltype(std::declval<cref>()[impl::access(i)])
  {
    const auto& self = static_cast<const type&>(*this);
    return value_of(self)[impl::access(i)];
  }

  template<typename I>
  [[nodiscard]]
  auto
  operator[](
    const I &i)
  &
  noexcept(noexcept(std::declval<ref>()[impl::access(i)]))
  -> decltype(std::declval<ref>()[impl::access(i)])
  {
    auto &self = static_cast<type &>(*this);
    return value_of(self)[impl::access(i)];
  }

  template<typename I>
  [[nodiscard]]
  auto
  operator[](
    const I &i)
  &&
  noexcept(noexcept(std::declval<rref>()[impl::access(i)]))
  -> decltype(std::declval<rref>()[impl::access(i)])
  {
    auto &self = static_cast<type &>(*this);
    return value_of(std::move(self))[impl::access(i)];
  }

  template<typename I, typename C = cref>
  [[nodiscard]]
  auto
  at(
    const I &i)
  const &
  -> decltype(std::declval<C>().at(impl::access(i)))
  {
    auto &self = static_cast<const type &>(*this);
    return value_of(self).at(impl::access(i));
  }

  template<typename I, typename R = ref>
  [[nodiscard]]
  auto
  at(
    const I &i)
  &
  -> decltype(std::declval<R>().at(impl::access(i)))
  {
    auto &self = static_cast<type &>(*this);
    return value_of(self).at(impl::access(i));
  }

  template<typename I, typename R = rref>
  [[nodiscard]]
  auto
  at(
    const I &i)
  &&
  -> decltype(std::declval<R>().at(impl::access(i)))
  {
    auto &self = static_cast<type &>(*this);
    return value_of(std::move(self)).at(impl::access(i));
  }
};

class iterator
{
public:
  template <typename I, typename category = typename std::iterator_traits<I>::iterator_category>
  class modifier
    : public pointer::modifier<I>
      , public equality::modifier<I>
      , public incrementable::modifier<I>
  {
  };

  template <typename I>
  class modifier<I, std::bidirectional_iterator_tag>
    : public modifier<I, std::forward_iterator_tag>
      , public decrementable::modifier<I>
  {
  };
  template <typename I>
  class modifier<I, std::random_access_iterator_tag>
    : public modifier<I, std::bidirectional_iterator_tag>
      , public affine_point<typename std::iterator_traits<I>::difference_type>::template modifier<I>
      , public indexed<>::modifier<I>
      , public ordered::modifier<I>
  {
  };
};

class range
{
public:
  template <typename R>
  class modifier;
};

template <std::ranges::range T, typename Tag, typename ... M>
class range::modifier<type<T, Tag, M...>>
{
  using type = ::strong::type<T, Tag, M...>;
  using r_iterator = decltype(std::declval<T&>().begin());
  using r_const_iterator = decltype(std::declval<const T&>().begin());
public:
  using iterator = ::strong::type<r_iterator, Tag, strong::iterator>;
  using const_iterator = ::strong::type<r_const_iterator, Tag, strong::iterator>;

  iterator
  begin()
  noexcept(noexcept(std::declval<T&>().begin()))
  {
    auto& self = static_cast<type&>(*this);
    return iterator{value_of(self).begin()};
  }

  iterator
  end()
  noexcept(noexcept(std::declval<T&>().end()))
  {
    auto& self = static_cast<type&>(*this);
    return iterator{value_of(self).end()};
  }

  const_iterator
  cbegin()
    const
  noexcept(noexcept(std::declval<const T&>().begin()))
  {
    auto& self = static_cast<const type&>(*this);
    return const_iterator{value_of(self).begin()};
  }

  const_iterator
  cend()
    const
  noexcept(noexcept(std::declval<const T&>().end()))
  {
    auto& self = static_cast<const type&>(*this);
    return const_iterator{value_of(self).end()};
  }

  const_iterator
  begin()
  const
  noexcept(noexcept(std::declval<const T&>().begin()))
  {
    auto& self = static_cast<const type&>(*this);
    return const_iterator{value_of(self).begin()};
  }

  const_iterator
  end()
  const
  noexcept(noexcept(std::declval<const T&>().end()))
  {
    auto& self = static_cast<const type&>(*this);
    return const_iterator{value_of(self).end()};
  }
};

namespace impl {
  template <typename ... Ts>
  constexpr bool hashable_func(::strong::hashable::modifier<Ts...>*) { return true; }

  constexpr bool hashable_func(...) { return false; }

  template <typename T>
  concept hashable = hashable_func(static_cast<T*>(nullptr));

  template <typename ... Ts>
  constexpr bool arithmetic_func(::strong::arithmetic::modifier<Ts...>*) { return true;}
  constexpr bool arithmetic_func(...) { return false;}

}
}

namespace std {
template <typename T, typename Tag, typename ... M>
requires(strong::impl::hashable<::strong::type<T, Tag, M...>>)
struct hash<::strong::type<T, Tag, M...>>
    : hash<T>
{
  using type = ::strong::type<T, Tag, M...>;
  decltype(auto)
  operator()(
    const type& t)
  const
  {
    return hash<T>::operator()(value_of(t));
  }
};


template <typename T, typename Tag, typename ... M>
struct is_arithmetic<::strong::type<T, Tag, M...>>
  : std::bool_constant<strong::impl::arithmetic_func(static_cast<strong::type<T, Tag, M...>*>(nullptr))> {};
#if 0
  : is_base_of<::strong::arithmetic::modifier<::strong::type<T, Tag, M...>>,
               ::strong::type<T, Tag, M...>>
{
};
#endif
template <typename T, typename Tag, typename ... M>
struct iterator_traits<::strong::type<T, Tag, M...>>
  : std::iterator_traits<T>
{
  using difference_type = typename std::iterator_traits<T>::difference_type;
  using value_type = typename std::iterator_traits<T>::value_type;
  using pointer = typename std::iterator_traits<T>::value_type;
  using reference = typename std::iterator_traits<T>::reference;
  using iterator_category = typename std::iterator_traits<T>::iterator_category;
};

}
#endif //SAFE_TYPE_SAFE_TYPE_HPP
