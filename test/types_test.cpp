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

TEST_CASE("wf::keybinding_t")
{
    /* Test simple constructor */
    const uint32_t modifier1 =
        wf::KEYBOARD_MODIFIER_ALT |wf::KEYBOARD_MODIFIER_LOGO;
    wf::keybinding_t binding1{modifier1, KEY_L};
    CHECK(binding1.get_modifiers() == modifier1);
    CHECK(binding1.get_key() == KEY_L);

    /* Test parsing */
    wf::keybinding_t binding2{"<shift><ctrl>KEY_TAB"};
    uint32_t modifier2 =
        wf::KEYBOARD_MODIFIER_SHIFT | wf::KEYBOARD_MODIFIER_CTRL;
    CHECK(binding2.get_modifiers() == modifier2);
    CHECK(binding2.get_key() == KEY_TAB);

    wf::keybinding_t binding3{"<alt><super>KEY_L"};
    CHECK(binding3.get_modifiers() == modifier1);
    CHECK(binding3.get_key() == KEY_L);

    /* Test invalid bindings */
    wf::keybinding_t binding4{"<invalid>KEY_L"};
    CHECK(binding4.get_modifiers() == 0);
    CHECK(binding4.get_key() == 0);

    wf::keybinding_t binding5{"<super> KEY_nonexist"};
    CHECK(binding5.get_modifiers() == 0);
    CHECK(binding5.get_key() == 0);

    /* Test equality */
    CHECK(binding1 == binding3);
    CHECK(binding4 == binding5);
    CHECK(!(binding2 == binding1));
}

TEST_CASE("wf::buttonbinding_t")
{
    /* Test simple constructor */
    wf::buttonbinding_t binding1{wf::KEYBOARD_MODIFIER_ALT, BTN_LEFT};
    CHECK(binding1.get_modifiers() == wf::KEYBOARD_MODIFIER_ALT);
    CHECK(binding1.get_button() == BTN_LEFT);

    /* Test parsing */
    wf::buttonbinding_t binding2{"<ctrl>BTN_EXTRA"};
    CHECK(binding2.get_modifiers() == wf::KEYBOARD_MODIFIER_CTRL);
    CHECK(binding2.get_button() == BTN_EXTRA);

    wf::buttonbinding_t binding3{"<alt>BTN_LEFT"};
    CHECK(binding3.get_modifiers() == wf::KEYBOARD_MODIFIER_ALT);
    CHECK(binding3.get_button() == BTN_LEFT);

    /* Test invalid bindings */
    wf::buttonbinding_t binding4{"<super> BTN_inv"};
    CHECK(binding4.get_modifiers() == 0);
    CHECK(binding4.get_button() == 0);

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
    wf::touchgesture_t binding2{"swipe up-left 4"};
    uint32_t direction2 = wf::GESTURE_DIRECTION_UP | wf::GESTURE_DIRECTION_LEFT;
    CHECK(binding2.get_type() == wf::GESTURE_TYPE_SWIPE);
    CHECK(binding2.get_direction() == direction2);
    CHECK(binding2.get_finger_count() == 4);

    wf::touchgesture_t binding3{"edge-swipe down 2"};
    CHECK(binding3.get_type() == wf::GESTURE_TYPE_EDGE_SWIPE);
    CHECK(binding3.get_direction() == wf::GESTURE_DIRECTION_DOWN);
    CHECK(binding3.get_finger_count() == 2);

    wf::touchgesture_t binding4{"pinch in 3"};
    CHECK(binding4.get_type() == wf::GESTURE_TYPE_PINCH);
    CHECK(binding4.get_direction() == wf::GESTURE_DIRECTION_IN);
    CHECK(binding4.get_finger_count() == 3);

    wf::touchgesture_t binding5{"pinch out 2"};
    CHECK(binding5.get_type() == wf::GESTURE_TYPE_PINCH);
    CHECK(binding5.get_direction() == wf::GESTURE_DIRECTION_OUT);
    CHECK(binding5.get_finger_count() == 2);

    /* A few bad description cases */
    wf::touchgesture_t binding6{"pinch out"}; // missing fingercount
    CHECK(binding6.get_type() == wf::GESTURE_TYPE_NONE);

    wf::touchgesture_t binding7{"wrong left 5"}; // no such type
    CHECK(binding7.get_type() == wf::GESTURE_TYPE_NONE);

    wf::touchgesture_t binding8{"edge-swipe up-down 3"}; // opposite dirs
    CHECK(binding8.get_type() == wf::GESTURE_TYPE_NONE);

    wf::touchgesture_t binding9{"swipe 3"}; // missing dir
    CHECK(binding9.get_type() == wf::GESTURE_TYPE_NONE);

    wf::touchgesture_t binding10{"pinch 3"};
    CHECK(binding10.get_type() == wf::GESTURE_TYPE_NONE);

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
