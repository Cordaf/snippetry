
#include <iostream>
#include <functional>
#include <vector>
#include <unordered_map>
#include <type_traits>

namespace misc {

struct anchor {};

namespace detail {
    template<typename TVal>
    struct static_storage {
        template<typename... TArgs>
        static decltype(auto) get(TArgs... args) {
            static TVal val{ args... };
            return (val);
        }
    };

    template<typename> struct deduce_type;

    template<typename TOut, typename TClass, typename... TIn>
    struct deduce_type<TOut(TClass::*)(TIn...) const> {
        using type = std::function<TOut(TIn...) >;
    };

    template<typename, typename...> struct anchored_function;

    template<typename TOut, typename... TIn>
    struct anchored_function<TOut(TIn...)> : anchor {
        using func_type = std::function<TOut(TIn...)>;
        anchored_function(std::string name, func_type f) {
            auto& container = static_storage<std::unordered_map<std::string, func_type>>::get();
            container.insert(std::make_pair(name, f));
        }
    };
}

template <typename TFunc>
auto make_function(const TFunc& fn) {
    return typename detail::deduce_type<decltype(&TFunc::operator())>::type(fn);
}

template<typename Tag, typename TOut, typename... TIn>
decltype(auto) make_anchored(std::string name, std::function<TOut(TIn...)> fn) {
    return (detail::anchored_function<TOut(TIn...)> { name, fn });
}

template<typename Func>
struct anchored_base {
    static anchor init;

private:
    static decltype(auto) do_init() {
        return make_anchored<Func>(Func::name(), make_function(Func::function_body()));
    }
};

template<typename Func> anchor anchored_base<Func>::init = anchored_base<Func>::do_init();

template<typename TSignature, typename... TIn>
decltype(auto) invoke(std::string name, TIn... args) {
    using func_type = std::function<TSignature>;
    auto& reg = detail::static_storage<std::unordered_map<std::string, func_type>>::get();
    if (reg.find(name) == reg.end())
        throw std::runtime_error("function not found");
    return reg[name](args...);
}

#define INVOKABLE_FUNCTION_(funcname, funcbody)     \
class funcname : misc::anchored_base<funcname> {    \
    using base_type = misc::anchored_base<funcname>;\
    inline void init() { base_type::init; }         \
    friend base_type;                               \
    inline static const char* name() noexcept {     \
        return #funcname; };                        \
    static decltype(auto) function_body() noexcept {\
        return funcbody;                            \
    }                                               \
};                                                  \

}//misc

class bar : misc::anchored_base<bar> {
    using base_type = misc::anchored_base<bar>;
    friend base_type;
    inline void init() { base_type::init; }

    inline static const char* name() noexcept { return "bar"; };

    static decltype(auto) function_body() noexcept {
        return [](int i) { std::cout << __FUNCTION__ << " " << i << std::endl; };
    }
};

INVOKABLE_FUNCTION_(foo, []() { std::cout << __FUNCTION__ << std::endl; })
INVOKABLE_FUNCTION_(baz, [](int i) { std::cout << __FUNCTION__ << " " << i << std::endl; })

int main() {
    misc::invoke<void(void)>("foo");
    misc::invoke<void(int)>("bar", 42);
    misc::invoke<void(int)>("baz", 43);

    //invoke<void(void)>("bar");
    //invoke<void(int)>("foo", 42);
}
