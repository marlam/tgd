#include <cstdio>
#include <algorithm>

#include "core/array.hpp"
#include "core/foreach.hpp"
#include "core/operators.hpp"
#include "core/io.hpp"

void check_failed(const char* expr, const char* file, unsigned int line)
{
    fprintf(stderr, "%s:%u: check '%s' failed\n", file, line, expr);
    exit(1);
}

#define EXPECT(E) (static_cast<bool>(E) ? void(0) : check_failed(#E, __FILE__, __LINE__))

int main(void)
{
    // Create test arrays
    TAD::Array<uint8_t> a({17, 19}, 3);
    TAD::Array<uint8_t> b({17, 19}, 3);
    TAD::forEachElementInplace(a, [] (uint8_t* element) { element[0] = 1; element[1] = 2; element[2] = 3; });
    TAD::forEachElementInplace(b, [] (uint8_t* element) { element[0] = 4; element[1] = 5; element[2] = 6; });

    // Some basic computations
    TAD::Array<uint8_t> r = a + b;
    TAD::forEachElementInplace(r, [] (const uint8_t* element) { EXPECT(element[0] == 5); EXPECT(element[1] == 7); EXPECT(element[2] == 9); });
    r += static_cast<uint8_t>(10);
    TAD::forEachElementInplace(r, [] (const uint8_t* element) { EXPECT(element[0] == 15); EXPECT(element[1] == 17); EXPECT(element[2] == 19); });
    r -= b;
    TAD::forEachElementInplace(r, [] (const uint8_t* element) { EXPECT(element[0] == 11); EXPECT(element[1] == 12); EXPECT(element[2] == 13); });

    // Different ways to use the forEachComponent function: lambdas, function pointers, functors
    TAD::Array<uint8_t> squared_diff_lambda = TAD::forEachComponent(a, b, 
            [] (uint8_t val_a, uint8_t val_b) -> uint8_t { return (val_a - val_b) * (val_a - val_b); } );
    auto sqd = [] (uint8_t val_a, uint8_t val_b) -> uint8_t { return (val_a - val_b) * (val_a - val_b); };
    uint8_t (*sqdptr)(uint8_t, uint8_t) = sqd;
    TAD::Array<uint8_t> squared_diff_func_pointer = TAD::forEachComponent(a, b, sqdptr);
    struct { uint8_t operator()(uint8_t val_a, uint8_t val_b) { return (val_a - val_b) * (val_a - val_b); } } sqdfunctor;
    TAD::Array<uint8_t> squared_diff_functor = TAD::forEachComponent(a, b, sqdfunctor);
    TAD::forEachComponent(squared_diff_lambda, squared_diff_func_pointer, [] (uint8_t v0, uint8_t v1) -> uint8_t { EXPECT(v0 == v1); return 0; });
    TAD::forEachComponent(squared_diff_lambda, squared_diff_functor, [] (uint8_t v0, uint8_t v1) -> uint8_t { EXPECT(v0 == v1); return 0; });

    // Type conversion
    TAD::Array<float> af = convert(a, TAD::float32);
    r = convert(af, TAD::uint8);
    TAD::forEachComponent(a, r, [] (uint8_t v0, uint8_t v1) -> uint8_t { EXPECT(v0 == v1); return 0; });

    // Iterators and the STL
    r = a.deepCopy();
    std::for_each(r.componentBegin(), r.componentEnd(), [](uint8_t& v) { v = 42; });
    TAD::forEachComponent(r, [] (uint8_t v) -> uint8_t { EXPECT(v == 42); return 0; });
    r = a.deepCopy();
    std::for_each(r.elementBegin(), r.elementEnd(), [](uint8_t* v) { v[0] = 0; v[1] = 1; v[2] = 2; });
    TAD::forEachElementInplace(r, [] (const uint8_t* element) { EXPECT(element[0] == 0); EXPECT(element[1] == 1); EXPECT(element[2] == 2); });
    r = a.deepCopy();
    std::sort(r.componentBegin(), r.componentEnd());
    for (size_t e = 0; e < r.elementCount(); e++) {
        for (size_t c = 0; c < r.componentCount(); c++) {
            size_t i = e * r.componentCount() + c;
            EXPECT(r.get<uint8_t>(e, c) == i / r.elementCount() + 1);
        }
    }

    return 0;
}
