#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <config/types.hpp>

#define WF_CONFIG_DOUBLE_EPS 0.01
static void check_color_equals(const wf::color_t& color,
    double r, double g, double b, double a)
{
    CHECK(color.r == doctest::Approx(r).epsilon(0.01));
    CHECK(color.g == doctest::Approx(g).epsilon(0.01));
    CHECK(color.b == doctest::Approx(b).epsilon(0.01));
    CHECK(color.a == doctest::Approx(a).epsilon(0.01));
}

/* Test that various wf::color_t constructors work */
TEST_CASE("wf::color_t")
{
    check_color_equals(wf::color_t{}, 0, 0, 0, 0);
    check_color_equals(wf::color_t{0.345, 0.127, 0.188, 1.0}, 0.345, 0.127, 0.188, 1.0);
    check_color_equals(wf::color_t{glm::vec4(0.7)}, 0.7, 0.7, 0.7, 0.7);
    check_color_equals(wf::color_t{"#66CC5EF7"}, 0.4, 0.8, 0.3686274, 0.9686274);
    check_color_equals(wf::color_t{"#0F0F"}, 0, 1, 0, 1);
    check_color_equals(wf::color_t{"#AUIO"}, 0, 0, 0, 0); // invalid color

    CHECK(wf::color_t::is_valid("#FFF") == false);
    CHECK(wf::color_t::is_valid("#0A4B7D9F") == true);
    CHECK(wf::color_t::is_valid("0C1A") == false);
    CHECK(wf::color_t::is_valid("") == false);
    CHECK(wf::color_t::is_valid("#0C1A") == true);
    CHECK(wf::color_t::is_valid("#ZYXUIOPQ") == false);
}
