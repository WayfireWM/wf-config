#pragma once

#include <wayfire/config/compound-option.hpp>
#include <wayfire/config/section.hpp>
#include <wayfire/nonstd/safe-list.hpp>
#include <libxml/tree.h>
#include <stdint.h>

namespace wf
{
namespace config
{
/**
 * Update the value of a compound option option by reading options from the section.
 * The format is as described in the compound option constructor docstring.
 *
 * Note: options which have been created from XML are ignored, and only
 * options which have been created from parsing a string/file with wf-config
 * are taken into account.
 */
void update_compound_from_section(compound_option_t& option,
    const std::shared_ptr<section_t>& section);
}
}

struct wf::config::option_base_t::impl
{
    std::string name;
    wf::safe_list_t<updated_callback_t*> updated_handlers;

    // Number of times the option has been locked
    int32_t lock_count = 0;

    // Associated XML node
    xmlNode *xml = nullptr;

    // Is option in config file?
    bool option_in_config_file = false;

    // Is option part of a successfully parsed compound option?
    bool is_part_compound = false;
    // Does this option match a compound option in part at least?
    bool could_be_compound = false;
};
