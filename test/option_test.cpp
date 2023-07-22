#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <wayfire/config/option.hpp>
#include <wayfire/config/compound-option.hpp>
#include <wayfire/config/types.hpp>
#include <linux/input-event-codes.h>
#include <algorithm>
#include "../src/option-impl.hpp"

/**
 * A struct to check whether the maximum and minimum methods are enabled on the
 * template parameter class.
 */
template<class U>
struct are_bounds_enabled
{
  private:
    template<class V>
    static constexpr bool has_bounds(
        decltype(&V::template set_maximum<>),
        decltype(&V::template set_minimum<>),
        decltype(&V::get_maximum),
        decltype(&V::get_minimum))
    {
        return true;
    }

    template<class V>
    static constexpr bool has_bounds(...)
    {
        return false;
    }

  public:
    enum
    {
        value = has_bounds<U>(nullptr,
            nullptr,
            nullptr,
            nullptr),
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

    CHECK(wf::option_type::from_string<wf::keybinding_t>(
        opt.get_value_str()).value() == binding1);

    CHECK(opt.set_default_value_str("<super>KEY_T"));
    opt.reset_to_default();
    CHECK(opt.get_value() == binding2);
    CHECK(opt.get_default_value() == binding2);
    CHECK(wf::option_type::from_string<wf::keybinding_t>(
        opt.get_default_value_str()) == binding2);

    CHECK(are_bounds_enabled<option_t<wf::keybinding_t>>::value == false);
    CHECK(are_bounds_enabled<option_t<wf::buttonbinding_t>>::value == false);
    CHECK(are_bounds_enabled<option_t<wf::touchgesture_t>>::value == false);
    CHECK(are_bounds_enabled<option_t<std::string>>::value == false);

