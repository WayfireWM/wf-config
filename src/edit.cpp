#include "edit-impl.hpp"

#include <wayfire/util/log.hpp>

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <utility>
#include <vector>

using wf::config::config_manager_t;
using wf::config::edit::patch;

static std::string_view trim(std::string_view sv)
{
    auto ws = [] (char c) { return std::isspace(static_cast<unsigned char>(c)); };
    while (!sv.empty() && ws(sv.front()))
    {
        sv.remove_prefix(1);
    }

    while (!sv.empty() && ws(sv.back()))
    {
        sv.remove_suffix(1);
    }

    return sv;
}

/* Keep the trailing '\n' on each line so concatenation reproduces the input. */
static std::vector<std::string> split_lines(std::string_view source)
{
    std::vector<std::string> lines;
    size_t start = 0;
    for (size_t idx = 0; idx < source.size(); idx++)
    {
        if (source[idx] == '\n')
        {
            lines.emplace_back(source.substr(start, idx - start + 1));
            start = idx + 1;
        }
    }

    if (start < source.size())
    {
        lines.emplace_back(source.substr(start));
    }

    return lines;
}

/* Return the section name for a "[name]" line, or empty string otherwise. */
static std::string parse_section_header(std::string_view line)
{
    auto trimmed = trim(line);
    if ((trimmed.size() < 2) || (trimmed.front() != '[') || (trimmed.back() != ']'))
    {
        return "";
    }

    return std::string(trimmed.substr(1, trimmed.size() - 2));
}

/* '#' starts a comment, '\' escapes the next char. */
static size_t find_key_equals(std::string_view logical)
{
    for (size_t idx = 0; idx < logical.size(); idx++)
    {
        char ch = logical[idx];
        if ((ch == '\\') && (idx + 1 < logical.size()))
        {
            idx++;
            continue;
        }

        if (ch == '#')
        {
            return std::string::npos;
        }

        if (ch == '=')
        {
            return idx;
        }
    }

    return std::string::npos;
}

/* Value runs to the next unescaped '#' or end of line. */
static size_t find_value_end(std::string_view logical, size_t equals_at)
{
    for (size_t idx = equals_at + 1; idx < logical.size(); idx++)
    {
        char ch = logical[idx];
        if ((ch == '\\') && (idx + 1 < logical.size()))
        {
            idx++;
            continue;
        }

        if (ch == '#')
        {
            return idx;
        }
    }

    return logical.size();
}

struct logical_line
{
    size_t first_index = 0;
    size_t last_index  = 0;
    std::string joined;
};

/* Trailing '\' continues onto the next line, unless escaped by another '\'. */
static logical_line read_logical(const std::vector<std::string>& lines, size_t first)
{
    logical_line result;
    result.first_index = first;
    result.last_index  = first;

    for (size_t idx = first; idx < lines.size(); idx++)
    {
        std::string_view line = lines[idx];
        if (!line.empty() && (line.back() == '\n'))
        {
            line.remove_suffix(1);
        }

        result.joined    += line;
        result.last_index = idx;

        size_t trailing_backslashes = 0;
        for (auto it = line.rbegin(); it != line.rend() && (*it == '\\'); ++it)
        {
            trailing_backslashes++;
        }

        if ((trailing_backslashes % 2) == 0)
        {
            break;
        }

        result.joined.pop_back(); /* drop the trailing '\' */
    }

    return result;
}

struct section_edit
{
    size_t header_index = std::string::npos;
    size_t next_header_index = std::string::npos;
    bool exists = false;
};

/* When missing, header_index == lines.size() so callers append. */
static section_edit find_section(const std::vector<std::string>& lines,
    const std::string& section_name)
{
    section_edit found;

    for (size_t idx = 0; idx < lines.size(); idx++)
    {
        auto this_section = parse_section_header(lines[idx]);
        if (this_section.empty())
        {
            continue;
        }

        if (!found.exists && (this_section == section_name))
        {
            found.header_index = idx;
            found.exists = true;
            continue;
        }

        if (found.exists)
        {
            found.next_header_index = idx;
            return found;
        }
    }

    if (!found.exists)
    {
        found.header_index = lines.size();
    } else
    {
        found.next_header_index = lines.size();
    }

    return found;
}

