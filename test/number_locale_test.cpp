#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <wayfire/config/types.hpp>
#include <linux/input-event-codes.h>
#include <limits>

#include <locale>

#define WF_CONFIG_DOUBLE_EPS 0.01

using namespace wf;
using namespace wf::option_type;


void setup_test_locale()
{
    // requires glibc-langpack-de
    // changes std::to_string output
    std::setlocale(LC_NUMERIC, "fr_FR.UTF-8");

}

TEST_CASE("wf::option_type::to_string<double>")
{
    setup_test_locale();

    std::string str = wf::option_type::to_string(3.14);
    CHECK(str == "3.140000");
    CHECK(wf::option_type::to_string(3140) == "3140");
}

TEST_CASE("wf::bool_wrapper_t locale")
{
    setup_test_locale();

    setup_test_locale();
    CHECK(from_string<bool>("True").value());
    setup_test_locale();
    CHECK(from_string<bool>("true").value());
    setup_test_locale();
    CHECK(from_string<bool>("1").value());

    setup_test_locale();
    CHECK(from_string<bool>("False").value() == false);
    setup_test_locale();
    CHECK(from_string<bool>("false").value() == false);
    setup_test_locale();
    CHECK(from_string<bool>("0").value() == false);
    setup_test_locale();

    setup_test_locale();
    CHECK(!from_string<bool>("vrai"));
    setup_test_locale();
    CHECK(!from_string<bool>("faux"));
    setup_test_locale();

    setup_test_locale();
    CHECK(from_string<bool>(to_string<bool>(true)).value());
    setup_test_locale();
    CHECK(from_string<bool>(to_string<bool>(false)).value() == false);
    setup_test_locale();
}

TEST_CASE("wf::int_wrapper_t locale")
{
    setup_test_locale();

    setup_test_locale();
    CHECK(from_string<int>("1456").value() == 1456);
    setup_test_locale();
    CHECK(from_string<int>("-89").value() == -89);

    int32_t max = std::numeric_limits<int32_t>::max();
    int32_t min = std::numeric_limits<int32_t>::min();
    setup_test_locale();
    CHECK(from_string<int>(wf::option_type::to_string(max)).value() == max);
    setup_test_locale();
    CHECK(from_string<int>(wf::option_type::to_string(min)).value() == min);

    setup_test_locale();
    CHECK(!from_string<int>("1e4"));
    setup_test_locale();
    CHECK(!from_string<int>(""));
    setup_test_locale();
    CHECK(!from_string<int>("1234567890000"));

    setup_test_locale();
    CHECK(from_string<int>(to_string<int>(456)).value() == 456);
    setup_test_locale();
    CHECK(from_string<int>(to_string<int>(0)).value() == 0);
}

TEST_CASE("wf::double_wrapper_t locale")
{
    setup_test_locale();
    CHECK(from_string<double>("0.378000").value() == doctest::Approx(0.378));
    setup_test_locale();
    CHECK(from_string<double>("-89.1847").value() == doctest::Approx(-89.1847));

    double max = std::numeric_limits<double>::max();
    double min = std::numeric_limits<double>::min();

    setup_test_locale();
    CHECK(from_string<double>(wf::option_type::to_string(max)).value() == doctest::Approx(max));
    setup_test_locale();
    CHECK(from_string<double>(wf::option_type::to_string(min)).value() == doctest::Approx(min));

    setup_test_locale();
    CHECK(!from_string<double>("1u4"));
    setup_test_locale();
    CHECK(!from_string<double>(""));
    setup_test_locale();
    CHECK(!from_string<double>("abc"));

    setup_test_locale();
    CHECK(from_string<double>(to_string<double>(-4.56)).value() ==
        doctest::Approx(-4.56));
    setup_test_locale();
    CHECK(from_string<double>(to_string<double>(0.0)).value() == doctest::Approx(0));
}

/* Test that various wf::color_t constructors work */
TEST_CASE("wf::color_t")
{
    using namespace wf;
    using namespace option_type;

    setup_test_locale();

    setup_test_locale();
    CHECK(!from_string<color_t>("#FFF"));
    setup_test_locale();
    CHECK(!from_string<color_t>("0C1A"));
    setup_test_locale();
    CHECK(!from_string<color_t>(""));
    setup_test_locale();
    CHECK(!from_string<color_t>("#ZYXUIOPQ"));
    setup_test_locale();
    CHECK(!from_string<color_t>("#AUIO")); // invalid color
    setup_test_locale();
    CHECK(!from_string<color_t>("1.0 0.5 0.5 1.0 1.0")); // invalid color
    setup_test_locale();
    CHECK(!from_string<color_t>("1.0 0.5 0.5 1.0 asdf")); // invalid color
    setup_test_locale();
    CHECK(!from_string<color_t>("1.0 0.5")); // invalid color

    setup_test_locale();
    CHECK(to_string<color_t>(color_t{0, 0, 0, 0}) == "#00000000");
    setup_test_locale();
    CHECK(to_string<color_t>(color_t{0.4, 0.8, 0.3686274,
        0.9686274}) == "#66CC5EF7");
    setup_test_locale();
    CHECK(to_string<color_t>(color_t{1, 1, 1, 1}) == "#FFFFFFFF");
}
