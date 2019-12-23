#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <wayfire/config/option-wrapper.hpp>
#include <wayfire/config/config-manager.hpp>

static wf::config::config_manager_t config;

template<class Type>
class wrapper_t : public wf::base_option_wrapper_t<Type>
{
  public:
    wrapper_t() : wf::base_option_wrapper_t<Type> () {}
    wrapper_t(const std::string& option)
        : wf::base_option_wrapper_t<Type>()
    {
        this->load_option(option);
    }

  protected:
    std::shared_ptr<wf::config::option_base_t>
        load_raw_option(const std::string& name) override
    {
        return config.get_option(name);
    }
};

TEST_CASE("wf::base_option_wrapper_t")
{
    using namespace wf;
    using namespace wf::config;

    auto section = std::make_shared<section_t> ("Test");
    auto opt = std::make_shared<option_t<int>> ("Option1", 5);
    section->register_new_option(opt);
    ::config.merge_section(section);

    wrapper_t<int> wrapper{"Test/Option1"};
    CHECK((option_sptr_t<int>)wrapper == opt);
    CHECK(wrapper == 5);

    bool updated = false;
    wrapper.set_callback([&] () {
        updated = true;
    });
    opt->set_value(6);
    CHECK(updated);

    /* Check move operations */
    wrapper_t<int> wrapper1;
    wrapper1 = wrapper_t<int>{"Test/Option1"};
    CHECK(wrapper1.raw_option == opt);
}
