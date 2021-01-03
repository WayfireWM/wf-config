#pragma once

#include <wayfire/config/compound-option.hpp>
#include <wayfire/config/section.hpp>
#include <libxml/tree.h>
#include <stdint.h>

struct wf::config::option_base_t::impl
{
    std::string name;
    std::vector<updated_callback_t*> updated_handlers;

    // Number of times the option has been locked
    int32_t lock_count = 0;

    // Associated XML node
    xmlNode *xml;

    // Is option in config file?
    bool option_in_config_file = false;
};

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
