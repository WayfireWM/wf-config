#include "parse.hpp"
#include <sstream>
#include <fstream>

std::ofstream out;

using std::string;
/* TODO: add checks to see if values are correct */

string wayfire_config_section::get_string(string name, string default_value)
{
    auto it = options.find(name);
    return (it == options.end() ? default_value : it->second);
}

int wayfire_config_section::get_int(string name, int df)
{
    auto it = options.find(name);
    return (it == options.end() ? df : parse_int(it->second));
}

int wayfire_config_section::get_duration(string name, int df)
{
    int result = get_int(name, df * (1000 / refresh_rate));
    return result / (1000 / refresh_rate);
}

double wayfire_config_section::get_double(string name, double df)
{
    auto it = options.find(name);
    return (it == options.end() ? df : parse_double(it->second));
}

wf_key wayfire_config_section::get_key(string name, wf_key df)
{
    auto it = options.find(name);
    if (it == options.end())
        return df;

    return parse_key(it->second);
}

wf_button wayfire_config_section::get_button(string name, wf_button df)
{
    auto it = options.find(name);
    if (it == options.end())
        return df;

    return parse_button(it->second);
}

wf_color wayfire_config_section::get_color(string name, wf_color df)
{
    auto it = options.find(name);
    if (it == options.end())
        return df;

    return parse_color(it->second);
}

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

wayfire_config::wayfire_config(string name, int rr)
{
    std::ifstream file(name);
    string line;

#ifdef WAYFIRE_DEBUG_ENABLED
    out.open("/tmp/.wayfire_config_debug");
    out << "use config: " << name << std::endl;
#endif

    refresh_rate = rr;
    wayfire_config_section *current_section = NULL;

    lines_t lines;
    while(std::getline(file, line))
    {
        lines.push_back(line);
    }

    prune_comments(lines);
    lines = filter_empty_lines(lines);
    lines = merge_lines(lines);

    for (auto line : lines)
    {
        if (line[0] == '[')
        {
            current_section = new wayfire_config_section();
            current_section->refresh_rate = rr;
            current_section->name = line.substr(1, line.size() - 2);
            sections.push_back(current_section);
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
            current_section->options[name] = value;
        }
    }
}

wayfire_config_section* wayfire_config::get_section(string name)
{
    for (auto section : sections)
        if (section->name == name)
            return section;

    auto nsect = new wayfire_config_section();
    nsect->name = name;
    nsect->refresh_rate = refresh_rate;
    sections.push_back(nsect);
    return nsect;
}

void wayfire_config::set_refresh_rate(int rr)
{
    for (auto section : sections)
        section->refresh_rate = rr;
}
