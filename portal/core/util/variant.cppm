//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

export module portal.core.util:variant;

import std;

export namespace portal
{
namespace details
{
    template <class... Ts>
    struct visitor : Ts...
    {
        using Ts::operator()...;
    };

    template <class... Ts>
    visitor(Ts...) -> visitor<Ts...>;
}

template <typename Variant, class... Matchers>
void match(Variant&& variant, Matchers&&... matchers)
{
    std::visit(details::visitor{std::forward<Matchers>(matchers)...}, std::forward<Variant>(variant));
}
}