    int callback_called = 0, clone_callback_called = 0;
    wf::config::option_base_t::updated_callback_t
        callback = [&] () { callback_called++; }, clone_callback = [&] ()
    {
        clone_callback_called++;
    };
    opt.add_updated_handler(&callback);
    opt.priv->xml = (xmlNode*)0x123;
    auto clone = std::static_pointer_cast<option_t<wf::keybinding_t>>(
        opt.clone_option());
    CHECK(clone->priv->xml == (xmlNode*)0x123);
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

TEST_CASE("compound options")
{
    using namespace wf;
    using namespace wf::config;

    compound_option_t::entries_t entries;
    entries.push_back(std::make_unique<compound_option_entry_t<int>>("hey_"));
    entries.push_back(std::make_unique<compound_option_entry_t<double>>("bey_"));

    compound_option_t opt{"Test", std::move(entries)};

    auto section = std::make_shared<section_t>("TestSection");
    section->register_new_option(std::make_shared<option_t<int>>("hey_k1", 1));
    section->register_new_option(std::make_shared<option_t<int>>("hey_k2", -12));
    section->register_new_option(std::make_shared<option_t<double>>("bey_k1", 1.2));
    section->register_new_option(std::make_shared<option_t<double>>("bey_k2",
        3.1415));

    // Not fully specified pairs
    section->register_new_option(std::make_shared<option_t<int>>("hey_k3", 3));
    // One of the values is a regular option with an associated XML tag, and
    // needs to be skipped
    auto xml_opt = std::make_shared<option_t<double>>("bey_k3", 5.5);
    xml_opt->priv->xml = (xmlNode*)0x123;
    section->register_new_option(xml_opt);

    section->register_new_option(std::make_shared<option_t<std::string>>("bey_k4",
        "invalid value"));
    section->register_new_option(std::make_shared<option_t<double>>("bey_k5", 3.5));

    // Options which don't match anything
    section->register_new_option(std::make_shared<option_t<double>>("hallo", 3.5));

    // Mark all options as coming from the config file, otherwise, they wont' be
    // parsed
    for (auto& opt : section->get_registered_options())
    {
        opt->priv->option_in_config_file = true;
    }

    update_compound_from_section(opt, section);
    auto values = opt.get_value<int, double>();

    REQUIRE(values.size() == 2);
    std::sort(values.begin(), values.end());

    CHECK(std::get<0>(values[0]) == "k1");
    CHECK(std::get<1>(values[0]) == 1);
    CHECK(std::get<2>(values[0]) == 1.2);

    CHECK(std::get<0>(values[1]) == "k2");
    CHECK(std::get<1>(values[1]) == -12);
    CHECK(std::get<2>(values[1]) == 3.1415);

    std::vector<std::vector<std::string>> untyped_values = {
        {"k1", "1", "1.200000"},
        {"k2", "-12", "3.141500"},
    };

    CHECK(opt.get_value_untyped() == untyped_values);

    compound_list_t<int, double> v = {
        {"k3", 1, 1.23}
    };

    opt.set_value<int, double>(v);
    CHECK(v == opt.get_value<int, double>());

    compound_option_t::stored_type_t v2 = {
        {"k3", "1", "1.23"}
    };

    CHECK(opt.set_value_untyped(v2));
    CHECK(v == opt.get_value<int, double>());

    // Fail to set
    compound_option_t::stored_type_t v3 = {
        {"k3", "1"}
    };
    CHECK(!opt.set_value_untyped(v3));

    compound_option_t::stored_type_t v4 = {
        {"k3", "1", "invalid double"}
    };
    CHECK(!opt.set_value_untyped(v4));
}

TEST_CASE("compound option with default values")
{
    using namespace wf;
    using namespace wf::config;

    compound_option_t::entries_t entries;
    entries.push_back(std::make_unique<compound_option_entry_t<int>>("int_", "int",
        "42"));
    entries.push_back(std::make_unique<compound_option_entry_t<double>>("double_",
        "double"));
    entries.push_back(std::make_unique<compound_option_entry_t<std::string>>("str_",
        "str", "default"));

    compound_option_t opt("Test", std::move(entries));

    auto section = std::make_shared<section_t>("TestSection");
    // k1 -- all entries are scecified
    // k2 -- double value is unspecified (error)
    // k3 -- invalid int value, should use the default one
    // k4 -- only double value is scecified
    section->register_new_option(std::make_shared<option_t<int>>("int_k1", 1));
    section->register_new_option(std::make_shared<option_t<int>>("int_k2", 2));
    section->register_new_option(std::make_shared<option_t<std::string>>("int_k3",
        "invalid"));

    section->register_new_option(std::make_shared<option_t<double>>("double_k1",
        1.0));
    section->register_new_option(std::make_shared<option_t<double>>("double_k3",
        3.0));
    section->register_new_option(std::make_shared<option_t<double>>("double_k4",
        4.0));

    section->register_new_option(std::make_shared<option_t<std::string>>("str_k1",
        "s1"));
    section->register_new_option(std::make_shared<option_t<std::string>>("str_k2",
        "s2"));
    section->register_new_option(std::make_shared<option_t<std::string>>("str_k3",
        "s3"));

    // Mark all options as coming from the config file, otherwise, they wont' be
    // parsed
    for (auto& opt : section->get_registered_options())
    {
        opt->priv->option_in_config_file = true;
    }

    update_compound_from_section(opt, section);
    auto values = opt.get_value<int, double, std::string>();

    REQUIRE(values.size() == 3);
    std::sort(values.begin(), values.end());

    CHECK(values[0] == std::tuple{"k1", 1, 1.0, "s1"});
    CHECK(values[1] == std::tuple{"k3", 42, 3.0, "s3"});
    CHECK(values[2] == std::tuple{"k4", 42, 4.0, "default"});
}

TEST_CASE("Plain list compound options")
{
    using namespace wf::config;

    compound_option_t::entries_t entries;
    entries.push_back(std::make_unique<compound_option_entry_t<int>>("hey_"));
    entries.push_back(std::make_unique<compound_option_entry_t<double>>("bey_"));
    compound_option_t opt{"Test", std::move(entries)};

    simple_list_t<int, double> simple_list = {
        {0, 0.0},
        {1, -1.5}
    };

    opt.set_value_simple(simple_list);
    auto with_names = opt.get_value<int, double>();

    compound_list_t<int, double> compound_list = {
        {"0", 0, 0.0},
        {"1", 1, -1.5}
    };
    CHECK(compound_list == opt.get_value<int, double>());

    compound_list = {
        {"test", 10, 0.0},
        {"blah", 20, 15.6}
    };
    opt.set_value(compound_list);

    simple_list = {
        {10, 0.0},
        {20, 15.6}
    };

    CHECK(simple_list == opt.get_value_simple<int, double>());
}
