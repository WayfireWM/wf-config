#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <wayfire/config/section.hpp>
#include <wayfire/config/types.hpp>

TEST_CASE("wf::config::section_t")
{
    using namespace wf;
    using namespace wf::config;

    section_t section{"Test_Section-12.34"};
    CHECK(section.get_name() == "Test_Section-12.34");
    CHECK(section.get_registered_options().empty());
    CHECK(section.get_option_or("non_existing") == nullptr);

    auto intopt = std::make_shared<option_t<int>>("IntOption", 123);
    section.register_new_option(intopt);

    CHECK(section.get_option("IntOption") == intopt);
    CHECK(section.get_option_or("IntOption") == intopt);
    CHECK(section.get_option_or("DoubleOption") == nullptr);

    auto reg_opts = section.get_registered_options();
    REQUIRE(reg_opts.size() == 1);
    CHECK(reg_opts.back() == intopt);

    auto intopt2 = std::make_shared<option_t<int>>("IntOption", 125);
    section.register_new_option(intopt2); // overwrite
    CHECK(section.get_option_or("IntOption") == intopt2);

    reg_opts = section.get_registered_options();
    REQUIRE(reg_opts.size() == 1);
    CHECK(reg_opts.back() == intopt2);
}

