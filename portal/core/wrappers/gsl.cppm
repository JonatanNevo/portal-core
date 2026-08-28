//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

module;

#include <gsl/gsl>

export module gsl;

// NOLINTBEGIN
export namespace gsl
{
using gsl::owner;
using gsl::not_null;
using gsl::make_not_null;
using gsl::swap;

using gsl::strict_not_null;
using gsl::make_strict_not_null;

using gsl::final_action;
using gsl::finally;
using gsl::narrow_cast;

using gsl::narrowing_error;
using gsl::narrow;
}
// NOLINTEND
