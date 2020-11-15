#pragma once
/**
 * This file contains definitions related to parsing XML files and turning them
 * into config options and sections.
 */

#include <libxml/parser.h>
#include <libxml/tree.h>

#include <wayfire/config/option.hpp>
#include <wayfire/config/section.hpp>

namespace wf
{
namespace config
{
namespace xml
{
/**
 * Create a new option from the given data in the xmlNode.
 * Errors are printed to the log (see wayfire/util/log.hpp).
 *
 * If the operation is successful, the xmlNodePtr should not be freed, because
 * an internal reference will be taken.
 *
 * @param node The xmlNode which corresponds to the option.
 *  The attributes used are name and type, as well as the <default> child.
 *
 * @return The generated option, if the xmlNode contained a valid option, and
 *  nullptr otherwise. The dynamic type of the option is
 *  wf::config::option_t<T>, depending on the type attribute of the xmlNode.
 */
std::shared_ptr<wf::config::option_base_t> create_option_from_xml_node(
    xmlNodePtr node);

/**
 * Create a new section from the given xmlNode and add each successfully parsed
 * child as an option in the section.
 * Errors are printed to the log (see wayfire/util/log.hpp).
 *
 * If the operation is successful, the xmlNodePtr should not be freed, because
 * an internal reference will be taken.
 *
 * @param node the xmlNode which corresponds to the section node. The only
 *  attribute of @node used is name, and it determines the name of the generated
 *  section.
 *
 * @return nullptr if section name is missing from the xmlNode, and the
 *  generated config section otherwise.
 */
std::shared_ptr<wf::config::section_t> create_section_from_xml_node(xmlNodePtr node);

/**
 * Get the XML node which was used to create @option with
 * create_option_from_xml_node.
 *
 * @return The xmlNodePtr or NULL if the option wasn't created from an xml node.
 */
xmlNodePtr get_option_xml_node(
    std::shared_ptr<wf::config::option_base_t> option);

/**
 * Get the XML node which was used to create @section with
 * create_section_from_xml_node.
 *
 * @return The xmlNodePtr or NULL if the section wasn't created from an xml
 *   node.
 */
xmlNodePtr get_section_xml_node(std::shared_ptr<wf::config::section_t> section);
}
}
}
