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
    timed_transition_t transition2{duration, 1, 2};
    transition.set(1, 2);
    CHECK(transition.start == doctest::Approx(1.0));
    CHECK(transition.end == doctest::Approx(2.0));

    duration.start();
    CHECK((double)transition == doctest::Approx(1.0));
    usleep(50000);
    CHECK((double)transition == doctest::Approx(1.5));
    CHECK((double)transition2 == doctest::Approx(1.5));
    transition.restart_with_end(3);
    transition2.restart_same_end();
    CHECK(transition.start == doctest::Approx(1.5));
    CHECK(transition2.start == doctest::Approx(1.5));
    CHECK(transition.end == doctest::Approx(3));
    CHECK(transition2.end == doctest::Approx(2));
    usleep(60000);
    CHECK((double)transition == doctest::Approx(3.0));

    transition.flip();
    CHECK(transition.start == doctest::Approx(3.0));
    CHECK(transition.end == doctest::Approx(1.5));
}

TEST_CASE("wf::animation::simple_animation_t")
{
    auto length = std::make_shared<option_t<int>>("length", 10);
    simple_animation_t anim{length, smoothing::linear};

    auto cycle_through = [&] (double s, double e, bool x1, bool x2)
    {
        if (!x1 && !x2)
            anim.animate(s, e);
        else if (!x2)
            anim.animate(e);
        else if (!x1)
            anim.animate();

        CHECK(anim.running());
        CHECK((double)anim == doctest::Approx(s));
        usleep(5000);
        CHECK((double)anim == doctest::Approx((s + e) / 2));
        CHECK(anim.running());
        usleep(5500);
        CHECK((double)anim == doctest::Approx(e));
        CHECK(anim.running());
        CHECK(!anim.running());
    };

    cycle_through(1, 2, false, false);
    cycle_through(2, 3, true, false);
    cycle_through(3, 3, false, true);
}
