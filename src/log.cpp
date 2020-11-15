#include <wayfire/util/log.hpp>
#include <sstream>
#include <functional>
#include <iostream>
#include <map>
#include <chrono>
#include <iomanip>

template<>
std::string wf::log::to_string<void*>(void *arg)
{
    if (!arg)
    {
        return "(null)";
    }

    std::ostringstream out;
    out << arg;
    return out.str();
}

template<>
std::string wf::log::to_string(bool arg)
{
    return arg ? "true" : "false";
}

/**
 * A singleton to hold log configuration.
 */
struct log_global_t
{
    std::reference_wrapper<std::ostream> out = std::ref(std::cout);

    wf::log::log_level_t level = wf::log::LOG_LEVEL_INFO;
    wf::log::color_mode_t color_mode = wf::log::LOG_COLOR_MODE_OFF;
    std::string strip_path = "";

    std::string clear_color = "";

    static log_global_t& get()
    {
        static log_global_t instance;
        return instance;
    }

  private:
    log_global_t()
    {}
};

void wf::log::initialize_logging(std::ostream& output_stream,
    log_level_t minimum_level, color_mode_t color_mode, std::string strip_path)
{
    auto& state = log_global_t::get();
    state.out   = std::ref(output_stream);
    state.level = minimum_level;
    state.color_mode = color_mode;
    state.strip_path = strip_path;

    if (state.color_mode == LOG_COLOR_MODE_ON)
    {
        state.clear_color = "\033[0m";
    }
}

/** Get the line prefix for the given log level */
static std::string get_level_prefix(wf::log::log_level_t level)
{
    bool color = log_global_t::get().color_mode == wf::log::LOG_COLOR_MODE_ON;
    static std::map<wf::log::log_level_t, std::string> color_codes =
    {
        {wf::log::LOG_LEVEL_DEBUG, "\033[0m"},
        {wf::log::LOG_LEVEL_INFO, "\033[0;34m"},
        {wf::log::LOG_LEVEL_WARN, "\033[0;33m"},
        {wf::log::LOG_LEVEL_ERROR, "\033[1;31m"},
    };

    static std::map<wf::log::log_level_t, std::string> line_prefix =
    {
        {wf::log::LOG_LEVEL_DEBUG, "DD"},
        {wf::log::LOG_LEVEL_INFO, "II"},
        {wf::log::LOG_LEVEL_WARN, "WW"},
        {wf::log::LOG_LEVEL_ERROR, "EE"},
    };

    if (color)
    {
        return color_codes[level] + line_prefix[level];
    }

    return line_prefix[level];
}

/** Get the current time and date in a suitable format. */
static std::string get_formatted_date_time()
{
    using namespace std::chrono;
    auto now = system_clock::now();
    auto tt  = system_clock::to_time_t(now);
    auto ms  = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

    std::ostringstream out;
    out << std::put_time(std::localtime(&tt), "%d-%m-%y %H:%M:%S.");
    out << std::setfill('0') << std::setw(3) << ms.count();
    return out.str();
}

/** Strip the strip_path from the given path. */
static std::string strip_path(const std::string& path)
{
    auto prefix = log_global_t::get().strip_path;
    if ((prefix.length() > 0) && (path.find(prefix) == 0))
    {
        return path.substr(prefix.length());
    }

    std::string skip_chars = "./";
    size_t idx = path.find_first_not_of(skip_chars);
    if (idx != std::string::npos)
    {
        return path.substr(idx);
    }

    return path;
}

/**
 * Log a plain message to the given output stream.
 * The output format is:
 *
 * LL DD-MM-YY HH:MM:SS.MSS - [source:line] message
 *
 * @param level The log level of the passed message.
 * @param contents The message to be printed.
 * @param source The file where the message originates from. The prefix
 *  strip_path specified in initialize_logging will be removed, if it exists.
 * @param line The line number of @source
 */
void wf::log::log_plain(log_level_t level, const std::string& contents,
    const std::string& source, int line_nr)
{
    auto& state = log_global_t::get();
    if (state.level > level)
    {
        return;
    }

    std::string path_info;
    if (!source.empty())
    {
        path_info = wf::log::detail::format_concat(
            "[", strip_path(source), ":", line_nr, "] ");
    }

    state.out.get() <<
        wf::log::detail::format_concat(
        get_level_prefix(level), " ",
        get_formatted_date_time(),
        " - ", path_info, contents) <<
        state.clear_color << std::endl;
}
