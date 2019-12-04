#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <algorithm>
#include <wayfire/config/config-manager.hpp>
#include <wayfire/config/types.hpp>

TEST_CASE("wf::config::config_manager_t")
{
    using namespace wf;
    using namespace wf::config;

    config_manager_t config{};
    auto expect_sections = [&] (std::set<std::string> names)
    {
        auto all = config.get_all_sections();
        CHECK(all.size() == names.size());

        std::set<std::string> present;
        std::transform(all.begin(), all.end(),
            std::inserter(present, present.end()),
            [] (const auto& section) { return section->get_name(); });
        CHECK(present == names);
    };

    expect_sections({});

    CHECK(config.get_option("no_such_option") == nullptr);
    CHECK(config.get_option("section/nonexistent") == nullptr);
    CHECK(config.get_option<int_wrapper_t>("section/nonexist/ent") == nullptr);

    CHECK(config.get_section("FirstSection") == nullptr);
    config.merge_section(std::make_shared<section_t> ("FirstSection"));
    expect_sections({"FirstSection"});

    auto section = config.get_section("FirstSection");
    REQUIRE(section != nullptr);
    CHECK(section->get_name() == "FirstSection");
    CHECK(section->get_registered_options().empty());

    CHECK(config.get_option("FirstSection/FirstOption") == nullptr);

    auto color = color_t::from_string("#FFFF").value();
    auto option = std::make_shared<option_t<color_t>> ("ColorOption", color);
    section->register_new_option(option);

    CHECK(config.get_option<color_t>("FirstSection/ColorOption")->get_value() == color);
    CHECK(config.get_option("FirstSection/ColorOption") == option);

    auto section2 = config.get_section("SecondSection");
    CHECK(section2 == nullptr);

    auto section_overwrite = std::make_shared<section_t> ("FirstSection");
    section_overwrite->register_new_option(
        std::make_shared<option_t<color_t>>(
            "ColorOption", color_t::from_string("#CCCC").value()));
    section_overwrite->register_new_option(
        std::make_shared<option_t<int_wrapper_t>> ("IntOption", 5));

    section2 = std::make_shared<section_t> ("SecondSection");
    section2->register_new_option(
        std::make_shared<option_t<int_wrapper_t>> ("IntOption", 6));

    config.merge_section(section_overwrite);
    CHECK(config.get_section("FirstSection") == section); // do not overwrite
    expect_sections({"FirstSection"});

    auto stored_color_opt =
        config.get_option<color_t>("FirstSection/ColorOption");
    REQUIRE(stored_color_opt != nullptr);
    CHECK(stored_color_opt->get_value_str() == "#CCCCCCCC");

    auto stored_int_opt =
        config.get_option<int_wrapper_t>("FirstSection/IntOption");
    REQUIRE(stored_int_opt);
    CHECK(stored_int_opt->get_value_str() == "5");

    config.merge_section(section2);
    expect_sections({"FirstSection", "SecondSection"});

    stored_int_opt = config.get_option<int_wrapper_t>("FirstSection/IntOption");
    REQUIRE(stored_int_opt);
    CHECK(stored_int_opt->get_value_str() == "5"); // remains same

    stored_int_opt = config.get_option<int_wrapper_t>("SecondSection/IntOption");
    REQUIRE(stored_int_opt);
    CHECK(stored_int_opt->get_value_str() == "6");
}
