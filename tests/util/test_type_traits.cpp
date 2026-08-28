//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <catch2/catch_all.hpp>

import portal.core;
import std;

namespace
{
template<typename T>
struct is_int : std::false_type
{};

template<>
struct is_int<int> : std::true_type
{};

template<typename T>
struct is_pointer_pred : std::bool_constant<std::is_pointer_v<T>>
{};
} // namespace

SCENARIO("is_specialization_of detects template specializations", "[unit][core][util][type_traits]")
{
    GIVEN("a std::vector<int> type")
    {
        THEN("it is a specialization of std::vector")
        {
            STATIC_REQUIRE(portal::is_specialization_of_v<std::vector<int>, std::vector>);
        }

        THEN("it is not a specialization of std::optional")
        {
            STATIC_REQUIRE_FALSE(portal::is_specialization_of_v<std::vector<int>, std::optional>);
        }
    }

    GIVEN("a non-template type")
    {
        THEN("it is not a specialization of std::vector")
        {
            STATIC_REQUIRE_FALSE(portal::is_specialization_of_v<int, std::vector>);
        }
    }

    GIVEN("a multi-argument template")
    {
        THEN("it correctly identifies std::map specializations")
        {
            STATIC_REQUIRE(portal::is_specialization_of_v<std::map<int, float>, std::map>);
            STATIC_REQUIRE_FALSE(portal::is_specialization_of_v<std::map<int, float>, std::pair>);
        }
    }
}

SCENARIO("concat_t joins type_lists together", "[unit][core][util][type_traits]")
{
    GIVEN("no type_lists")
    {
        THEN("the result is an empty type_list")
        {
            STATIC_REQUIRE(std::is_same_v<portal::concat_t<>, portal::type_list<>>);
        }
    }

    GIVEN("a single type_list")
    {
        THEN("the result is the same type_list")
        {
            STATIC_REQUIRE(
                    std::is_same_v<portal::concat_t<portal::type_list<int, float>>, portal::type_list<int, float>>
            );
        }
    }

    GIVEN("two non-empty type_lists")
    {
        THEN("the result contains all types in order")
        {
            STATIC_REQUIRE(
                    std::is_same_v<
                            portal::concat_t<portal::type_list<int, float>, portal::type_list<double>>,
                            portal::type_list<int, float, double>>
            );
        }
    }

    GIVEN("more than two type_lists")
    {
        THEN("the result concatenates all of them in order")
        {
            STATIC_REQUIRE(
                    std::is_same_v<
                            portal::concat_t<
                                    portal::type_list<int>,
                                    portal::type_list<float, double>,
                                    portal::type_list<char>>,
                            portal::type_list<int, float, double, char>>
            );
        }
    }

    GIVEN("empty type_lists mixed with non-empty ones")
    {
        THEN("the empty lists contribute nothing")
        {
            STATIC_REQUIRE(
                    std::is_same_v<
                            portal::concat_t<portal::type_list<>, portal::type_list<int>, portal::type_list<>>,
                            portal::type_list<int>>
            );
        }
    }
}

SCENARIO("filter_t selects types matching a predicate", "[unit][core][util][type_traits]")
{
    GIVEN("a predicate that matches int")
    {
        THEN("only int types are kept, in order")
        {
            STATIC_REQUIRE(
                    std::is_same_v<portal::filter_t<is_int, float, int, double, int>, portal::type_list<int, int>>
            );
        }
    }

    GIVEN("no matching types")
    {
        THEN("the result is an empty type_list")
        {
            STATIC_REQUIRE(std::is_same_v<portal::filter_t<is_int, float, double, char>, portal::type_list<>>);
        }
    }

    GIVEN("no input types at all")
    {
        THEN("the result is an empty type_list")
        {
            STATIC_REQUIRE(std::is_same_v<portal::filter_t<is_int>, portal::type_list<>>);
        }
    }

    GIVEN("all types matching the predicate")
    {
        THEN("every type is kept in order")
        {
            STATIC_REQUIRE(std::is_same_v<portal::filter_t<is_int, int, int, int>, portal::type_list<int, int, int>>);
        }
    }

    GIVEN("a pointer predicate over mixed types")
    {
        THEN("only pointer types are kept")
        {
            STATIC_REQUIRE(
                    std::is_same_v<
                            portal::filter_t<is_pointer_pred, int, int*, float, double*>,
                            portal::type_list<int*, double*>>
            );
        }
    }
}
