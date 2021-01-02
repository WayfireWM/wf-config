#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <iostream>
#include "doctest.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include <iostream>
#include <fstream>

#include <wayfire/config/file.hpp>
#include <wayfire/util/log.hpp>
#include <wayfire/config/types.hpp>
#include "wayfire/config/compound-option.hpp"

const std::string contents =
    R"(
illegal_option = value

[section1]
option1 = value1
option2=3
#Comment
option3         = value value value      # Comment

hey_a = 15
bey_a = 1.2

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

    using wf::config::compound_option_t;
    using wf::config::compound_option_entry_t;

    compound_option_t::entries_t entries;
    entries.push_back(std::make_unique<compound_option_entry_t<int>>("hey_"));
    entries.push_back(std::make_unique<compound_option_entry_t<double>>("bey_"));
    auto opt = new compound_option_t{"option_list", std::move(entries)};

    using namespace wf;
    using namespace wf::config;
    config_manager_t config;

    /* Create the first section and add an option there */
    auto section = std::make_shared<section_t>("section1");
    section->register_new_option(
        std::make_shared<option_t<int>>("option1", 10));
    section->register_new_option(
        std::make_shared<option_t<int>>("option2", 5));
    section->register_new_option(
        std::make_shared<option_t<std::string>>("option4", std::string("option4")));
    section->register_new_option(std::shared_ptr<option_base_t>(opt));

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

    CHECK(opt->get_value<int, double>().size() == 1);

    EXPECT_LINE(log, "Error in file test:2");
    EXPECT_LINE(log, "Error in file test:5");
    EXPECT_LINE(log, "Error in file test:20");
    EXPECT_LINE(log, "Error in file test:21");
}

const std::string minimal_config_with_opt =
    R"(
[section]
option = value
)";

TEST_CASE("wf::config::load_configuration_options_from_string - lock & reload")
{
    using namespace wf;
    using namespace wf::config;

    config_manager_t cfg;
    load_configuration_options_from_string(cfg, minimal_config_with_opt);

    SUBCASE("locked")
    {
        cfg.get_option("section/option")->set_locked();
        load_configuration_options_from_string(cfg, "");
        CHECK(cfg.get_option("section/option")->get_value_str() == "value");
    }

    SUBCASE("unlocked")
    {
        load_configuration_options_from_string(cfg, "");
        CHECK(cfg.get_option("section/option")->get_value_str() == "");
    }
}

wf::config::config_manager_t build_simple_config()
{
    using namespace wf;
    using namespace wf::config;
    auto section1 = std::make_shared<section_t>("section1");
    auto section2 = std::make_shared<section_t>("section2");

    section1->register_new_option(std::make_shared<option_t<int>>("option1", 4));
    section1->register_new_option(std::make_shared<option_t<std::string>>("option2",
        std::string("45 # 46 \\")));
    section2->register_new_option(std::make_shared<option_t<double>>("option1",
        4.25));

    compound_option_t::entries_t entries;
    entries.push_back(std::make_unique<compound_option_entry_t<int>>("hey_"));
    entries.push_back(std::make_unique<compound_option_entry_t<double>>("bey_"));
    auto opt = new compound_option_t{"option_list", std::move(entries)};
    opt->set_value(compound_list_t<int, double>{{"k1", 1, 1.2}});
    section2->register_new_option(std::shared_ptr<compound_option_t>(opt));

    config_manager_t config;
    config.merge_section(section1);
    config.merge_section(section2);

    return config;
}

std::string simple_config_source =
    R"([section1]
option1 = 4
option2 = 45 \# 46 \\

[section2]
bey_k1 = 1.200000
hey_k1 = 1
option1 = 4.250000

)";


TEST_CASE("wf::config::save_configuration_options_to_string")
{
    auto config = build_simple_config();
    auto stringified = save_configuration_options_to_string(config);
    CHECK(stringified == simple_config_source);
}

TEST_CASE("wf::config::load_configuration_options_from_file - no such file")
{
    std::string test_config = std::string("FileDoesNotExist");
    wf::config::config_manager_t manager;
    CHECK(!load_configuration_options_from_file(manager, test_config));
}

TEST_CASE("wf::config::load_configuration_options_from_file - locking fails")
{
    std::string test_config = std::string("../test/config_lock.ini");

    const int delay = 100e3; /** 100ms */

    int pid = fork();
    if (pid == 0)
    {
        /* Lock config file before parent tries to lock it */
        int fd = open(test_config.c_str(), O_RDWR);
        flock(fd, LOCK_EX);

        /* Obtained a lock. Now wait until parent tries to lock */
        usleep(2 * delay);

        /* By now, parent should have failed. */
        flock(fd, LOCK_UN);
        close(fd);
    }

    /* Wait for other process to lock the file */
    usleep(delay);

    wf::config::config_manager_t manager;
    CHECK(!load_configuration_options_from_file(manager, test_config));
}

