#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <wayfire/config/edit.hpp>
#include <wayfire/config/config-manager.hpp>
#include <wayfire/config/option.hpp>
#include <wayfire/config/section.hpp>
#include "../src/edit-impl.hpp"

#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

using namespace wf::config::edit;

TEST_CASE("wf::config::edit::apply - modify an existing key")
{
    auto out = apply("[section1]\noption1 = foo\n",
        {patch{"section1", "option1", std::string("bar")}});
    CHECK(out == "[section1]\noption1 = bar\n");
}

TEST_CASE("wf::config::edit::apply - modify preserves no-space format")
{
    auto out = apply("[section1]\noption1=foo\n",
        {patch{"section1", "option1", std::string("bar")}});
    CHECK(out == "[section1]\noption1=bar\n");
}

TEST_CASE("wf::config::edit::apply - preserve trailing comment on modified line")
{
    auto out = apply("[section1]\noption1 = foo  # keep me\n",
        {patch{"section1", "option1", std::string("bar")}});
    CHECK(out == "[section1]\noption1 = bar  # keep me\n");
}

TEST_CASE("wf::config::edit::apply - preserve unrelated lines")
{
    auto out = apply("[section1]\noption1 = foo\noption2 = 2.5\n",
        {patch{"section1", "option1", std::string("bar")}});
    CHECK(out == "[section1]\noption1 = bar\noption2 = 2.5\n");
}

TEST_CASE("wf::config::edit::apply - preserve comments and blank lines around modified line")
{
    const std::string input =
        R"(# top of file
[section1]
# about option1
option1 = foo
# about option2
option2 = 2.5
)";
    const std::string expected =
        R"(# top of file
[section1]
# about option1
option1 = bar
# about option2
option2 = 2.5
)";
    auto out = apply(input,
        {patch{"section1", "option1", std::string("bar")}});
    CHECK(out == expected);
}

TEST_CASE("wf::config::edit::apply - insert new key into existing section")
{
    auto out = apply("[section1]\noption1 = foo\n",
        {patch{"section1", "option2", std::string("3")}});
    CHECK(out == "[section1]\noption1 = foo\noption2 = 3\n");
}

TEST_CASE("wf::config::edit::apply - insert new section")
{
    auto out = apply("[section1]\noption1 = foo\n",
        {patch{"section2", "option1", std::string("bar")}});
    CHECK(out == "[section1]\noption1 = foo\n\n[section2]\noption1 = bar\n");
}

TEST_CASE("wf::config::edit::apply - reset removes a key")
{
    auto out = apply("[section1]\noption1 = foo\noption2 = 2.5\n",
        {patch{"section1", "option1", std::nullopt}});
    CHECK(out == "[section1]\noption2 = 2.5\n");
}

TEST_CASE("wf::config::edit::apply - drop empty section after reset")
{
    auto out = apply("[section1]\noption1 = foo\n\n[section2]\noption1 = bar\n",
        {patch{"section1", "option1", std::nullopt}});
    CHECK(out == "[section2]\noption1 = bar\n");
}

TEST_CASE("wf::config::edit::apply - line continuation with trailing backslash")
{
    const std::string input = R"([section1]
option1 = foo \
  bar \
  baz
)";
    auto out = apply(input,
        {patch{"section1", "option1", std::string("foo bar")}});
    CHECK(out == "[section1]\noption1 = foo bar\n");
}

TEST_CASE("wf::config::edit::apply - multiple patches applied in order")
{
    auto out = apply("[section1]\noption1 = foo\n", {
        patch{"section1", "option1", std::string("bar")},
        patch{"section1", "option2", std::string("3")},
    });
    CHECK(out == "[section1]\noption1 = bar\noption2 = 3\n");
}

TEST_CASE("wf::config::edit::apply - empty patches is a no-op")
{
    auto out = apply("[section1]\noption1 = foo\n", {});
    CHECK(out == "[section1]\noption1 = foo\n");
}

TEST_CASE("wf::config::edit::apply - missing section reset is a no-op")
{
    auto out = apply("[section1]\noption1 = foo\n",
        {patch{"section2", "option1", std::nullopt}});
    CHECK(out == "[section1]\noption1 = foo\n");
}

const std::string patch_file_scratch = TEST_SOURCE "/edit_scratch.ini";

std::string read_file(const std::string& path)
{
    std::ifstream in(path);
    std::stringstream buf;
    buf << in.rdbuf();
    return buf.str();
}

TEST_CASE("wf::config::edit::patch_file - empty patches is a no-op")
{
    std::ofstream(patch_file_scratch) << "[section1]\noption1 = foo\n";

    CHECK(patch_file(patch_file_scratch, {}));
    CHECK(read_file(patch_file_scratch) == "[section1]\noption1 = foo\n");

    ::unlink(patch_file_scratch.c_str());
}

