#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <iostream>
#include "doctest.h"

#include <wayfire/config/file.hpp>
#include <wayfire/util/log.hpp>
#include <wayfire/config/types.hpp>

const std::string contents = R"(
illegal_option = value

[section1]
option1 = value1
option2=3
#Comment
option3         = value value value      # Comment



[section2]
option1 = value 4 \
value # Ignore
option2 = value \\
# Ignore
option3 = \#No way

[wrongsection
option1
)";

#include "expect_line.hpp"

TEST_CASE("wf::config::load_configuration_options_from_string")
{
    std::stringstream log;
    wf::log::initialize_logging(log, wf::log::LOG_LEVEL_DEBUG,
        wf::log::LOG_COLOR_MODE_OFF);

    using namespace wf;
    using namespace wf::config;
    config_manager_t config;

    /* Create the first section and add an option there */
    auto section = std::make_shared<section_t> ("section1");
    section->register_new_option(
        std::make_shared<option_t<int_wrapper_t>> ("option1", 10));
    section->register_new_option(
        std::make_shared<option_t<int_wrapper_t>> ("option2", 5));
    section->register_new_option(
        std::make_shared<option_t<string_wrapper_t>> ("option4", std::string("option4")));

    config.merge_section(section);
    load_configuration_options_from_string(config, contents, "test");

    REQUIRE(config.get_section("section1"));
    REQUIRE(config.get_section("section2"));
    CHECK(config.get_section("wrongsection") == nullptr);

    auto s1 = config.get_section("section1");
    auto s2 = config.get_section("section2");

    CHECK(s1->get_option("option1")->get_value_str() == "10");
    CHECK(s1->get_option("option2")->get_value_str() == "3");
    CHECK(s1->get_option("option3")->get_value_str() == "value value value");
    CHECK(s1->get_option("option4")->get_value_str() == "option4");

    CHECK(s2->get_option("option1")->get_value_str() == "value 4 value");
    CHECK(s2->get_option("option2")->get_value_str() == "value \\");
    CHECK(s2->get_option("option3")->get_value_str() == "#No way");
    CHECK(!s2->get_option_or("Ignored"));

    EXPECT_LINE(log, "Error in file test:2");
    EXPECT_LINE(log, "Error in file test:5");
    EXPECT_LINE(log, "Error in file test:19");
    EXPECT_LINE(log, "Error in file test:20");
}

TEST_CASE("wf::config::save_configuration_options_to_string")
{
    using namespace wf;
    using namespace wf::config;
    auto section1 = std::make_shared<section_t> ("section1");
    auto section2 = std::make_shared<section_t> ("section2");

    section1->register_new_option(std::make_shared<option_t<int_wrapper_t>> ("option1", 4));
    section1->register_new_option(std::make_shared<option_t<string_wrapper_t>> ("option2", std::string("45 # 46 \\")));
    section2->register_new_option(std::make_shared<option_t<double_wrapper_t>> ("option1", 4.25));

    config_manager_t config;
    config.merge_section(section1);
    config.merge_section(section2);

    auto stringified = save_configuration_options_to_string(config);
    CHECK(stringified ==
R"([section1]
option1 = 4
option2 = 45 \# 46 \\

[section2]
option1 = 4.250000

)");

}
