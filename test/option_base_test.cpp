#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include <wayfire/config/option.hpp>

class option_base_stub_t : public wf::config::option_base_t
{
  public:
    option_base_stub_t(std::string name) :
        option_base_t(name)
    {}

    std::shared_ptr<option_base_t> clone_option() const override
    {
        return nullptr;
    }

    bool set_default_value_str(const std::string&) override
    {
        return true;
    }

    bool set_value_str(const std::string&) override
    {
        return false;
    }

    void reset_to_default() override
    {}
    std::string get_value_str() const override
    {
        return "";
    }

    std::string get_default_value_str() const override
    {
        return "";
    }

  public:
    void notify_updated() const
    {
        option_base_t::notify_updated();
    }
};


TEST_CASE("wf::option_base_t")
{
    option_base_stub_t option{"string"};
    CHECK(option.get_name() == "string");

    int callback_called  = 0;
    int callback2_called = 0;

    wf::config::option_base_t::updated_callback_t callback, callback2;
    callback  = [&] () { callback_called++; };
    callback2 = [&] () { callback2_called++; };

    option.add_updated_handler(&callback);
    option.notify_updated();
    CHECK(callback_called == 1);
    CHECK(callback2_called == 0);

    option.add_updated_handler(&callback);
    option.add_updated_handler(&callback2);
    option.notify_updated();
    CHECK(callback_called == 3);
    CHECK(callback2_called == 1);

    option.rem_updated_handler(&callback);
    option.notify_updated();
    CHECK(callback_called == 3);
    CHECK(callback2_called == 2);

    option.rem_updated_handler(&callback2);
    option.notify_updated();
    CHECK(callback_called == 3);
    CHECK(callback2_called == 2);
}
