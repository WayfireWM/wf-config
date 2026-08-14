#pragma once

#include <wayfire/config/edit.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace wf
{
namespace config
{
namespace edit
{
/**
 * Apply patches to a wayfire.ini text, touching only the named keys.
 *
 * Comments, blank lines, key order, whitespace around '=', and any unrelated lines survive unchanged. A new
 * key is appended to its section. A new section is appended to the file. Patches apply in order and each one
 * sees the result of the previous.
 *
 */
std::string apply(std::string_view source,
    const std::vector<patch>& patches);
}
}
}
