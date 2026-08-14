#pragma once

#include <wayfire/config/config-manager.hpp>

#include <optional>
#include <string>
#include <vector>

namespace wf
{
namespace config
{
namespace edit
{
/**
 * A single write or removal to be applied by patch_file().
 *
 * A set @value writes or replaces the key. An unset @value removes the key, and drops the section too if that
 * leaves it with no other assignments.
 *
 * The wayfire.ini dialect: unescaped '#' starts a comment, unescaped trailing '\' continues onto the next
 * line. Comments, blank lines, key order, and whitespace around '=' survive unchanged.
 */
struct patch
{
    std::string section;
    std::string key;
    std::optional<std::string> value;
};

/**
 * Read @path, apply @patches, and write the result back to @path.
 *
 * Empty @patches leave the file untouched. Missing files are treated as empty sources: writes create the
 * file, resets are ignored.
 *
 * @manager acts as an optional validation authority. For each write patch, the corresponding option in
 * @manager is cloned and its set_value_str is called with the patch's value. Patches that fail validation, or
 * that refer to a section/key not present in @manager, are silently skipped. Accepted patches carry the
 * option's canonical form, not the caller's raw string. Resets are not validated.
 *
 * @param path The wayfire.ini file to edit.
 * @param patches The operations to apply, in order.
 * @param manager Optional validation authority. Not modified.
 *
 * @return True on success, false on IO or permission error.
 */
bool patch_file(const std::string& path,
    const std::vector<patch>& patches,
    const config_manager_t *manager = nullptr);
}
}
}
