#include <wayfire/config/compound-option.hpp>
#include <wayfire/config/file.hpp>
#include <wayfire/config/types.hpp>
#include <wayfire/config/xml.hpp>
#include <wayfire/util/log.hpp>
#include <sstream>
#include <fstream>
#include <cassert>
#include <set>

#include "option-impl.hpp"

#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

class line_t : public std::string
{
  public:
    template<class T>
    line_t(T source) : std::string(source)
    {}

    line_t() : std::string()
    {}
    line_t(const line_t& other) = default;
    line_t(line_t&& other) = default;
    line_t& operator =(const line_t& other) = default;
    line_t& operator =(line_t&& other) = default;

  public:
    line_t substr(size_t start, size_t length = npos) const
    {
        line_t result = std::string::substr(start, length);
        result.source_line_number = this->source_line_number;
        return result;
    }

    size_t source_line_number;
};
using lines_t = std::vector<line_t>;

static lines_t split_to_lines(const std::string& source)
{
    std::istringstream stream(source);
    lines_t output;
    line_t line;

    size_t line_idx = 1;
    while (std::getline(stream, line))
    {
        line.source_line_number = line_idx;
        output.push_back(line);
        ++line_idx;
    }

    return output;
}

/**
 * Check whether at the given index @idx in @line, there is a character
 * @ch which isn't escaped (i.e preceded by \).
 */
static bool is_nonescaped(const std::string& line, char ch, int idx)
{
    return line[idx] == ch && (idx == 0 || line[idx - 1] != '\\');
}

static size_t find_first_nonescaped(const std::string& line, char ch)
{
    /* Find first not-escaped # */
    size_t pos = 0;
    while (pos != std::string::npos && !is_nonescaped(line, ch, pos))
    {
        pos = line.find(ch, pos + 1);
    }

    return pos;
}

line_t remove_escaped_sharps(const line_t& line)
{
    line_t result;
    result.source_line_number = line.source_line_number;

    bool had_escape = false;
    for (auto& ch : line)
    {
        if ((ch == '#') && had_escape)
        {
            result.pop_back();
        }

        result    += ch;
        had_escape = (ch == '\\');
    }

    return result;
}

static lines_t remove_comments(const lines_t& lines)
{
    lines_t result;
    for (const auto& line : lines)
    {
        auto pos = find_first_nonescaped(line, '#');
        result.push_back(
            remove_escaped_sharps(line.substr(0, pos)));
    }

    return result;
}

static lines_t remove_trailing_whitespace(const lines_t& lines)
{
    lines_t result;
    for (const auto& line : lines)
    {
        auto result_line = line;
        while (!result_line.empty() && std::isspace(result_line.back()))
        {
            result_line.pop_back();
        }

        result.push_back(result_line);
    }

    return result;
}

lines_t join_lines(const lines_t& lines)
{
    lines_t result;
    bool in_concat_mode = false;

    for (const auto& line : lines)
    {
        if (in_concat_mode)
        {
            assert(!result.empty());
            result.back() += line;
        } else
        {
            result.push_back(line);
        }

        if (result.empty() || result.back().empty())
        {
            in_concat_mode = false;
        } else
        {
            in_concat_mode = (result.back().back() == '\\');
            if (in_concat_mode) /* pop last \ */
            {
                result.back().pop_back();
            }

            /* If last \ was escaped, we should ignore it */
            bool was_escaped =
                !result.back().empty() && result.back().back() == '\\';
            in_concat_mode = in_concat_mode && !was_escaped;
        }
    }

    return result;
}

lines_t skip_empty(const lines_t& lines)
{
    lines_t result;
    for (auto& line : lines)
    {
        if (!line.empty())
        {
            result.push_back(line);
        }
    }

    return result;
}

static std::string ignore_leading_trailing_whitespace(const std::string& string)
{
    if (string.empty())
    {
        return "";
    }

    size_t i = 0;
    size_t j = string.size() - 1;
    while (i < j && std::isspace(string[i]))
    {
        ++i;
    }

    while (i < j && std::isspace(string[j]))
    {
        --j;
    }

    return string.substr(i, j - i + 1);
}

