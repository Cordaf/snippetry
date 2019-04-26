
#include <iostream>
#include <functional>
#include <vector>
#include <unordered_map>

#define TEXTIFY(A) #A

#define INVOKABLE_FUNCTION(funcname, funcbody)      \
struct funcname : anchored_base<funcname> {         \
inline static const char* name() {                  \
    return TEXTIFY(funcname);                       \
}                                                   \
static void call() funcbody                         \
private:                                            \
    inline void init() {                            \
        anchored_base<funcname>::init;              \
    }                                               \
};                                                  \


template<typename TVal>
struct static_storage {
    //static decltype(auto) get() {
    //    static TVal val;
    //    return (val);
    //}

    template<typename... TArgs>
    static decltype(auto) get(TArgs... args) {
        static TVal val { args... };
        return (val);
    }
};

struct anchored {};

template<typename, typename...>
struct anchored_function;

template<typename TOut, typename... TIn>
struct anchored_function<TOut(TIn...)> : anchored {
    using func_type = std::function<TOut(TIn...)>;
    anchored_function(std::string name, func_type f) {
        static_storage<std::unordered_map<std::string, func_type>>::get().insert(std::make_pair(name, f));
    }
};

template<typename Tag, typename TOut, typename... TIn>
decltype(auto) make_anchored(std::string name, std::function<TOut(TIn...)> f) {
    return std::move(static_storage<anchored_function<TOut(TIn...)>>::get(name, f));
}

template<typename Func>
struct anchored_base {
    static anchored init;
};

template<typename Func>
anchored anchored_base<Func>::init = make_anchored<Func>(Func::name(), std::function<decltype(Func::call)>(&Func::call));

template<typename TSignature, typename... TIn>
decltype(auto) invoke(std::string name, TIn... args) {
    using func_type = std::function<TSignature>;
    auto& reg = static_storage<std::unordered_map<std::string, func_type>>::get();
    if (reg.find(name) == reg.end())
        throw std::runtime_error("function not found");
    return reg[name](args...);
}


struct foo : anchored_base<foo> {
    inline static const char* name() { return "foo"; };

    static void call() {
        std::cout << "bang!" << std::endl;
    }

private:
    inline void init() { anchored_base<foo>::init; }
};

struct bar : anchored_base<bar> {
    inline static const char* name() { return "bar"; };

    static void call(int i) {
        std::cout << __FUNCTION__ << " " << i << std::endl;
    }

private:
    inline void init() { anchored_base<bar>::init; }
};

INVOKABLE_FUNCTION(baz, { std::cout << "baz!" << std::endl; })

int main() {
    invoke<void(int)>("bar", 42);
    //invoke<void(void)>("bar");

    invoke<void(void)>("foo");
    invoke<void(void)>("baz");
    //invoke<void(int)>("foo", 42);
}
