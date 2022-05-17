#pragma once

#include "clamp.h"
#include "opt.h"
#include "ordr.h"

namespace Karm::Base {

template <typename Next>
struct Iter {
    Next next;
    using Item = decltype(next());

    constexpr Iter(Next next) : next(next) {}

    constexpr auto map(auto f) {
        return Iter{[=]() mutable {
            auto v = next();

            if (!next) {
                return NONE;
            }

            return f(*v);
        }};
    }

    constexpr auto filter(auto f) {
        return Iter{[=]() mutable {
            while (true) {
                auto v = next();
                if (!next) {
                    return NONE;
                }

                if (f(*v)) {
                    return *v;
                }
            }
        }};
    }

    constexpr auto reduce(auto init, auto f) {
        auto result = init;
        forEach([&](auto v) {
            result = f(result, v);
        });
        return result;
    }

    constexpr auto forEach(auto f) {
        for (auto item = next(); item; item = next()) {
            f(*item);
        }
    }

    constexpr auto find(auto f) {
        for (auto item = next(); item; item = next()) {
            if (f(*item)) {
                return *item;
            }
        }

        return NONE;
    }

    constexpr size_t len() {
        size_t result = 0;
        forEach([&](auto) {
            result++;
        });
        return result;
    }

    constexpr auto sum() {
        return reduce(0, [](auto &a, auto &b) {
            return a + b;
        });
    }

    constexpr bool any() {
        return next() != NONE;
    }

    constexpr auto first() {
        return next();
    }

    constexpr auto first(auto f) {
        for (auto item = next(); item; item = next()) {
            if (f(*item)) {
                return item;
            }
        }

        return NONE;
    }

    constexpr auto last() {
        Item result = NONE;
        for (auto item = next(); item; item = next()) {
            result = item;
        }
        return result;
    }

    constexpr auto last(auto f) {
        Item result = NONE;
        for (auto item = next(); item; item = next()) {
            if (f(*item)) {
                result = item;
            }
        }
        return result;
    }

    constexpr auto skip(size_t n) {
        for (size_t i = 0; i < n; i++) {
            next();
        }
        return *this;
    }

    constexpr auto cycle(size_t n) {
        return Iter{[start = *this, curr = *this, i = 0, n]() mutable {
            auto v = curr.next();

            if (!v && i < n) {
                curr = start;
                i++;
                v = curr.next();
            }

            return v;
        }};
    }

    constexpr auto take(size_t n) {
        return Iter{[=]() mutable {
            if (n == 0) {
                return NONE;
            }

            auto v = next();
            n--;
            return v;
        }};
    }

    constexpr bool any(auto f) {
        return find(f) != NONE;
    }

    constexpr bool all(auto f) {
        for (auto item = next(); item; item = next()) {
            if (!f(*item)) {
                return false;
            }
        }

        return true;
    }

    constexpr auto min() {
        Item result = NONE;

        for (auto item = next(); item; item = next()) {
            if (!result || Op::lt(*item, *result)) {
                result = item;
            }
        }

        return result;
    }

    constexpr auto max() {
        Item result = NONE;

        for (auto item = next(); item; item = next()) {
            if (!result || Op::gt(*item, *result)) {
                result = item;
            }
        }

        return result;
    }

    constexpr auto avg() -> Item {
        Item result = NONE;
        size_t count = 0;

        for (auto item = next(); item; item = next()) {
            if (!result) {
                result = item;
            } else {
                result = *result + *item;
            }
            count++;
        }

        if (count == 0) {
            return NONE;
        }

        return *result / count;
    }

    struct _It {
        Item curr;
        Iter iter;

        constexpr auto &operator*() {
            return *curr;
        }

        constexpr auto operator++() {
            curr = iter.next();
            return *this;
        }

        constexpr bool operator!=(None) {
            return curr != NONE;
        }
    };

    constexpr _It begin() {
        return _It{next(), *this};
    }

    constexpr None end() {
        return NONE;
    }

    template <typename C>
    constexpr auto collect(C &c) {
        forEach([&](auto v) {
            c.push(v);
        });
    }

    template <typename C>
    constexpr auto collect(C &c, auto f) {
        forEach([&](auto v) {
            c.push(f(v));
        });
    }

    template <typename C>
    constexpr auto collect() {
        C c;
        collect(c);
        return c;
    }

    template <typename C>
    constexpr auto collect(auto f) {
        C c;
        collect(c, f);
        return c;
    }
};

template <typename T>
constexpr auto single(T value) {
    return Iter<None>{[value, end = false]() mutable {
        if (end) {
            return NONE;
        }

        end = true;
        return value;
    }};
}

template <typename T>
constexpr auto repeat(T value, size_t count) {
    return Iter{[value, count]() mutable -> Opt<T> {
        if (count == 0) {
            return NONE;
        }

        count--;
        return value;
    }};
}

template <typename T>
constexpr auto range(T end) {
    return Iter{[value = static_cast<T>(0), end]() mutable -> Opt<T> {
        if (value >= end) {
            return NONE;
        }

        return value++;
    }};
}

template <typename T>
constexpr auto range(T start, T end) {
    return Iter{[value = start, end]() mutable -> Opt<T> {
        if (value >= end) {
            return NONE;
        }

        auto result = value;
        value++;
        return result;
    }};
}

template <typename T>
constexpr auto range(T start, T end, T step) {
    return Iter{[value = start, end, step]() mutable -> Opt<T> {
        if (value >= end) {
            return NONE;
        }

        auto result = value;
        value += step;
        return result;
    }};
}

} // namespace Karm::Base