enum option_parsing_result
{
    /* Line was valid */
    OPTION_PARSED_OK,
    /* Line has wrong format */
    OPTION_PARSED_WRONG_FORMAT,
    /* Specified value does not match existing option type */
    OPTION_PARSED_INVALID_CONTENTS,
};

/**
 * Try to parse an option line.
 * If the option line is valid, the corresponding option is modified or added
 * to @current_section, and the option is added to @reloaded.
 *
 * @return The parse status of the line.
 */
static option_parsing_result parse_option_line(
    wf::config::section_t& current_section, const line_t& line,
    std::set<std::shared_ptr<wf::config::option_base_t>>& reloaded)
{
    size_t equal_sign = line.find_first_of("=");
    if (equal_sign == std::string::npos)
    {
        return OPTION_PARSED_WRONG_FORMAT;
    }

    auto name  = ignore_leading_trailing_whitespace(line.substr(0, equal_sign));
    auto value = ignore_leading_trailing_whitespace(line.substr(equal_sign + 1));

    auto option = current_section.get_option_or(name);
    if (!option)
    {
        using namespace wf;
        option = std::make_shared<config::option_t<std::string>>(name, "");
        option->set_value_str(value);
        current_section.register_new_option(option);
    }

    if (option->is_locked() || option->set_value_str(value))
    {
        reloaded.insert(option);
        return OPTION_PARSED_OK;
    }

    return OPTION_PARSED_INVALID_CONTENTS;
}

/**
 * Check whether the @line is a valid section start.
 * If yes, it will either return the section in @config with the same name, or
 * create a new section and register it in config.
 *
 * @return nullptr if line is not a valid section, the section otherwise.
 */
static std::shared_ptr<wf::config::section_t> check_section(
    wf::config::config_manager_t& config, const line_t& line)
{
    auto name = ignore_leading_trailing_whitespace(line);
    if (name.empty() || (name.front() != '[') || (name.back() != ']'))
    {
        return {};
    }

    auto real_name = name.substr(1, name.length() - 2);

    auto section = config.get_section(real_name);
    if (!section)
    {
        size_t splitter = real_name.find_first_of(":");
        if (splitter != std::string::npos)
        {
            auto obj_type_name = real_name.substr(0, splitter);
            auto section_name  = real_name.substr(splitter + 1); // only for the
                                                                 // empty check
            if (!obj_type_name.empty() && !section_name.empty())
            {
                auto parent_section = config.get_section(obj_type_name);
                if (parent_section)
                {
                    section = parent_section->clone_with_name(real_name);
                    config.merge_section(section);
                    return section;
                }
            }
        }

        section = std::make_shared<wf::config::section_t>(real_name);
        config.merge_section(section);
    }

    return section;
}

void wf::config::load_configuration_options_from_string(
    config_manager_t& config, const std::string& source,
    const std::string& source_name)
{
    std::set<std::shared_ptr<option_base_t>> reloaded;

    auto lines =
        skip_empty(
            join_lines(
                remove_trailing_whitespace(
                    remove_comments(
                        split_to_lines(source)))));

    std::shared_ptr<wf::config::section_t> current_section;

    for (const auto& line : lines)
    {
        auto next_section = check_section(config, line);
        if (next_section)
        {
            current_section = next_section;
            continue;
        }

        if (!current_section)
        {
            LOGE("Error in file ", source_name, ":", line.source_line_number,
                ", option declared before a section starts!");
            continue;
        }

        auto status = parse_option_line(*current_section, line, reloaded);
        switch (status)
        {
          case OPTION_PARSED_WRONG_FORMAT:
            LOGE("Error in file ", source_name, ":",
                line.source_line_number, ", invalid option format ",
                "(allowed <option_name> = <value>)");
            break;

          case OPTION_PARSED_INVALID_CONTENTS:
            LOGE("Error in file ", source_name, ":",
                line.source_line_number, ", invalid option value!");
            break;

          default:
            break;
        }
    }

    // Go through all options and reset options which are loaded from the config
    // string but are not there anymore.
    for (auto section : config.get_all_sections())
    {
        for (auto opt : section->get_registered_options())
        {
            opt->priv->option_in_config_file = (reloaded.count(opt) > 0);
            if (!opt->priv->option_in_config_file && !opt->is_locked())
            {
                opt->reset_to_default();
            }
        }
    }

    // After resetting all options which are no longer in the config file, make
    // sure to rebuild compound options as well.
    for (auto section : config.get_all_sections())
    {
        for (auto opt : section->get_registered_options())
        {
            auto as_compound = std::dynamic_pointer_cast<compound_option_t>(opt);
            if (as_compound)
            {
                update_compound_from_section(*as_compound, section);
            }
        }
    }
}

