#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <wayfire/config/option.hpp>
#include <wayfire/config/types.hpp>
#include <linux/input-event-codes.h>

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
            decltype(&V::template set_minimum<>),
            decltype(&V::get_maximum),
            decltype(&V::get_minimum)) { return true; }

    template<class V>
        static constexpr bool has_bounds(...) { return false; }
  public:
    enum {
        value = has_bounds<U> (nullptr, nullptr, nullptr, nullptr),
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
    CHECK(opt.set_value_str("<super>KEY_T"));
    CHECK(opt.get_value() == binding2);
    CHECK(!opt.set_value_str("garbage"));
    CHECK(opt.get_value() == binding2);
    opt.set_value_str("<super>KEY_T");
    CHECK(opt.get_value() == binding2);
    opt.reset_to_default();
    CHECK(opt.get_value() == binding1);

    CHECK(wf::option_type::from_string<wf::keybinding_t>(opt.get_value_str()).value() == binding1);

    CHECK(opt.set_default_value_str("<super>KEY_T"));
    opt.reset_to_default();
    CHECK(opt.get_value() == binding2);
    CHECK(opt.get_default_value() == binding2);
    CHECK(wf::option_type::from_string<wf::keybinding_t>(opt.get_default_value_str()) == binding2);

    CHECK(are_bounds_enabled<option_t<wf::keybinding_t>>::value == false);
    CHECK(are_bounds_enabled<option_t<wf::buttonbinding_t>>::value == false);
    CHECK(are_bounds_enabled<option_t<wf::touchgesture_t>>::value == false);
    CHECK(are_bounds_enabled<option_t<std::string>>::value == false);

    int callback_called = 0, clone_callback_called = 0;
    wf::config::option_base_t::updated_callback_t
    callback = [&] () { callback_called++; }, clone_callback = [&] () { clone_callback_called++; };
    opt.add_updated_handler(&callback);
    auto clone = std::static_pointer_cast<option_t<wf::keybinding_t>>(opt.clone_option());
    CHECK(clone->get_name() == opt.get_name());
    CHECK(clone->get_default_value() == opt.get_default_value());
    CHECK(clone->get_value() == opt.get_value());
    opt.set_value_str("<super>KEY_F");
    CHECK(callback_called == 1);
    clone->add_updated_handler(&clone_callback);
    clone->set_value_str("<super>KEY_F");
    CHECK(callback_called == 1);
    CHECK(clone_callback_called == 1);
}

TEST_CASE("wf::config::option_t<boundable>")
{
    using namespace wf;
    using namespace wf::config;

    option_t<int> iopt{"int123", 5};
    CHECK(iopt.get_name() == "int123");
    CHECK(iopt.get_value() == 5);
    CHECK(!iopt.get_minimum());
    CHECK(!iopt.get_maximum());

    iopt.set_minimum(0);
    CHECK(!iopt.get_maximum());
    CHECK(iopt.get_minimum().value_or(0) == 0);

    iopt.set_maximum(10);
    CHECK(iopt.get_maximum().value_or(11) == 10);
    CHECK(iopt.get_minimum().value_or(1) == 0);

    CHECK(iopt.get_value() == 5);
    iopt.set_value(8);
    CHECK(iopt.get_value() == 8);
    iopt.set_value(11);
    CHECK(iopt.get_value() == 10);
    iopt.set_value(-1);
    CHECK(iopt.get_value() == 0);
    iopt.set_minimum(3);
    CHECK(iopt.get_minimum().value_or(0) == 3);
    CHECK(iopt.get_value() == 3);
    iopt.reset_to_default();
    CHECK(iopt.get_value() == 5);
    CHECK(wf::option_type::from_string<int>(iopt.get_value_str()).value_or(0) == 5);

    option_t<double> dopt{"dbl123", -1.0};
    dopt.set_value(-100);
    CHECK(dopt.get_value() == doctest::Approx(-100.0));
    dopt.set_minimum(50);
    dopt.set_maximum(50);
    CHECK(dopt.get_value() == doctest::Approx(50));
    CHECK(dopt.get_minimum().value_or(60) == doctest::Approx(50));
    CHECK(dopt.get_maximum().value_or(60) == doctest::Approx(50));
    CHECK(wf::option_type::from_string<double>(dopt.get_value_str()).value_or(0) ==
        doctest::Approx(50));

    dopt.set_maximum(60);
    CHECK(dopt.set_default_value_str("55"));
    CHECK(!dopt.set_default_value_str("invalid dobule"));
    dopt.reset_to_default();
    CHECK(dopt.get_value() == doctest::Approx(55));
    CHECK(dopt.get_default_value() == doctest::Approx(55));

    CHECK(dopt.set_default_value_str("75")); // invalid wrt min/max
    dopt.reset_to_default();
    CHECK(dopt.get_value() == doctest::Approx(60)); // not more than max

    auto clone = std::static_pointer_cast<option_t<double>>(dopt.clone_option());
    CHECK(clone->get_minimum() == dopt.get_minimum());
    CHECK(clone->get_maximum() == dopt.get_maximum());

    CHECK(are_bounds_enabled<option_t<int>>::value);
    CHECK(are_bounds_enabled<option_t<double>>::value);
}
