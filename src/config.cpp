#include <sys/file.h>
#include <unistd.h>
#include "parse.hpp"
#include <sstream>
#include <fstream>

std::ofstream out;

using std::string;
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

void wf_option_t::set_value(string value, int64_t age)
{
    if (age == -1)
        age = this->age;

    this->age = age;
    value = trim(value);

    if (raw_value != value)
    {
        raw_value = value;
        is_cached = false;

        auto to_call = updated;
        for (auto call : to_call)
            (*call)();
    }
}

void wf_option_t::set_value(int value, int64_t age)
{
    set_value(std::to_string(value), age);
}

void wf_option_t::set_value(double value, int64_t age)
{
    auto old = std::locale::global(std::locale("C"));
    auto conv = std::to_string(value);
    std::locale::global(old);

    set_value(conv, age);
}

void wf_option_t::set_value(const wf_key& value, int64_t age)
{
    set_value(to_string(value), age);
}

void wf_option_t::set_value(const wf_button& value, int64_t age)
{
    set_value(to_string(value), age);
}

void wf_option_t::set_value(const wf_color& value, int64_t age)
{
    set_value(to_string(value), age);
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

void wayfire_config_section::update_option(string name, string value, int64_t age)
{
    auto option = get_option(name);
    option->set_value(value, age);
}

wf_option wayfire_config_section::get_option(string name)
{
    for (auto opt : options)
    {
        if (opt->name == name)
            return opt;
    }

    auto opt = std::make_shared<wf_option_t> (name);
    options.push_back(opt);
    return opt;
}

wf_option wayfire_config_section::get_option(string name, string default_value)
{
    auto option = get_option(name);
    option->default_value = default_value;

    return option;
}

/* wayfire_config implementation */


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
    ++reload_age;

    auto fd = open(fname.c_str(), O_RDONLY);

    if (flock(fd, LOCK_SH | LOCK_NB))
    {
        close(fd);
        return;
    }

    auto lines = read_lines(fname);
    flock(fd, LOCK_UN);
    close(fd);

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

            current_section->update_option(name, value, reload_age);
        }
    }
    /* reset all options to empty, meaning default values */
    for (auto& section : sections)
    {
        for (auto option : section->options)
        {
            if (option->age < reload_age)
                section->update_option(option->name, "", reload_age);
        }
    }
}

void wayfire_config::save_config()
{
    save_config(fname);
}

void wayfire_config::save_config(std::string file)
{
    auto fd = open(file.c_str(), O_RDONLY);
    flock(fd, LOCK_EX);

    auto fout = std::ofstream(file, std::ios::trunc);

    for (auto section : sections)
    {
        fout << "[" << section->name << "]\n";
        for (auto opt : section->options)
        {
            /* there is no reason to save default values and empty values are
             * ignored when reading anyway */
            if ((opt->is_from_file || opt->raw_value != opt->default_value)
                && !opt->raw_value.empty())
                fout << opt->name << " = " << opt->raw_value << "\n";
        }

        fout << std::endl;
    }

    flock(fd, LOCK_UN);
    close(fd);
    fout << std::endl;
}

wayfire_config_section* wayfire_config::get_section(const string& name)
{
    for (auto section : sections)
    {
        if (section->name == name)
            return section;
    }


    auto nsect = new wayfire_config_section();
    nsect->name = name;
    sections.push_back(nsect);
    return nsect;
}

wayfire_config_section* wayfire_config::operator[](const string& name)
{
    return get_section(name);
}