std::string wf::config::save_configuration_options_to_string(
    const config_manager_t& config)
{
    std::vector<std::string> lines;

    for (auto& section : config.get_all_sections())
    {
        lines.push_back("[" + section->get_name() + "]");

        // Go through each option and add the necessary lines.
        // Take care so that regular options overwrite compound options
        // in case of conflict!
        std::map<std::string, std::string> option_values;
        std::set<std::string> all_compound_prefixes;
        for (auto& option : section->get_registered_options())
        {
            auto as_compound = std::dynamic_pointer_cast<compound_option_t>(option);
            if (as_compound)
            {
                auto value = as_compound->get_value_untyped();
                const auto& prefixes = as_compound->get_entries();
                for (auto& p : prefixes)
                {
                    all_compound_prefixes.insert(p->get_prefix());
                }

                for (size_t i = 0; i < value.size(); i++)
                {
                    for (size_t j = 0; j < prefixes.size(); j++)
                    {
                        auto full_name = prefixes[j]->get_prefix() + value[i][0];
                        option_values[full_name] = value[i][j + 1];
                    }
                }
            }
        }

        // An option is part of a compound option if it begins with any of the
        // prefixes.
        const auto& is_part_of_compound_option = [&] (const std::string& name)
        {
            return std::any_of(
                all_compound_prefixes.begin(), all_compound_prefixes.end(),
                [&] (const auto& prefix)
            {
                return name.substr(0, prefix.size()) == prefix;
            });
        };

        for (auto& option : section->get_registered_options())
        {
            auto as_compound = std::dynamic_pointer_cast<compound_option_t>(option);
            if (!as_compound)
            {
                // Check whether this option does not conflict with a compound
                // option entry.
                if (xml::get_option_xml_node(option) ||
                    !is_part_of_compound_option(option->get_name()))
                {
                    option_values[option->get_name()] = option->get_value_str();
                }
            }
        }

        for (auto& [name, value] : option_values)
        {
            lines.push_back(name + " = " + value);
        }

        lines.push_back("");
    }

    /* Check which characters need escaping */
    for (auto& line : lines)
    {
        size_t sharp = line.find_first_of("#");
        while (sharp != line.npos)
        {
            line.insert(line.begin() + sharp, '\\');
            sharp = line.find_first_of("#", sharp + 2);
        }

        if (!line.empty() && (line.back() == '\\'))
        {
            line += '\\';
        }
    }

    std::string result;
    for (const auto& line : lines)
    {
        result += line + "\n";
    }

    return result;
}

static std::string load_file_contents(const std::string& file)
{
    std::ifstream infile(file);
    std::string file_contents((std::istreambuf_iterator<char>(infile)),
        std::istreambuf_iterator<char>());

    return file_contents;
}

bool wf::config::load_configuration_options_from_file(config_manager_t& manager,
    const std::string& file)
{
    /* Try to lock the file */
    auto fd = open(file.c_str(), O_RDONLY);
    if (flock(fd, LOCK_SH | LOCK_NB))
    {
        close(fd);
        return false;
    }

    auto file_contents = load_file_contents(file);

    /* Release lock */
    flock(fd, LOCK_UN);
    close(fd);

    load_configuration_options_from_string(manager, file_contents, file);
    return true;
}