struct key_edit
{
    size_t logical_first_index = std::string::npos;
    size_t logical_last_index  = std::string::npos;
    std::string logical_joined;
    size_t equals_at = std::string::npos;
    size_t value_end = std::string::npos;
    bool exists = false;
};

static key_edit find_key(const std::vector<std::string>& lines,
    size_t from, size_t until, const std::string& key_name)
{
    key_edit found;

    size_t idx = from;
    while (idx < until)
    {
        auto logical = read_logical(lines, idx);
        if (logical.last_index >= until)
        {
            break;
        }

        auto equals_at = find_key_equals(logical.joined);
        if (equals_at != std::string::npos)
        {
            auto this_key = trim(std::string_view(logical.joined).substr(0, equals_at));
            if (this_key == key_name)
            {
                found.logical_first_index = logical.first_index;
                found.logical_last_index  = logical.last_index;
                found.logical_joined = std::move(logical.joined);
                found.equals_at = equals_at;
                found.value_end = find_value_end(found.logical_joined, equals_at);
                found.exists    = true;
                return found;
            }
        }

        idx = logical.last_index + 1;
    }

    return found;
}

/* Preserve the '=' spacing and any trailing '# comment' from the old line. */
static std::string rewrite_key_line(const std::string& value,
    const std::string& old_logical, size_t equals_at, size_t value_end)
{
    auto is_h = [] (char c) { return c == ' ' || c == '\t'; };

    size_t vs = equals_at + 1;
    while ((vs < value_end) && is_h(old_logical[vs]))
    {
        vs++;
    }

    size_t ve = value_end;
    while ((ve > vs) && is_h(old_logical[ve - 1]))
    {
        ve--;
    }

    std::string line;
    line.reserve(old_logical.size() + value.size() + 1);
    line.append(old_logical, 0, equals_at + 1);                  /* key + '=' */
    line.append(old_logical, equals_at + 1, vs - equals_at - 1); /* leading ws */
    line.append(value);
    line.append(old_logical, ve, value_end - ve);                /* trailing ws */
    line.append(old_logical, value_end, std::string::npos);      /* trailing '# comment' */
    line.push_back('\n');
    return line;
}

/* Blank + comment-only lines still count as empty. */
static bool section_body_is_empty(const std::vector<std::string>& lines,
    size_t from, size_t until)
{
    size_t idx = from;
    while (idx < until)
    {
        auto logical = read_logical(lines, idx);
        if (logical.last_index >= until)
        {
            break;
        }

        auto trimmed = trim(logical.joined);
        if (!trimmed.empty() && (trimmed.front() != '#') &&
            (find_key_equals(logical.joined) != std::string::npos))
        {
            return false;
        }

        idx = logical.last_index + 1;
    }

    return true;
}

static void apply_patch(std::vector<std::string>& lines,
    const std::string& section, const std::string& key, const std::string& value)
{
    auto sec = find_section(lines, section);

    if (!sec.exists)
    {
        /* Ensure the previous last line ends in '\n', then append blank / header / key. */
        if (!lines.empty() && !lines.back().empty() && (lines.back().back() != '\n'))
        {
            lines.back().push_back('\n');
        }

        if (!lines.empty())
        {
            lines.push_back("\n");
        }

        lines.push_back("[" + section + "]\n");
        lines.push_back(key + " = " + value + "\n");
        return;
    }

    auto key_e = find_key(lines, sec.header_index + 1, sec.next_header_index, key);

    if (key_e.exists)
    {
        lines[key_e.logical_first_index] = rewrite_key_line(value,
            key_e.logical_joined, key_e.equals_at, key_e.value_end);
        if (key_e.logical_last_index > key_e.logical_first_index)
        {
            lines.erase(lines.begin() + key_e.logical_first_index + 1,
                lines.begin() + key_e.logical_last_index + 1);
        }

        return;
    }

    lines.insert(lines.begin() + sec.next_header_index, key + " = " + value + "\n");
}

