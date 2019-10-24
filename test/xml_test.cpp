#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include <set>

#include <sstream>
#include <wayfire/config/types.hpp>
#include <wayfire/config/xml.hpp>
#include <wayfire/util/log.hpp>
#include <linux/input-event-codes.h>

static const std::string xml_option_int = R"(
<option name="IntOption" type="int">
<default>3</default>
<min>0</min>
<max>10</max>
</option>
)";

static const std::string xml_option_key = R"(
<option name="KeyOption" type="key">
<default>&lt;super&gt; KEY_E</default>
</option>
)";

static const std::string xml_option_bad_tag = R"(
<invalid name="KeyOption" type="key">
<default>&lt;super&gt; KEY_E</default>
</invalid>
)";

static const std::string xml_option_int_bad_min = R"(
<option name="IntOption" type="int">
<default>3</default>
<min>sfd</min>
<max>10</max>
</option>
)";

static const std::string xml_option_int_bad_max = R"(
<option name="IntOption" type="int">
<default>3</default>
<min>0</min>
<max>sdf</max>
</option>
)";

static const std::string xml_option_bad_type = R"(
<option name="KeyOption" type="unknown">
<default>&lt;super&gt; KEY_E</default>
</option>
)";

static const std::string xml_option_bad_default = R"(
<option name="KeyOption" type="key">
<default>&lt;super&gt; e</default>
</option>
)";


static const std::string xml_option_missing_name = R"(
<option type="int">
</option>
)";

static const std::string xml_option_missing_type = R"(
<option name="IntOption">
</option>
)";

static const std::string xml_option_missing_default_value = R"(
<option name="IntOption" type="int">
</option>
)";

auto EXPECT_LINE = [] (std::istream& log, std::string expect)
{
    auto tolower = [] (std::string s)
    {
        for (auto& c : s)
            c = std::tolower(c);
        return s;
    };

    bool found = false;

    std::string line;
    while (std::getline(log, line))
    {
        /* Case-insensitive matching */
        line = tolower(line);
        expect = tolower(expect);
        found |= (line.find(expect) != std::string::npos);
    }

    CHECK(found);
};

TEST_CASE("wf::config::xml::create_option")
{
    std::stringstream log;
    wf::log::initialize_logging(log,
        wf::log::LOG_LEVEL_DEBUG, wf::log::LOG_COLOR_MODE_OFF);

    namespace wxml = wf::config::xml;
    namespace wc = wf::config;

    auto initialize_option = [] (std::string source)
    {
        auto node = xmlParseDoc((const xmlChar*)source.c_str());
        REQUIRE(node != nullptr);
        return wxml::create_option_from_xml_node(xmlDocGetRootElement(node));
    };

    SUBCASE("IntOption")
    {
        auto option = initialize_option(xml_option_int);
        REQUIRE(option != nullptr);

        CHECK(option->get_name() == "IntOption");

        auto as_int =
            std::dynamic_pointer_cast<wc::option_t<wf::int_wrapper_t>> (option);
        REQUIRE(as_int);
        REQUIRE(as_int->get_minimum());
        REQUIRE(as_int->get_maximum());

        CHECK(as_int->get_value() == 3);
        CHECK(as_int->get_minimum().value() == 0);
        CHECK(as_int->get_maximum().value() == 10);
    }

    SUBCASE("KeyOption")
    {
        auto option = initialize_option(xml_option_key);
        REQUIRE(option != nullptr);

        CHECK(option->get_name() == "KeyOption");

        auto as_key =
            std::dynamic_pointer_cast<wc::option_t<wf::keybinding_t>> (option);
        REQUIRE(as_key);

        CHECK(as_key->get_value() ==
            wf::keybinding_t{wf::KEYBOARD_MODIFIER_LOGO, KEY_E});
    }

    /* Generate a subcase where the given xml source can't be parsed to an
     * option, and check that the output in the log is as expected. */
#define SUBCASE_BAD_OPTION(subcase_name,xml_source,expected_log) \
    SUBCASE(subcase_name) \
    { \
        auto option = initialize_option(xml_source); \
        CHECK(option == nullptr); \
        EXPECT_LINE(log, expected_log); \
    }

    SUBCASE_BAD_OPTION("Invalid xml tag",
        xml_option_bad_tag, "is not an option element");

    SUBCASE_BAD_OPTION("Invalid option type",
        xml_option_bad_type, "invalid type \"unknown\"");

    SUBCASE_BAD_OPTION("Invalid default value",
            xml_option_bad_default, "invalid default value");

    SUBCASE_BAD_OPTION("Invalid minimum value",
            xml_option_int_bad_min, "invalid minimum value");

    SUBCASE_BAD_OPTION("Invalid maximum value",
            xml_option_int_bad_max, "invalid maximum value");

    SUBCASE_BAD_OPTION("Missing option name",
            xml_option_missing_name, "missing \"name\" attribute");

    SUBCASE_BAD_OPTION("Missing option type",
            xml_option_missing_type, "missing \"type\" attribute");

    SUBCASE_BAD_OPTION("Missing option default value",
            xml_option_missing_default_value, "no default value specified");
}

