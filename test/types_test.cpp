#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <wayfire/config/types.hpp>
#include <linux/input-event-codes.h>
#include <limits>

#define WF_CONFIG_DOUBLE_EPS 0.01

TEST_CASE("wf::int_wrapper_t")
{
    CHECK(wf::int_wrapper_t::from_string("456").value() == 456);
    CHECK(wf::int_wrapper_t::from_string("-89").value() == -89);

    int32_t max = std::numeric_limits<int32_t>::max();
    int32_t min = std::numeric_limits<int32_t>::min();
    CHECK(wf::int_wrapper_t::from_string(std::to_string(max)).value() == max);
    CHECK(wf::int_wrapper_t::from_string(std::to_string(min)).value() == min);

    CHECK(!wf::int_wrapper_t::from_string("1e4"));
    CHECK(!wf::int_wrapper_t::from_string(""));
    CHECK(!wf::int_wrapper_t::from_string("1234567890000"));

    using wiw = wf::int_wrapper_t;
    CHECK(wiw::from_string(wiw::to_string(456)).value() == 456);
    CHECK(wiw::from_string(wiw::to_string(0)).value() == 0);
}

TEST_CASE("wf::double_wrapper_t")
{
    CHECK(wf::double_wrapper_t::from_string("0.378").value() ==
        doctest::Approx(0.378));
    CHECK(wf::double_wrapper_t::from_string("-89.1847").value() ==
        doctest::Approx(-89.1847));

    double max = std::numeric_limits<double>::max();
    double min = std::numeric_limits<double>::min();
    CHECK(wf::double_wrapper_t::from_string(std::to_string(max)).value()
        == doctest::Approx(max));
    CHECK(wf::double_wrapper_t::from_string(std::to_string(min)).value()
        == doctest::Approx(min));

    CHECK(!wf::double_wrapper_t::from_string("1u4"));
    CHECK(!wf::double_wrapper_t::from_string(""));
    CHECK(!wf::double_wrapper_t::from_string("abc"));

    using wd = wf::double_wrapper_t;
    CHECK(wd::from_string(wd::to_string(-4.56)).value() == doctest::Approx(-4.56));
    CHECK(wd::from_string(wd::to_string(0.0)).value() == doctest::Approx(0));
}

static void check_color_equals(const wf::color_t& color,
    double r, double g, double b, double a)
{
    CHECK(color.r == doctest::Approx(r).epsilon(0.01));
    CHECK(color.g == doctest::Approx(g).epsilon(0.01));
    CHECK(color.b == doctest::Approx(b).epsilon(0.01));
    CHECK(color.a == doctest::Approx(a).epsilon(0.01));
}

static void check_color_equals(
    const std::experimental::optional<wf::color_t>& color,
    double r, double g, double b, double a)
{
    check_color_equals(color.value(), r, g, b, a);
    CHECK(color == wf::color_t{r, g, b, a});
}

/* Test that various wf::color_t constructors work */
TEST_CASE("wf::color_t")
{
    check_color_equals(wf::color_t{}, 0, 0, 0, 0);
    check_color_equals(wf::color_t{0.345, 0.127, 0.188, 1.0}, 0.345, 0.127, 0.188, 1.0);
    check_color_equals(wf::color_t{glm::vec4(0.7)}, 0.7, 0.7, 0.7, 0.7);

    check_color_equals(wf::color_t::from_string("#66CC5EF7"),
        0.4, 0.8, 0.3686274, 0.9686274);
    check_color_equals(wf::color_t::from_string("#0F0F"), 0, 1, 0, 1);

    CHECK(!wf::color_t::from_string("#FFF"));
    CHECK(!wf::color_t::from_string("0C1A"));
    CHECK(!wf::color_t::from_string(""));
    CHECK(!wf::color_t::from_string("#ZYXUIOPQ"));
    CHECK(!wf::color_t::from_string("#AUIO")); // invalid color

    using wc_t = wf::color_t;
    CHECK(wc_t::to_string(wc_t{0, 0, 0, 0}) == "#00000000");
    CHECK(wc_t::to_string(wc_t{0.4, 0.8, 0.3686274, 0.9686274}) == "#66CC5EF7");
    CHECK(wc_t::to_string(wc_t{1, 1, 1, 1}) == "#FFFFFFFF");
}

