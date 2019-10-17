#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#define protected public // hack for the test
#include <config/option.hpp>

TEST_CASE("wf::option_base_t")
{
    wf::config::option_base_t option{"string"};
    CHECK(option.get_name() == "string");

    int callback_called = 0;
    int callback2_called = 0;

    wf::config::option_base_t::updated_callback_t callback, callback2;
    callback = [&] () { callback_called++; };
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
