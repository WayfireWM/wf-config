#include <sys/inotify.h>
#include "parse.hpp"
#include <sstream>
#include <fstream>

std::ofstream out;

using std::string;

bool wf_key::valid()
{
    return mod > 0 || keyval > 0;
}

bool wf_button::valid()
{
    return mod > 0 || button > 0;
}


/* TODO: add checks to see if values are correct */
wf_option_t::wf_option_t(std::string name)
{
    this->name = name;
}

void wf_option_t::set_value(string value)
{
    raw_value = value;
    is_cached = false;

    if (updated)
        updated();
}

string wf_option_t::as_string()
{
    return (raw_value.empty() || raw_value == "default") ?
        default_value : raw_value;
}

wf_option_t::operator string()
{
    return as_string();
}

int wf_option_t::as_int()
{
    return parse_int(as_string());
}

wf_option_t::operator int()
{
    return as_int();
}

double wf_option_t::as_double()
{
    return parse_double(as_string());
}

wf_option_t::operator double()
{
    return as_double();
}

wf_key wf_option_t::as_key()
{
    return parse_key(as_string());
}

wf_option_t::operator wf_key()
{
    return as_key();
}

wf_button wf_option_t::as_button()
{
    return parse_button(as_string());
}

wf_option_t::operator wf_button()
{
    return as_button();
}

wf_color wf_option_t::as_color()
{
    return parse_color(as_string());
}

wf_option_t::operator wf_color()
{
    return as_color();
}

int wf_option_t::as_cached_int()
{
    if (is_cached)
        return cached.i;
    is_cached = true;
    return cached.i = as_int();
}

double wf_option_t::as_cached_double()
{
    if (is_cached)
        return cached.d;
    is_cached = true;
    return cached.d = as_double();
}

wf_key wf_option_t::as_cached_key()
{
    if (is_cached)
        return cached.key;
    is_cached = true;
    return cached.key = as_key();
}

wf_button wf_option_t::as_cached_button()
{
    if (is_cached)
        return cached.button;

    is_cached = true;
    return cached.button = as_button();
}

wf_color wf_option_t::as_cached_color()
{
    if (is_cached)
        return cached.color;

    is_cached = true;
    return cached.color = as_color();
}
/* convenience functions */
wf_option new_static_option(std::string value)
{
    auto option = std::make_shared<wf_option_t> (value);
    option->set_value(value);

    return option;
}

/* wayfire_config_section implementation */

void wayfire_config_section::update_option(string name, string value)
{
    auto option = get_option(name);
    option->set_value(value);
}

wf_option wayfire_config_section::get_option(string name)
{
    if (options.count(name))
        return options[name];

    auto ptr = options[name] = std::make_shared<wf_option_t> (name);
    return ptr;
}

wf_option wayfire_config_section::get_option(string name, string default_value)
{
    auto option = get_option(name);
    option->default_value = default_value;

    return option;
}

/* wayfire_config implementation */
static string trim(const string& x)
{
    int i = 0, j = x.length() - 1;
    while(i < (int)x.length() && std::iswspace(x[i])) ++i;
    while(j >= 0 && std::iswspace(x[j])) --j;

    if (i <= j)
        return x.substr(i, j - i + 1);
    else
        return "";
}

using lines_t = std::vector<string>;
static void prune_comments(lines_t& file)
{
    for (auto& line : file)
    {
        size_t i = line.find_first_of('#');
        if (i != string::npos)
            line = line.substr(0, i);
    }
}

static lines_t filter_empty_lines(const lines_t& file)
{
    lines_t pruned;
    for (const auto& line : file)
    {
        string cleaned_line = trim(line);
        if (cleaned_line.size())
            pruned.push_back(cleaned_line);
    }

    return pruned;
}

static lines_t merge_lines(const lines_t& file)
{
    lines_t merged;
    int i = 0;
    for (; i < (int)file.size(); i++)
    {
        string result = file[i]; ++i;
        while(file[i - 1].back() == '\\' && i < (int)file.size())
        {
            result.pop_back();
            result += file[i++];
        }
        --i;

        merged.push_back(result);
    }

    return merged;
}

static lines_t read_lines(std::string file_name)
{
    std::ifstream file(file_name);

    string line;
    lines_t lines;

    while(std::getline(file, line))
        lines.push_back(line);

    return lines;
}

wayfire_config::wayfire_config(string name)
{
    fname = name;
    reload_config();
}

void wayfire_config::reload_config()
{
    /* reset all options to empty, meaning default values */
    for (auto& section : sections)
        for (auto option : section.second->options)
            section.second->update_option(option.first, "");

    auto lines = read_lines(fname);
    prune_comments(lines);
    lines = filter_empty_lines(lines);
    lines = merge_lines(lines);

    wayfire_config_section *current_section = NULL;
    for (auto line : lines)
    {
        if (line[0] == '[')
        {
            auto name = line.substr(1, line.size() - 2);
            current_section = get_section(name);
            continue;
        }

        if (!current_section)
            continue;

        string name, value;
        size_t i = line.find_first_of('=');
        if (i != string::npos)
        {
            name = trim(line.substr(0, i));
            value = trim(line.substr(i + 1, line.size() - i - 1));

            current_section->update_option(name, value);
        }
    }
}

wayfire_config_section* wayfire_config::get_section(const string& name)
{
    if (sections.count(name))
        return sections[name];

    auto nsect = new wayfire_config_section();
    nsect->name = name;
    sections[name] = nsect;

    return nsect;
}

wayfire_config_section* wayfire_config::operator[](const string& name)
{
    return get_section(name);
}