TEST_CASE("wf::keybinding_t")
{
    /* Test simple constructor */
    const uint32_t modifier1 =
        wf::KEYBOARD_MODIFIER_ALT |wf::KEYBOARD_MODIFIER_LOGO;
    wf::keybinding_t binding1{modifier1, KEY_L};
    CHECK(binding1.get_modifiers() == modifier1);
    CHECK(binding1.get_key() == KEY_L);

    /* Test parsing */
    auto binding2 = wf::keybinding_t::from_string(
        "<shift><ctrl>KEY_TAB").value();
    uint32_t modifier2 =
        wf::KEYBOARD_MODIFIER_SHIFT | wf::KEYBOARD_MODIFIER_CTRL;
    CHECK(binding2.get_modifiers() == modifier2);
    CHECK(binding2.get_key() == KEY_TAB);

    auto binding3 =
        wf::keybinding_t::from_string("<alt><super>KEY_L").value();
    CHECK(binding3.get_modifiers() == modifier1);
    CHECK(binding3.get_key() == KEY_L);

    wf::keybinding_t mod_binding = {wf::KEYBOARD_MODIFIER_LOGO, 0};
    CHECK(wf::keybinding_t::from_string("<super>").value() == mod_binding);

    /* Test invalid bindings */
    CHECK(!wf::keybinding_t::from_string("<invalid>KEY_L"));
    CHECK(!wf::keybinding_t::from_string(""));
    CHECK(!wf::keybinding_t::from_string("<super> KEY_nonexist"));
    CHECK(!wf::keybinding_t::from_string("<alt> BTN_LEFT"));
    CHECK(!wf::keybinding_t::from_string("<alt> super KEY_L"));
    CHECK(!wf::keybinding_t::from_string(("<alt><alt>")));

    /* Test equality */
    CHECK(binding1 == binding3);
    CHECK(!(binding2 == binding1));

    using wk_t = wf::keybinding_t;
    CHECK(wk_t::from_string(wk_t::to_string(binding1)).value() == binding1);
    CHECK(wk_t::from_string(wk_t::to_string(binding2)).value() == binding2);
}

TEST_CASE("wf::buttonbinding_t")
{
    /* Test simple constructor */
    wf::buttonbinding_t binding1{wf::KEYBOARD_MODIFIER_ALT, BTN_LEFT};
    CHECK(binding1.get_modifiers() == wf::KEYBOARD_MODIFIER_ALT);
    CHECK(binding1.get_button() == BTN_LEFT);

    /* Test parsing */
    auto binding2 =
        wf::buttonbinding_t::from_string("<ctrl>BTN_EXTRA").value();
    CHECK(binding2.get_modifiers() == wf::KEYBOARD_MODIFIER_CTRL);
    CHECK(binding2.get_button() == BTN_EXTRA);

    auto binding3 =
        wf::buttonbinding_t::from_string("<alt>BTN_LEFT").value();
    CHECK(binding3.get_modifiers() == wf::KEYBOARD_MODIFIER_ALT);
    CHECK(binding3.get_button() == BTN_LEFT);

    /* Test invalid bindings */
    CHECK(!wf::buttonbinding_t::from_string("<super> BTN_inv"));
    CHECK(!wf::buttonbinding_t::from_string("<super> KEY_E"));
    CHECK(!wf::buttonbinding_t::from_string(""));
    CHECK(!wf::buttonbinding_t::from_string("<super>"));
    CHECK(!wf::buttonbinding_t::from_string("super BTN_LEFT"));

    /* Test equality */
    CHECK(binding1 == binding3);
    CHECK(!(binding2 == binding1));

    using wb_t = wf::buttonbinding_t;
    CHECK(wb_t::from_string(wb_t::to_string(binding1)).value() == binding1);
    CHECK(wb_t::from_string(wb_t::to_string(binding2)).value() == binding2);
    CHECK(wb_t::from_string(wb_t::to_string(binding3)).value() == binding3);
}

