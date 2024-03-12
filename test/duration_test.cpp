#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <wayfire/config/option.hpp>
#include <wayfire/util/duration.hpp>
#include <unistd.h>

using namespace wf;
using namespace wf::config;
using namespace wf::animation;

TEST_CASE("wf::animation::duration_t")
{
    duration_t duration;

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

    SUBCASE("Int option")
    {
        auto length = std::make_shared<option_t<int>>("length", 100);
        duration = duration_t{length, smoothing::linear};
    }

    SUBCASE("Animation option")
    {
        auto length = std::make_shared<option_t<wf::animation_description_t>>("length",
            wf::animation_description_t{
            .length_ms = 100,
            .easing    = smoothing::linear,
            .easing_name = "linear",
        });
        duration = duration_t{length};
    }

    /* Check twice, so that we can test restarting */
    check_lifetime();
    check_lifetime();

    auto check_reverse_duration = [&] ()
    {
        auto direction = duration.get_direction();
        CHECK(duration.running() == false);
        CHECK(duration.progress() == doctest::Approx{direction ? 1.0 : 0.0});

        duration.start();
        CHECK(duration.running());
        CHECK(duration.progress() == doctest::Approx{direction ? 0.0 : 1.0});

        usleep(75000);
        CHECK(duration.progress() == doctest::Approx{direction ? 0.75 : 0.25}.epsilon(
            0.1));
        duration.reverse();
        usleep(50000);
        CHECK(duration.progress() == doctest::Approx{direction ? 0.25 : 0.75}.epsilon(
            0.1));
        CHECK(duration.running());
        CHECK(duration.running());
        usleep(35000);

        /* At this point, duration must be finished */
        CHECK(duration.progress() ==
            doctest::Approx{direction ? 0.0 : 1.0}.epsilon(0.01));
        CHECK(duration.running()); // one last time
        CHECK(duration.running() == false);
        CHECK(duration.running() == false);
    };

    /* Check twice, so that we can test direction */
    check_reverse_duration();
    check_reverse_duration();
}

TEST_CASE("wf::animation::timed_transition_t")
{
    const double start   = 1.0;
    const double end     = 2.0;
    const double overend = 3.0;
    const double middle  = (start + end) / 2.0;

    auto length = std::make_shared<option_t<int>>("length", 100);
    duration_t duration{length, smoothing::linear};
    timed_transition_t transition{duration};
    timed_transition_t transition2{duration, start, end};
    transition.set(start, end);
    CHECK(transition.start == doctest::Approx(start));
    CHECK(transition.end == doctest::Approx(end));

    duration.start();
    CHECK((double)transition == doctest::Approx(start));
    usleep(50000);
    CHECK((double)transition == doctest::Approx(middle).epsilon(0.1));
    CHECK((double)transition2 == doctest::Approx(middle).epsilon(0.1));
    transition.restart_with_end(overend);
    transition2.restart_same_end();
    CHECK(transition.start == doctest::Approx(middle).epsilon(0.1));
    CHECK(transition2.start == doctest::Approx(middle).epsilon(0.1));
    CHECK(transition.end == doctest::Approx(overend));
    CHECK(transition2.end == doctest::Approx(end));
    usleep(60000);
    CHECK((double)transition == doctest::Approx(overend).epsilon(0.1));

    transition.flip();
    CHECK(transition.start == doctest::Approx(3.0).epsilon(0.1));
    CHECK(transition.end == doctest::Approx(middle).epsilon(0.1));
}

TEST_CASE("wf::animation::simple_animation_t")
{
    auto length = std::make_shared<option_t<int>>("length", 10);
    simple_animation_t anim{length, smoothing::linear};

    auto cycle_through = [&] (double s, double e, bool x1, bool x2)
    {
        if (!x1 && !x2)
        {
            anim.animate(s, e);
        } else if (!x2)
        {
            anim.animate(e);
        } else if (!x1)
        {
            anim.animate();
        }

        CHECK(anim.running());
        CHECK((double)anim == doctest::Approx(s));
        usleep(5000);
        CHECK((double)anim == doctest::Approx((s + e) / 2).epsilon(0.1));
        CHECK(anim.running());
        usleep(5500);
        CHECK((double)anim == doctest::Approx(e));
        CHECK(anim.running());
        CHECK(!anim.running());
    };

    cycle_through(1, 2, false, false);
    cycle_through(2, 3, true, false);
    cycle_through(3, 3, false, true);

    simple_animation_t sa;
    sa = simple_animation_t{length};
    sa.animate(1, 2);
    CHECK((double)sa == doctest::Approx(1.0));
}