/* ------------------------- create_section test ---------------------------- */
static const std::string xml_section_empty = R"(
<plugin name="TestPluginEmpty">
</plugin>
)";

static const std::string xml_section_no_plugins = R"(
<plugin name="TestPluginNoPlugins">
<description> </description>
<check> </check>
</plugin>
)";

static const std::string xml_section_full = R"(
<plugin name="TestPluginFull">
    <option name="KeyOption" type="key">
        <default>&lt;super&gt; KEY_E</default>
    </option>
    <option name="ButtonOption" type="button">
        <default>&lt;super&gt; BTN_LEFT</default>
    </option>
    <option name="TouchOption" type="gesture">
        <default>swipe up 3</default>
    </option>
    <option name="ActivatorOption" type="activator">
        <default>&lt;super&gt; KEY_E | swipe up 3</default>
    </option>
    <option name="IntOption" type="int">
        <default>3</default>
    </option>
    <option name="DoubleOption" type="double">
        <default>5.0</default>
    </option>
    <option name="StringOption" type="string">
        <default>test</default>
    </option>
    <option name="KeyOption2" type="invalid">
        <default>&lt;super&gt; KEY_T</default>
    </option>
</plugin>
)";

static const std::string xml_section_missing_name = R"(
<plugin>
    <option name="KeyOption" type="key">
        <default>&lt;super&gt; KEY_T</default>
    </option>
</plugin>
)";

static const std::string xml_section_bad_tag = R"(
<invalid>
    <option name="KeyOption" type="key">
        <default>&lt;super&gt; KEY_T</default>
    </option>
</invalid>
)";

TEST_CASE("wf::config::xml::create_section")
{
    std::stringstream log;
    wf::log::initialize_logging(log,
        wf::log::LOG_LEVEL_DEBUG, wf::log::LOG_COLOR_MODE_OFF);

    namespace wxml = wf::config::xml;
    namespace wc = wf::config;

    auto initialize_section = [] (std::string xml_source)
    {
        auto node = xmlParseDoc((const xmlChar*)xml_source.c_str());
        REQUIRE(node != nullptr);
        return wxml::create_section_from_xml_node(xmlDocGetRootElement(node));
    };

    SUBCASE("Empty section")
    {
        auto section = initialize_section(xml_section_empty);
        REQUIRE(section != nullptr);
        CHECK(section->get_name() == "TestPluginEmpty");
        CHECK(section->get_registered_options().empty());
    }

    SUBCASE("Empty section - unnecessary data")
    {
        auto section = initialize_section(xml_section_no_plugins);
        REQUIRE(section != nullptr);
        CHECK(section->get_name() == "TestPluginNoPlugins");
        CHECK(section->get_registered_options().empty());
    }

    SUBCASE("Section with options")
    {
        auto section = initialize_section(xml_section_full);
        REQUIRE(section != nullptr);
        CHECK(section->get_name() == "TestPluginFull");

        auto opts = section->get_registered_options();
        std::set<std::string> opt_names;
        for (auto& opt : opts)
            opt_names.insert(opt->get_name());

        std::set<std::string> expected_names = {
            "KeyOption", "ButtonOption", "TouchOption", "ActivatorOption",
            "IntOption", "DoubleOption", "StringOption"};
        CHECK(opt_names == expected_names);
    }

    SUBCASE("Missing section name")
    {
        auto section = initialize_section(xml_section_missing_name);
        CHECK(section == nullptr);
        EXPECT_LINE(log, "missing \"name\" attribute");
    }

    SUBCASE("Invalid section xml tag")
    {
        auto section = initialize_section(xml_section_bad_tag);
        CHECK(section == nullptr);
        EXPECT_LINE(log, "is not a plugin element");
    }
}
