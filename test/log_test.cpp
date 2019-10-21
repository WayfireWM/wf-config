#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <wayfire/util/log.hpp>

struct test_struct
{
    int x, y;
};

namespace wf
{
namespace log
{
template<>
    std::string to_string<test_struct>(test_struct str)
{
    return "(" + std::to_string(str.x) + "," + std::to_string(str.y) + ")";
}
}
}

TEST_CASE("wf::log::detail::format_concat()")
{
    using namespace wf::log;

    CHECK(detail::format_concat("test", 123) == "test123");
    CHECK(detail::format_concat("test ", 123, " ", false, true) == "test 123 falsetrue");
    int *p = (int*) 0xfff;
    int *null = nullptr;
    CHECK(detail::format_concat("test ", p) == "test 0xfff");
    CHECK(detail::format_concat("test ", null) == "test (null)");
    CHECK(detail::format_concat("$", test_struct{1, 2}, "$") == "$(1,2)$");

    char *t = nullptr;
    CHECK(detail::format_concat(t, "$") == "(null)$");
}

TEST_CASE("wf::log::log_plain()")
{
    using namespace wf::log;
    std::stringstream out;

    auto check_line = [&out] (std::string expect)
    {
        std::string line;
        std::getline(out, line);

        /* Remove date and current time, because it isn't reproducible. */
        int time_start_index = 2;
        int time_length = 1 + 8 + 1 + 12; /* space + date + space + time */

        REQUIRE(line.length() >= time_start_index + time_length);
        line.erase(time_start_index, time_length);

        CHECK(line == expect);
    };

    initialize_logging(out, LOG_LEVEL_DEBUG, LOG_COLOR_MODE_OFF, "/test/strip/");

    log_plain(LOG_LEVEL_DEBUG, "Test log", "/test/strip/main.cpp", 5);
    check_line("DD - [main.cpp:5] Test log");

    log_plain(LOG_LEVEL_INFO, "Test log", "/test/strip/main.cpp", 56789);
    check_line("II - [main.cpp:56789] Test log");

    log_plain(LOG_LEVEL_WARN, "Test log", "test/strip/main.cpp", 5);
    check_line("WW - [test/strip/main.cpp:5] Test log");

    log_plain(LOG_LEVEL_ERROR, "Test error", "/test/strip//test/strip/main.cpp", 5);
    check_line("EE - [/test/strip/main.cpp:5] Test error");

    initialize_logging(out, LOG_LEVEL_ERROR, LOG_COLOR_MODE_OFF, "/test/strip/");

    /* Ignore non-error messages */
    log_plain(LOG_LEVEL_WARN, "Test log", "test/strip/main.cpp", 5);
    log_plain(LOG_LEVEL_DEBUG, "Test log", "/test/strip/main.cpp", 5);
    log_plain(LOG_LEVEL_INFO, "Test log", "/test/strip/main.cpp", 56789);
    /* Show just errors */
    log_plain(LOG_LEVEL_ERROR, "Test error", "main.cpp", 5);
    check_line("EE - [main.cpp:5] Test error");

    /* Stream shouldn't have any more characters */
    char dummy; out >> dummy;
    CHECK(out.eof());
}