TEST_CASE("wf::touchgesture_t")
{
    /* Test simple constructor */
    wf::touchgesture_t binding1{wf::GESTURE_TYPE_SWIPE,
        wf::GESTURE_DIRECTION_UP, 3};
    CHECK(binding1.get_type() == wf::GESTURE_TYPE_SWIPE);
    CHECK(binding1.get_finger_count() == 3);
    CHECK(binding1.get_direction() == wf::GESTURE_DIRECTION_UP);

    /* Test parsing */
    auto binding2 =
        wf::touchgesture_t::from_string("swipe up-left 4").value();
    uint32_t direction2 = wf::GESTURE_DIRECTION_UP | wf::GESTURE_DIRECTION_LEFT;
    CHECK(binding2.get_type() == wf::GESTURE_TYPE_SWIPE);
    CHECK(binding2.get_direction() == direction2);
    CHECK(binding2.get_finger_count() == 4);

    auto binding3 =
        wf::touchgesture_t::from_string("edge-swipe down 2").value();
    CHECK(binding3.get_type() == wf::GESTURE_TYPE_EDGE_SWIPE);
    CHECK(binding3.get_direction() == wf::GESTURE_DIRECTION_DOWN);
    CHECK(binding3.get_finger_count() == 2);

    auto binding4 =
        wf::touchgesture_t::from_string("pinch in 3").value();
    CHECK(binding4.get_type() == wf::GESTURE_TYPE_PINCH);
    CHECK(binding4.get_direction() == wf::GESTURE_DIRECTION_IN);
    CHECK(binding4.get_finger_count() == 3);

    auto binding5 =
        wf::touchgesture_t::from_string("pinch out 2").value();
    CHECK(binding5.get_type() == wf::GESTURE_TYPE_PINCH);
    CHECK(binding5.get_direction() == wf::GESTURE_DIRECTION_OUT);
    CHECK(binding5.get_finger_count() == 2);

    /* A few bad description cases */
    CHECK(!wf::touchgesture_t::from_string("pinch out")); // missing fingercount
    CHECK(!wf::touchgesture_t::from_string("wrong left 5")); // no such type
    CHECK(!wf::touchgesture_t::from_string("edge-swipe up-down 3")); // opposite dirs
    CHECK(!wf::touchgesture_t::from_string("swipe 3")); // missing dir
    CHECK(!wf::touchgesture_t::from_string("pinch 3"));

    /* Equality */
    CHECK(!(binding1 == wf::touchgesture_t{wf::GESTURE_TYPE_PINCH, 0, 3}));
    CHECK(binding1 == wf::touchgesture_t{wf::GESTURE_TYPE_SWIPE, 0, 3});
    CHECK(binding3 == wf::touchgesture_t{
        wf::GESTURE_TYPE_EDGE_SWIPE, wf::GESTURE_DIRECTION_DOWN, 2});
    CHECK(binding5 == wf::touchgesture_t{
        wf::GESTURE_TYPE_PINCH, wf::GESTURE_DIRECTION_OUT, 2});
    CHECK(binding5 == wf::touchgesture_t{wf::GESTURE_TYPE_PINCH, 0, 2});
    CHECK(!(binding5 == wf::touchgesture_t{
        wf::GESTURE_TYPE_PINCH, wf::GESTURE_DIRECTION_IN, 2}));

    using wt_t = wf::touchgesture_t;
    CHECK(wt_t::from_string(wt_t::to_string(binding1)).value() == binding1);
    CHECK(wt_t::from_string(wt_t::to_string(binding2)).value() == binding2);
    CHECK(wt_t::from_string(wt_t::to_string(binding3)).value() == binding3);
    CHECK(wt_t::from_string(wt_t::to_string(binding4)).value() == binding4);
    CHECK(wt_t::from_string(wt_t::to_string(binding5)).value() == binding5);
}
