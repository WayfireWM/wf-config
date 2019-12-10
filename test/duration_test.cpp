#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <wayfire/config/option.hpp>
#include <wayfire/util/duration.hpp>
#include <unistd.h>

using namespace wf;
using namespace wf::config;
using namespace wf::animation;

TEST_CASE("wf::animation::duration_t")
{
    auto length = std::make_shared<option_t<int>>("length", 100);
    duration_t duration{length, smoothing::linear};

    auto check_lifetime = [&] ()
    {
        CHECK(duration.running() == false);
        CHECK(duration.progress() == doctest::Approx{1.0});

        duration.start();
        CHECK(duration.running());
        CHECK(duration.progress() == doctest::Approx{0.0});

        usleep(50000);
        CHECK(duration.progress() == doctest::Approx{0.5}.epsilon(0.1));
        CHECK(duration.running());
        CHECK(duration.running());
        usleep(60000);

        /* At this point, duration must be finished */
        CHECK(duration.progress() == doctest::Approx{1.0}.epsilon(0.01));
        CHECK(duration.running()); // one last time
        CHECK(duration.running() == false);
        CHECK(duration.running() == false);
    };

    /* Check twice, so that we can test restarting */
    check_lifetime();
    check_lifetime();
}

TEST_CASE("wf::animation::timed_transition_t")
{
    auto length = std::make_shared<option_t<int>>("length", 100);
    duration_t duration{length, smoothing::linear};
    timed_transition_t transition{duration};
    transition.set(1, 2);
    CHECK(transition.start == doctest::Approx(1.0));
    CHECK(transition.end == doctest::Approx(2.0));

    duration.start();
    CHECK((double)transition == doctest::Approx(1.0));
    usleep(50000);
    CHECK((double)transition == doctest::Approx(1.5));
    transition.restart_with_end(3);
    CHECK(transition.start == doctest::Approx(1.5));
    CHECK(transition.end == doctest::Approx(3));
    usleep(60000);
    CHECK((double)transition == doctest::Approx(3.0));
}