static void apply_reset(std::vector<std::string>& lines,
    const std::string& section, const std::string& key)
{
    auto sec = find_section(lines, section);
    if (!sec.exists)
    {
        return;
    }

    auto key_e = find_key(lines, sec.header_index + 1, sec.next_header_index, key);
    if (!key_e.exists)
    {
        return;
    }

    lines.erase(lines.begin() + key_e.logical_first_index,
        lines.begin() + key_e.logical_last_index + 1);
}

/* Drop any section whose body has no key assignments left. */
static void drop_empty_sections(std::vector<std::string>& lines)
{
    std::vector<std::pair<size_t, size_t>> drop_spans;

    size_t idx = 0;
    while (idx < lines.size())
    {
        auto section_name = parse_section_header(lines[idx]);
        if (section_name.empty())
        {
            idx++;
            continue;
        }

        size_t next = idx + 1;
        while ((next < lines.size()) && parse_section_header(lines[next]).empty())
        {
            next++;
        }

        if (section_body_is_empty(lines, idx + 1, next))
        {
            drop_spans.emplace_back(idx, next);
        }

        idx = next;
    }

    /* Erase in reverse so earlier spans keep their indices. */
    for (auto it = drop_spans.rbegin(); it != drop_spans.rend(); ++it)
    {
        lines.erase(lines.begin() + it->first, lines.begin() + it->second);
    }
}

std::string wf::config::edit::apply(std::string_view source,
    const std::vector<patch>& patches)
{
    auto lines = split_lines(source);

    bool had_reset = false;
    for (const auto& p : patches)
    {
        if (p.value.has_value())
        {
            apply_patch(lines, p.section, p.key, *p.value);
        } else
        {
            apply_reset(lines, p.section, p.key);
            had_reset = true;
        }
    }

    if (had_reset)
    {
        drop_empty_sections(lines);
    }

    std::string result;
    for (const auto& line : lines)
    {
        result += line;
    }

    return result;
}

static std::vector<patch> validate_patches(const config_manager_t& manager,
    const std::vector<patch>& patches)
{
    std::vector<patch> accepted;
    accepted.reserve(patches.size());

    for (const auto& p : patches)
    {
        if (!p.value.has_value())
        {
            accepted.push_back(p);
            continue;
        }

        auto option  = manager.get_option(p.section + "/" + p.key);
        auto scratch = option ? option->clone_option() : nullptr;
        if (scratch && scratch->set_value_str(*p.value))
        {
            accepted.push_back(patch{p.section, p.key, scratch->get_value_str()});
        }
    }

    return accepted;
}

static bool has_write(const std::vector<patch>& patches)
{
    return std::any_of(patches.begin(), patches.end(),
        [] (const patch& p) { return p.value.has_value(); });
}

bool wf::config::edit::patch_file(const std::string& path,
    const std::vector<patch>& patches,
    const config_manager_t *manager)
{
    std::vector<patch> validated;
    if (manager)
    {
        validated = validate_patches(*manager, patches);
    }

    const auto& effective = manager ? validated : patches;

    if (effective.empty())
    {
        return true;
    }

    std::string source;
    struct stat st;
    if (stat(path.c_str(), &st) == 0)
    {
        std::ifstream in(path);
        if (!in.is_open())
        {
            LOGE("Failed to open ", path, " for read: ", std::strerror(errno));
            return false;
        }

        source = std::string((std::istreambuf_iterator<char>(in)),
            std::istreambuf_iterator<char>());
    } else if (errno != ENOENT)
    {
        LOGE("Failed to stat ", path, ": ", std::strerror(errno));
        return false;
    } else if (!has_write(effective))
    {
        return true;
    }

    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open())
    {
        LOGE("Failed to open ", path, " for write: ", std::strerror(errno));
        return false;
    }

    out << wf::config::edit::apply(source, effective);
    out.close();
    if (out.fail())
    {
        LOGE("Write to ", path, " failed: ", std::strerror(errno));
        return false;
    }

    return true;
}
