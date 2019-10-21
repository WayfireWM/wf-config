#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <wayfire/config/option.hpp>
#include <wayfire/config/types.hpp>
#include <linux/input-event-codes.h>

TEST_CASE("wf::config::detail::is_wrapped_comparable_type")
{
    using namespace wf;
    using namespace wf::config::detail;

    CHECK(is_wrapped_comparable_type<int_wrapper_t>::value);
    CHECK(is_wrapped_comparable_type<double_wrapper_t>::value);

    CHECK(!is_wrapped_comparable_type<int>::value);
    CHECK(!is_wrapped_comparable_type<string_wrapper_t>::value);
    CHECK(!is_wrapped_comparable_type<keybinding_t>::value);
}

/**
 * A struct to check whether the maximum and minimum methods are enabled on the
 * template parameter class.
 */
template<class U> struct are_bounds_enabled
{
  private:
    template<class V>
        static constexpr bool has_bounds(
            decltype(&V::template set_maximum<>),
            decltype(&V::template set_minimum<>)) { return true; }

    template<class V>
        static constexpr bool has_bounds(...) { return false; }
  public:
    enum {
        value = has_bounds<U> (nullptr, nullptr),
    };
};

TEST_CASE("wf::config::option_t<unboundable>")
{
    using namespace wf;
    using namespace wf::config;

    const wf::keybinding_t binding1{KEYBOARD_MODIFIER_ALT, KEY_E};
    const wf::keybinding_t binding2{KEYBOARD_MODIFIER_LOGO, KEY_T};

    option_t<wf::keybinding_t> opt("test123", binding1);
    CHECK(opt.get_name() == "test123");
    CHECK(opt.get_value() == binding1);

    opt.set_value(binding2);
    CHECK(opt.get_value() == binding2);

    opt.set_value(binding1);
    CHECK(opt.get_value() == binding1);
    opt.set_value("<super>KEY_T");
    CHECK(opt.get_value() == binding2);
    opt.set_value("garbage");
    CHECK(opt.get_value() == binding1); // default value

    CHECK(are_bounds_enabled<option_t<wf::keybinding_t>>::value == false);
    CHECK(are_bounds_enabled<option_t<wf::buttonbinding_t>>::value == false);
    CHECK(are_bounds_enabled<option_t<wf::touchgesture_t>>::value == false);
    CHECK(are_bounds_enabled<option_t<wf::string_wrapper_t>>::value == false);
}

TEST_CASE("wf::config::option_t<boundable>")
{
    using namespace wf;
    using namespace wf::config;

    option_t<int_wrapper_t> iopt{"int123", 5};
    CHECK(iopt.get_name() == "int123");
    CHECK(iopt.get_value() == 5);

    iopt.set_minimum(0);
    iopt.set_maximum(10);
    CHECK(iopt.get_value() == 5);
    iopt.set_value(8);
    CHECK(iopt.get_value() == 8);
    iopt.set_value(11);
    CHECK(iopt.get_value() == 10);
    iopt.set_value(-1);
    CHECK(iopt.get_value() == 0);
    iopt.set_minimum(3);
    CHECK(iopt.get_value() == 3);

    option_t<double_wrapper_t> dopt{"dbl123", -1.0};
    dopt.set_value(-100);
    CHECK(dopt.get_value() == doctest::Approx(-100.0));
    dopt.set_minimum(50);
    dopt.set_maximum(50);
    CHECK(dopt.get_value() == doctest::Approx(50));

    CHECK(are_bounds_enabled<option_t<int_wrapper_t>>::value);
    CHECK(are_bounds_enabled<option_t<double_wrapper_t>>::value);
}
