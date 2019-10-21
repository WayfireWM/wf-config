#include <wayfire/util/log.hpp>
#include <sstream>
#include <functional>
#include <iostream>
#include <map>
#include <chrono>
#include <iomanip>

template<> std::string wf::log::to_string<void*> (void* arg)
{
    if (!arg)
        return "(null)";

    std::ostringstream out;
    out << arg;
    return out.str();
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

    static log_global_t& get()
    {
        static log_global_t instance;
        return instance;
    }

  private:
    log_global_t() {};
};

void wf::log::initialize_logging(std::ostream& output_stream,
    log_level_t minimum_level, color_mode_t color_mode, std::string strip_path)
{
    auto& state = log_global_t::get();
    state.out = std::ref(output_stream);
    state.level = minimum_level;
    state.color_mode = color_mode;
    state.strip_path = strip_path;
}

/** Get the line prefix for the given log level */
static std::string get_level_prefix(wf::log::log_level_t level)
{
    static std::map<wf::log::log_level_t, std::string> line_prefix =
    {
        {wf::log::LOG_LEVEL_DEBUG, "DD"},
        {wf::log::LOG_LEVEL_INFO,  "II"},
        {wf::log::LOG_LEVEL_WARN,  "WW"},
        {wf::log::LOG_LEVEL_ERROR, "EE"},
    };
    return line_prefix[level];
}

/** Get the current time and date in a suitable format. */
static std::string get_formatted_date_time()
{
    using namespace std::chrono;
    auto now = system_clock::now();
    auto tt = system_clock::to_time_t(now);
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

    std::ostringstream out;
    out << std::put_time(std::localtime(&tt), "%d-%m-%y %H:%M:%S.");
    out << std::setfill('0') << std::setw(3) << ms.count();
    return out.str();
}

/** Strip the strip_path from the given path. */
static std::string strip_path(const std::string& path)
{
    auto prefix = log_global_t::get().strip_path;
    if (path.find(prefix) == 0)
        return path.substr(prefix.length());

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
        return;

    state.out.get() <<
        wf::log::detail::format_concat(
            get_level_prefix(level), " ",
            get_formatted_date_time(),
            " - [", strip_path(source), ":", line_nr, "] ", contents)
        << std::endl;
}