TEST_CASE("wf::config::edit::patch_file - writes edited contents")
{
    std::ofstream(patch_file_scratch) << "[section1]\noption1 = foo\n";

    CHECK(patch_file(patch_file_scratch,
        {patch{"section1", "option1", std::string("bar")}}));
    CHECK(read_file(patch_file_scratch) == "[section1]\noption1 = bar\n");

    ::unlink(patch_file_scratch.c_str());
}

TEST_CASE("wf::config::edit::patch_file - missing file with only resets is a no-op")
{
    ::unlink(patch_file_scratch.c_str());

    CHECK(patch_file(patch_file_scratch,
        {patch{"section1", "option1", std::nullopt}}));
    CHECK(::access(patch_file_scratch.c_str(), F_OK) != 0);
}

TEST_CASE("wf::config::edit::patch_file - missing file with a write creates the file")
{
    ::unlink(patch_file_scratch.c_str());

    CHECK(patch_file(patch_file_scratch,
        {patch{"section1", "option1", std::string("bar")}}));
    CHECK(read_file(patch_file_scratch) == "[section1]\noption1 = bar\n");

    ::unlink(patch_file_scratch.c_str());
}

TEST_CASE("wf::config::edit::patch_file - returns false on unwritable file")
{
    std::ofstream(patch_file_scratch) << "keep-me";
    REQUIRE(::chmod(patch_file_scratch.c_str(), 0444) == 0);

    CHECK(!patch_file(patch_file_scratch,
        {patch{"section1", "option1", std::string("bar")}}));
    CHECK(read_file(patch_file_scratch) == "keep-me");

    ::chmod(patch_file_scratch.c_str(), 0644);
    ::unlink(patch_file_scratch.c_str());
}

wf::config::config_manager_t build_validation_manager()
{
    using namespace wf;
    using namespace wf::config;

    auto section = std::make_shared<section_t>("section1");
    section->register_new_option(std::make_shared<option_t<std::string>>("option1",
        std::string("foo")));

    auto bounded = std::make_shared<option_t<int>>("option2", 2);
    bounded->set_minimum(0);
    bounded->set_maximum(8);
    section->register_new_option(bounded);

    config_manager_t manager;
    manager.merge_section(section);
    return manager;
}

TEST_CASE("wf::config::edit::patch_file - manager-validated write applies")
{
    auto manager = build_validation_manager();
    std::ofstream(patch_file_scratch) << "[section1]\noption1 = foo\n";

    CHECK(patch_file(patch_file_scratch,
        {patch{"section1", "option1", std::string("bar")}}, &manager));
    CHECK(read_file(patch_file_scratch) == "[section1]\noption1 = bar\n");

    ::unlink(patch_file_scratch.c_str());
}

TEST_CASE("wf::config::edit::patch_file - invalid patches are silently skipped")
{
    auto manager = build_validation_manager();
    std::ofstream(patch_file_scratch) << "[section1]\noption1 = foo\n";

    CHECK(patch_file(patch_file_scratch, {
        patch{"section1", "option1", std::string("bar")},
        patch{"section1", "option2", std::string("not_a_number")},
    }, &manager));
    CHECK(read_file(patch_file_scratch) == "[section1]\noption1 = bar\n");

    ::unlink(patch_file_scratch.c_str());
}

TEST_CASE("wf::config::edit::patch_file - all patches invalid leaves file untouched")
{
    auto manager = build_validation_manager();
    std::ofstream(patch_file_scratch) << "[section1]\noption1 = foo\n";

    CHECK(patch_file(patch_file_scratch,
        {patch{"section1", "option2", std::string("not_a_number")}}, &manager));
    CHECK(read_file(patch_file_scratch) == "[section1]\noption1 = foo\n");

    ::unlink(patch_file_scratch.c_str());
}

TEST_CASE("wf::config::edit::patch_file - unknown options are silently skipped")
{
    auto manager = build_validation_manager();
    std::ofstream(patch_file_scratch) << "[section1]\noption1 = foo\n";

    CHECK(patch_file(patch_file_scratch,
        {patch{"section2", "unknown", std::string("value")}}, &manager));
    CHECK(read_file(patch_file_scratch) == "[section1]\noption1 = foo\n");

    ::unlink(patch_file_scratch.c_str());
}

TEST_CASE("wf::config::edit::patch_file - resets pass through validation")
{
    auto manager = build_validation_manager();
    std::ofstream(patch_file_scratch) << "[section1]\noption1 = foo\n";

    CHECK(patch_file(patch_file_scratch,
        {patch{"section1", "option1", std::nullopt}}, &manager));
    CHECK(read_file(patch_file_scratch).find("option1") == std::string::npos);

    ::unlink(patch_file_scratch.c_str());
}

TEST_CASE("wf::config::edit::patch_file - accepted value is canonicalised")
{
    auto manager = build_validation_manager();
    std::ofstream(patch_file_scratch) << "[section1]\noption2 = 2\n";

    CHECK(patch_file(patch_file_scratch,
        {patch{"section1", "option2", std::string("12")}}, &manager));
    CHECK(read_file(patch_file_scratch) == "[section1]\noption2 = 8\n");

    ::unlink(patch_file_scratch.c_str());
}
