#include <wayfire/config/file.hpp>
#include <wayfire/config/types.hpp>
#include <wayfire/util/log.hpp>
#include <sstream>
#include <cassert>

class line_t : public std::string
{
  public:
    template<class T> line_t(T source) : std::string(source) {}

    line_t() : std::string() {}
    line_t(const line_t& other) = default;
    line_t(line_t&& other) = default;
    line_t& operator = (const line_t& other) = default;
    line_t& operator = (line_t&& other) = default;

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
        pos = line.find(ch, pos + 1);

    return pos;
}

line_t remove_escaped_sharps(const line_t& line)
{
    line_t result;
    result.source_line_number = line.source_line_number;

    bool had_escape = false;
    for (auto& ch : line)
    {
        if (ch == '#' && had_escape)
            result.pop_back();


        result += ch;
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
            result_line.pop_back();

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
        if (in_concat_mode) {
            assert(!result.empty());
            result.back() += line;
        } else {
            result.push_back(line);
        }

        if (result.empty() || result.back().empty()) {
            in_concat_mode = false;
        } else {
            in_concat_mode = (result.back().back() == '\\');
            if (in_concat_mode) /* pop last \ */
                result.back().pop_back();

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
            result.push_back(line);
    }

    return result;
}

static std::string ignore_leading_trailing_whitespace(const std::string& string)
{
    if (string.empty())
        return "";

    size_t i = 0;
    size_t j = string.size() - 1;
    while (i < j && std::isspace(string[i]))
        ++i;
    while (i < j && std::isspace(string[j]))
        --j;

    return string.substr(i, j - i + 1);
}

enum option_parsing_result {
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
 * to @current_section.
 *
 * @return The parse status of the line.
 */
static option_parsing_result parse_option_line(
    wf::config::section_t& current_section, const line_t& line)
{
    size_t equal_sign = line.find_first_of("=");
    if (equal_sign == std::string::npos)
        return OPTION_PARSED_WRONG_FORMAT;

    auto name = ignore_leading_trailing_whitespace(line.substr(0, equal_sign));
    auto value = ignore_leading_trailing_whitespace(line.substr(equal_sign + 1));

    auto option = current_section.get_option_or(name);
    if (!option)
    {
        using namespace wf;
        option = std::make_shared<config::option_t<std::string>> (name, value);
        current_section.register_new_option(option);
    }

    if (!option->set_value_str(value))
        return OPTION_PARSED_INVALID_CONTENTS;

    return OPTION_PARSED_OK;
}

/**
 * Check whether the @line is a valid section start.
 * If yes, it will either return the section in @config with the same name, or
 * create a new section and register it in config.
 *
 * @return nullptr if line is not a valid section, the section otherwise.
 */
static std::shared_ptr<wf::config::section_t>
    check_section(wf::config::config_manager_t& config, const line_t& line)
{
    auto name = ignore_leading_trailing_whitespace(line);
    if (name.empty() || name.front() != '[' || name.back() != ']')
        return {};

    auto real_name = name.substr(1, name.length() - 2);

    auto section = config.get_section(real_name);
    if (!section)
    {
        section = std::make_shared<wf::config::section_t> (real_name);
        config.merge_section(section);
    }

    return section;
}

void wf::config::load_configuration_options_from_string(
    config_manager_t& config, const std::string& source,
    const std::string& source_name)
{
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

        auto status = parse_option_line(*current_section, line);
        switch(status)
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
}

std::string wf::config::save_configuration_options_to_string(
    const config_manager_t& config)
{
    std::vector<std::string> lines;

    for (auto& section : config.get_all_sections())
    {
        lines.push_back("[" + section->get_name() + "]");
        for (auto& option : section->get_registered_options())
        {
            lines.push_back(
                option->get_name() + " = " + option->get_value_str());
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

        if (!line.empty() && line.back() == '\\')
            line += '\\';
    }

    std::string result;
    for (const auto& line : lines)
        result += line + "\n";

    return result;
}

