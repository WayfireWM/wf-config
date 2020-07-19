#pragma once

#include <wayfire/config/config-manager.hpp>

namespace wf
{
namespace config
{
/**
 * Parse a multi-line string as a configuration file.
 * The string consists of multiple sections of the following format:
 *
 * [section_name]
 * option1 = value1
 * option2 = value2
 * ...
 *
 * Blank lines and whitespace characters around the '=' sign are ignored, as
 * well as whitespaces at the beginning or at the end of each line.
 *
 * When a line contains a '#', the line from this point on is considered a
 * comment except when it is immediately preceded by a '\'.
 *
 * When a line ends in '\', it automatically joined with the next line, except
 * if it isn't escaped with another '\'.
 *
 * Each valid parsed option is used to set the value of the corresponding option
 * in @manager. Each line which contains errors is reported on the log and then
 * ignored.
 *
 * @param manager The config manager to update.
 * @param source The multi-line string representing the source
 * @param source_name The name to be used when reporting errors to the log
 */
void load_configuration_options_from_string(config_manager_t& manager,
    const std::string& source, const std::string& source_name = "");

/**
 * Create a string which conttains all the sections and the options in the given
 * configuration manager. The format is the same one as the one described in
 * load_configuration_options_from_string()
 *
 * @return The string representation of the config manager.
 */
std::string save_configuration_options_to_string(
    const config_manager_t& manager);

/**
 * Load the options from the given config file.
 *
 * This is roughly equivalent to reading the file to a string, and then calling
 * load_configuration_options_from_string(), but this function also tries to get
 * a shared lock on the config file, and does not do anything if another process
 * already holds an exclusive lock.
 *
 * @param manager The config manager to update.
 * @param file The config file to use.
 *
 * @return True if the config file was reloaded, false if file could not be
 * opened or a lock could not be acquired.
 */
bool load_configuration_options_from_file(config_manager_t& manager,
    const std::string& file);

/**
 * Writes the options in the given configuration to the given file.
 * It is roughly equivalent to calling serialize_configuration_manager() and
 * then replacing the file contents with the resulting string, but this function
 * waits until it can get an exclusive lock on the config file.
 */
void save_configuration_to_file(const config_manager_t& manager,
    const std::string& file);

/**
 * Build a configuration for the given program from the files on the filesystem.
 *
 * The following steps are performed:
 * 1. Each of the XML files in each of @xmldirs are read, and options there
 *   are used to build a configuration. Note that the XML nodes which are
 *   allocated will not be freed.
 * 2. The @sysconf file is used to overwrite default values from XML files.
 * 3. The @userconf file is used to determine the actual values of options.
 *
 * If any of the steps results in an error, the error will be reported to the
 * command line and the process will continue.
 */
config_manager_t build_configuration(const std::vector<std::string>& xmldirs,
    const std::string& sysconf, const std::string& userconf);
}
}
