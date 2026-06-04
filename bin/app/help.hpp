#pragma once

/// @file
/// @brief Contextual help text rendering.

#include <ostream>

namespace cli {

/// Print the top-level usage/overview (commands, globals, env vars).
void print_root_help(std::ostream& os);

}  // namespace cli