void check_int_test_config(const wf::config::config_manager_t& manager,
    std::string value_opt1 = "12")
{
    auto s1 = manager.get_section("section1");
    auto s2 = manager.get_section("section2");
    REQUIRE(s1 != nullptr);
    REQUIRE(s2 != nullptr);

    auto o1 = manager.get_option("section1/option1");
    auto o2 = manager.get_option("section2/option2");
    auto o3 = manager.get_option("section2/option3");

    REQUIRE(o1);
    REQUIRE(o2);
    REQUIRE(o3);
    CHECK(o1->get_value_str() == value_opt1);
    CHECK(o2->get_value_str() == "opt2");
    CHECK(o3->get_value_str() == "DoesNotExistInXML # \\");
}

TEST_CASE("wf::config::load_configuration_options_from_file - success")
{
    std::string test_config = std::string(TEST_SOURCE "/int_test/config.ini");

    /* Init with one section */
    wf::config::config_manager_t manager;
    auto s = std::make_shared<wf::config::section_t>("section1");
    s->register_new_option(
        std::make_shared<wf::config::option_t<int>>("option1", 1));
    manager.merge_section(s);

    CHECK(load_configuration_options_from_file(manager, test_config));
    REQUIRE(manager.get_section("section1") == s);
    check_int_test_config(manager);
}

TEST_CASE("wf::config::save_configuration_to_file - success")
{
    std::string test_config = std::string(TEST_SOURCE "/dummy.ini");

    {
        std::ofstream clr(test_config, std::ios::trunc | std::ios::ate);
        clr << "Dummy";
    }

    wf::config::save_configuration_to_file(build_simple_config(), test_config);

    /* Read file contents */
    std::ifstream infile(test_config);
    std::string file_contents((std::istreambuf_iterator<char>(infile)),
        std::istreambuf_iterator<char>());

    CHECK(file_contents == simple_config_source);

    /* Check lock is released */
    int fd = open(test_config.c_str(), O_RDWR);
    CHECK(flock(fd, LOCK_EX | LOCK_NB) == 0);
    flock(fd, LOCK_UN);
    close(fd);
}

TEST_CASE("wf::config::build_configuration")
{
    wf::log::initialize_logging(std::cout, wf::log::LOG_LEVEL_DEBUG,
        wf::log::LOG_COLOR_MODE_ON);
    std::string xmldir   = std::string(TEST_SOURCE "/int_test/xml");
    std::string sysconf  = std::string(TEST_SOURCE "/int_test/sys.ini");
    std::string userconf = std::string(TEST_SOURCE "/int_test/config.ini");

    std::vector xmldirs(1, xmldir);
    auto config = wf::config::build_configuration(xmldirs, sysconf, userconf);
    check_int_test_config(config, "10");

    auto o1 = config.get_option("section1/option1");
    auto o2 = config.get_option("section2/option2");
    auto o3 = config.get_option("section2/option3");
    auto o4 = config.get_option("section2/option4");
    auto o5 = config.get_option("section2/option5");
    auto o6 = config.get_option("sectionobj:objtest/option6");

    REQUIRE(o4);
    REQUIRE(o5);

    using namespace wf;
    using namespace wf::config;
    CHECK(std::dynamic_pointer_cast<option_t<int>>(o1) != nullptr);
    CHECK(std::dynamic_pointer_cast<option_t<std::string>>(o2) != nullptr);
    CHECK(std::dynamic_pointer_cast<option_t<std::string>>(o3) != nullptr);
    CHECK(std::dynamic_pointer_cast<option_t<std::string>>(o4) != nullptr);
    CHECK(std::dynamic_pointer_cast<option_t<std::string>>(o5) != nullptr);
    CHECK(std::dynamic_pointer_cast<option_t<int>>(o6) != nullptr);

    CHECK(o4->get_value_str() == "DoesNotExistInConfig");
    CHECK(o5->get_value_str() == "Option5Sys");
    CHECK(o6->get_value_str() == "10"); // bounds from xml applied

    o1->reset_to_default();
    o2->reset_to_default();
    o3->reset_to_default();
    o4->reset_to_default();
    o5->reset_to_default();
    o6->reset_to_default();

    CHECK(o1->get_value_str() == "4");
    CHECK(o2->get_value_str() == "XMLDefault");
    CHECK(o3->get_value_str() == "");
    CHECK(o4->get_value_str() == "DoesNotExistInConfig");
    CHECK(o5->get_value_str() == "Option5Sys");
    CHECK(o6->get_value_str() == "1");
}
