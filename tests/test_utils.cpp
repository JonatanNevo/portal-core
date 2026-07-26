//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <catch2/catch_all.hpp>

import portal.core;
import std;

SCENARIO("A variant class works with the portal::match", "[unit][core][util]")
{
    GIVEN("A variant of various types")
    {
        std::variant<int, float, double, std::string> some_variant;

        WHEN("it holds an int")
        {
            some_variant = 42;

            THEN("match dispatches to the int handler")
            {
                std::string result;
                portal::match(
                    some_variant,
                    [&](int v) { result = "int:" + std::to_string(v); },
                    [&](float) { result = "float"; },
                    [&](double) { result = "double"; },
                    [&](const std::string&) { result = "string"; });

                REQUIRE(result == "int:42");
            }
        }

        WHEN("it holds a float")
        {
            some_variant = 1.5f;

            THEN("match dispatches to the float handler")
            {
                std::string result;
                portal::match(
                    some_variant,
                    [&](int) { result = "int"; },
                    [&](float v) { result = "float:" + std::to_string(v); },
                    [&](double) { result = "double"; },
                    [&](const std::string&) { result = "string"; });

                REQUIRE(result.starts_with("float:"));
            }
        }

        WHEN("it holds a double")
        {
            some_variant = 2.5;

            THEN("match dispatches to the double handler")
            {
                std::string result;
                portal::match(
                    some_variant,
                    [&](int) { result = "int"; },
                    [&](float) { result = "float"; },
                    [&](double v) { result = "double:" + std::to_string(v); },
                    [&](const std::string&) { result = "string"; });

                REQUIRE(result.starts_with("double:"));
            }
        }

        WHEN("it holds a string")
        {
            some_variant = std::string{"hello"};

            THEN("match dispatches to the string handler")
            {
                std::string result;
                portal::match(
                    some_variant,
                    [&](int) { result = "int"; },
                    [&](float) { result = "float"; },
                    [&](double) { result = "double"; },
                    [&](const std::string& v) { result = "string:" + v; });

                REQUIRE(result == "string:hello");
            }
        }
    }

    GIVEN("A generic catch-all matcher")
    {
        std::variant<int, float, double, std::string> some_variant = 7;

        WHEN("only a generic handler is provided")
        {
            THEN("it matches regardless of the held type")
            {
                bool called = false;
                portal::match(some_variant, [&](auto&&) { called = true; });

                REQUIRE(called);
            }
        }
    }

    GIVEN("A const variant")
    {
        constexpr std::variant<int, std::string> some_variant = std::string{"const"};

        WHEN("match is called on it")
        {
            THEN("it still dispatches correctly")
            {
                std::string result;
                portal::match(
                    some_variant,
                    [&](int) { result = "int"; },
                    [&](const std::string& v) { result = "string:" + v; });

                REQUIRE(result == "string:const");
            }
        }
    }
}