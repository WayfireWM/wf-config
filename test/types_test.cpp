#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <config/types.hpp>
#include <linux/input-event-codes.h>

#define WF_CONFIG_DOUBLE_EPS 0.01
static void check_color_equals(const wf::color_t& color,
    double r, double g, double b, double a)
{
    CHECK(color.r == doctest::Approx(r).epsilon(0.01));
    CHECK(color.g == doctest::Approx(g).epsilon(0.01));
    CHECK(color.b == doctest::Approx(b).epsilon(0.01));
    CHECK(color.a == doctest::Approx(a).epsilon(0.01));
}

static void check_color_equals(const wf::optional<wf::color_t>& color,
    double r, double g, double b, double a)
{
    check_color_equals(color.get_unchecked(), r, g, b, a);
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
        "<shift><ctrl>KEY_TAB").get_unchecked();
    uint32_t modifier2 =
        wf::KEYBOARD_MODIFIER_SHIFT | wf::KEYBOARD_MODIFIER_CTRL;
    CHECK(binding2.get_modifiers() == modifier2);
    CHECK(binding2.get_key() == KEY_TAB);

    auto binding3 =
        wf::keybinding_t::from_string("<alt><super>KEY_L").get_unchecked();
    CHECK(binding3.get_modifiers() == modifier1);
    CHECK(binding3.get_key() == KEY_L);

    /* Test invalid bindings */
    CHECK(!wf::keybinding_t::from_string("<invalid>KEY_L"));

    CHECK(!wf::keybinding_t::from_string("<super> KEY_nonexist"));

    /* Test equality */
    CHECK(binding1 == binding3);
    CHECK(!(binding2 == binding1));
}

TEST_CASE("wf::buttonbinding_t")
{
    /* Test simple constructor */
    wf::buttonbinding_t binding1{wf::KEYBOARD_MODIFIER_ALT, BTN_LEFT};
    CHECK(binding1.get_modifiers() == wf::KEYBOARD_MODIFIER_ALT);
    CHECK(binding1.get_button() == BTN_LEFT);

    /* Test parsing */
    auto binding2 =
        wf::buttonbinding_t::from_string("<ctrl>BTN_EXTRA").get_unchecked();
    CHECK(binding2.get_modifiers() == wf::KEYBOARD_MODIFIER_CTRL);
    CHECK(binding2.get_button() == BTN_EXTRA);

    auto binding3 =
        wf::buttonbinding_t::from_string("<alt>BTN_LEFT").get_unchecked();
    CHECK(binding3.get_modifiers() == wf::KEYBOARD_MODIFIER_ALT);
    CHECK(binding3.get_button() == BTN_LEFT);

    /* Test invalid bindings */
    CHECK(!wf::buttonbinding_t::from_string("<super> BTN_inv"));

    /* Test equality */
    CHECK(binding1 == binding3);
    CHECK(!(binding2 == binding1));
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
        wf::touchgesture_t::from_string("swipe up-left 4").get_unchecked();
    uint32_t direction2 = wf::GESTURE_DIRECTION_UP | wf::GESTURE_DIRECTION_LEFT;
    CHECK(binding2.get_type() == wf::GESTURE_TYPE_SWIPE);
    CHECK(binding2.get_direction() == direction2);
    CHECK(binding2.get_finger_count() == 4);

    auto binding3 =
        wf::touchgesture_t::from_string("edge-swipe down 2").get_unchecked();
    CHECK(binding3.get_type() == wf::GESTURE_TYPE_EDGE_SWIPE);
    CHECK(binding3.get_direction() == wf::GESTURE_DIRECTION_DOWN);
    CHECK(binding3.get_finger_count() == 2);

    auto binding4 =
        wf::touchgesture_t::from_string("pinch in 3").get_unchecked();
    CHECK(binding4.get_type() == wf::GESTURE_TYPE_PINCH);
    CHECK(binding4.get_direction() == wf::GESTURE_DIRECTION_IN);
    CHECK(binding4.get_finger_count() == 3);

    auto binding5 =
        wf::touchgesture_t::from_string("pinch out 2").get_unchecked();
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
}