void wf::config::save_configuration_to_file(
    const wf::config::config_manager_t& manager, const std::string& file)
{
    auto contents = save_configuration_options_to_string(manager);
    contents.pop_back(); // remove last newline

    auto fd = open(file.c_str(), O_RDONLY);
    flock(fd, LOCK_EX);

    auto fout = std::ofstream(file, std::ios::trunc);
    fout << contents;

    flock(fd, LOCK_UN);
    close(fd);

    /* Modify the file one last time. Now programs waiting for updates can
     * acquire a shared lock. */
    fout << std::endl;
}

/**
 * Parse the XML file and return the node which corresponds to the section.
 */
static xmlNodePtr find_section_start_node(const std::string& file)
{
    auto doc = xmlParseFile(file.c_str());
    if (!doc)
    {
        LOGE("Failed to parse XML file ", file);
        return nullptr;
    }

    auto root = xmlDocGetRootElement(doc);
    if (!root)
    {
        LOGE(file, ": missing root element.");
        xmlFreeDoc(doc);
        return nullptr;
    }

    /* Seek the plugin section */
    auto section = root->children;
    while (section != nullptr)
    {
        if ((section->type == XML_ELEMENT_NODE) &&
            (((const char*)section->name == (std::string)"plugin") ||
             ((const char*)section->name == (std::string)"object")))
        {
            break;
        }

        section = section->next;
    }

    return section;
}

static wf::config::config_manager_t load_xml_files(
    const std::vector<std::string>& xmldirs)
{
    wf::config::config_manager_t manager;

    for (auto& xmldir : xmldirs)
    {
        auto xmld = opendir(xmldir.c_str());
        if (NULL == xmld)
        {
            LOGW("Failed to open XML directory ", xmldir);
            continue;
        }

        LOGI("Reading XML configuration options from directory ", xmldir);
        struct dirent *entry;
        while ((entry = readdir(xmld)) != NULL)
        {
            if ((entry->d_type != DT_LNK) && (entry->d_type != DT_REG))
            {
                continue;
            }

            std::string filename = xmldir + '/' + entry->d_name;
            if ((filename.length() > 4) &&
                (filename.rfind(".xml") == filename.length() - 4))
            {
                LOGI("Reading XML configuration options from file ", filename);
                auto node = find_section_start_node(filename);
                if (node)
                {
                    manager.merge_section(
                        wf::config::xml::create_section_from_xml_node(node));
                }
            }
        }

        closedir(xmld);
    }

    return manager;
}

void override_defaults(wf::config::config_manager_t& manager,
    const std::string& sysconf)
{
    auto sysconf_str = load_file_contents(sysconf);

    wf::config::config_manager_t overrides;
    load_configuration_options_from_string(overrides, sysconf_str, sysconf);
    for (auto& section : overrides.get_all_sections())
    {
        for (auto& option : section->get_registered_options())
        {
            auto full_name   = section->get_name() + '/' + option->get_name();
            auto real_option = manager.get_option(full_name);
            if (real_option)
            {
                if (!real_option->set_default_value_str(
                    option->get_value_str()))
                {
                    LOGW("Invalid value for ", full_name, " in ", sysconf);
                } else
                {
                    /* Set the value to the new default */
                    real_option->reset_to_default();
                }
            } else
            {
                LOGW("Unused default value for ", full_name, " in ", sysconf);
            }
        }
    }
}

#include <iostream>

wf::config::config_manager_t wf::config::build_configuration(
    const std::vector<std::string>& xmldirs, const std::string& sysconf,
    const std::string& userconf)
{
    auto manager = load_xml_files(xmldirs);
    override_defaults(manager, sysconf);
    load_configuration_options_from_file(manager, userconf);
    return manager;
}
