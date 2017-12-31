#if !defined( INCLUDED_FUNCTIONAL_H )
#define INCLUDED_FUNCTIONAL_H

#include <tuple>

namespace detail {
    template<class F>
    struct Fn;

    template<class R, class... Ts>
    struct Fn<R(Ts...)> {
        using result_type = R;

        template<int N>
        using get = typename std::tuple_element<N, std::tuple<Ts...>>::type;
    };
}

template<class Caller>
using get_result_type = typename detail::Fn<typename Caller::func>::result_type;

template<class Caller, int N>
using get_argument = typename detail::Fn<typename Caller::func>::template get<N>;

template<class Object, class F>
class MemberN;

template<class Object, class R, class... Ts>
class MemberN<Object, R(Ts...)> {
public:
    template<R(Object::*f)(Ts...)>
    class instance {
    public:
        using func = R(Object &, Ts...);

        static R call(Object &object, Ts... args) {
            return (object.*f)(args...);
        }
    };
};

template<class Object, class F>
class ConstMemberN;

template<class Object, class R, class... Ts>
class ConstMemberN<Object, R(Ts...)> {
public:
    template<R(Object::*f)(Ts...) const>
    class instance {
    public:
        using func = R(const Object &, Ts...);

        static R call(const Object &object, Ts... args) {
            return (object.*f)(args...);
        }
    };
};

template<class F>
class FunctionN;

template<class R, class... Ts>
class FunctionN<R(Ts...)> {
public:
    template<R(*f)(Ts...)>
    class instance {
    public:
        using func = R(Ts...);

        static R call(Ts... args) {
            return (f)(args...);
        }
    };
};

template<class Caller, class F>
class CallerShiftFirst;

template<class Caller, class R, class FirstArgument, class... Ts>
class CallerShiftFirst<Caller, R(FirstArgument, Ts...)> {
public:
    using func = R(FirstArgument, Ts...);

    static R call(FirstArgument, Ts... args) {
        return Caller::call(args...);
    }
};

template<class Functor, class F>
class FunctorNInvoke;

namespace detail {
    template<int ...>
    struct seq {
    };

    template<int N, int... S>
    struct gens : gens<N - 1, N - 1, S...> {
    };

    template<int... S>
    struct gens<0, S...> {
        using type = seq<S...>;
    };

    template<int N>
    using seq_new = typename gens<N>::type;
}

template<class Functor, class R, class... Ts>
class FunctorNInvoke<Functor, R(Ts...)> {
    std::tuple<Ts...> args;

    template<class T>
    struct caller;

    template<int ...I>
    struct caller<detail::seq<I...>> {
        static inline R call(FunctorNInvoke<Functor, R(Ts...)> *self, Functor functor) {
            (void) self;
            return functor(std::get<I>(self->args)...);
        }
    };

public:
    FunctorNInvoke(Ts... args) : args(args...) {
    }

    inline R operator()(Functor functor) {
        return caller<detail::seq_new<sizeof...(Ts)>>::call(this, functor);
    }
};

template<class Functor>
using FunctorInvoke = FunctorNInvoke<Functor, typename Functor::func>;

template<class Object, class R, R(Object::*member)()>
using Member = typename MemberN<Object, R()>::template instance<member>;

template<class Object, class R, R(Object::*member)() const>
using ConstMember = typename ConstMemberN<Object, R()>::template instance<member>;

template<class Object, class A1, class R, R(Object::*member)(A1)>
using Member1 = typename MemberN<Object, R(A1)>::template instance<member>;

template<class Object, class A1, class R, R(Object::*member)(A1) const>
using ConstMember1 = typename ConstMemberN<Object, R(A1)>::template instance<member>;

template<class Object, class A1, class A2, class R, R(Object::*member)(A1, A2)>
using Member2 = typename MemberN<Object, R(A1, A2)>::template instance<member>;

template<class Object, class A1, class A2, class R, R(Object::*member)(A1, A2) const>
using ConstMember2 = typename ConstMemberN<Object, R(A1, A2)>::template instance<member>;

template<class Object, class A1, class A2, class A3, class R, R(Object::*member)(A1, A2, A3)>
using Member3 = typename MemberN<Object, R(A1, A2, A3)>::template instance<member>;

template<class Object, class A1, class A2, class A3, class R, R(Object::*member)(A1, A2, A3) const>
using ConstMember3 = typename ConstMemberN<Object, R(A1, A2, A3)>::template instance<member>;

template<class R, R(*func)()>
using Function0 = typename FunctionN<R()>::template instance<func>;

template<class A1, class R, R(*func)(A1)>
using Function1 = typename FunctionN<R(A1)>::template instance<func>;

template<class A1, class A2, class R, R(*func)(A1, A2)>
using Function2 = typename FunctionN<R(A1, A2)>::template instance<func>;

template<class A1, class A2, class A3, class R, R(*func)(A1, A2, A3)>
using Function3 = typename FunctionN<R(A1, A2, A3)>::template instance<func>;

template<class A1, class A2, class A3, class A4, class R, R(*func)(A1, A2, A3, A4)>
using Function4 = typename FunctionN<R(A1, A2, A3, A4)>::template instance<func>;

template<class Caller, class FirstArgument = void *>
using Caller0To1 = CallerShiftFirst<Caller, get_result_type<Caller>(
        FirstArgument
)>;

template<class Caller, class FirstArgument = void *>
using Caller1To2 = CallerShiftFirst<Caller, get_result_type<Caller>(
        FirstArgument,
        get_argument<Caller, 0>
)>;

template<class Caller, class FirstArgument = void *>
using Caller2To3 = CallerShiftFirst<Caller, get_result_type<Caller>(
        FirstArgument,
        get_argument<Caller, 0>,
        get_argument<Caller, 1>
)>;

template<class Caller, class FirstArgument = void *>
using Caller3To4 = CallerShiftFirst<Caller, get_result_type<Caller>(
        FirstArgument,
        get_argument<Caller, 0>,
        get_argument<Caller, 1>,
        get_argument<Caller, 2>
)>;

#endif
