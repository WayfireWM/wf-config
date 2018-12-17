#include "animation.hpp"
#include <cmath>

namespace wf_animation
{
    smooth_function linear  = [] (double x) -> double { return x; };
    smooth_function circle  = [] (double x) -> double { return std::sqrt(2 * x - x * x); };
    smooth_function sigmoid = [] (double x) -> double { return 1.0 / (1 + exp(-12 * x + 6)); };
};

wf_duration::wf_duration(wf_option dur, wf_animation::smooth_function smooth)
    : smooth_function(smooth), duration(dur) {}

void wf_duration::start(double x, double y)
{
    begin = std::chrono::system_clock::now();
    is_running = true;

    if (duration)
        msec = duration->as_int();
    else
        msec = 0;

    start_value = x;
    end_value = y;
}

double wf_duration::progress_percentage()
{
    using namespace std::chrono;
    auto now = system_clock::now();
    auto elapsed = duration_cast<milliseconds> (now - begin).count();

    double p = msec > 0 ? (1. * elapsed / msec) : 1.0;
    double smoothed = smooth_function(std::min(p, 1.0));

    if (smoothed >= 0.995)
        smoothed = 1.0;

    return smoothed;
}

double wf_duration::progress()
{
    auto p = progress_percentage();
    return start_value * (1 - p) + end_value * p;
}

double wf_duration::progress(double x, double y)
{
    auto p = progress_percentage();
    return x * (1 - p) + y * p;
}

double wf_duration::progress(const wf_transition& transition)
{
    return progress(transition.start, transition.end);
}

/* The first time we reach 1.0 progress, we usually want to report the duration is
 * still running to ensure that animations that use wf_duration render their last
 * frame properly before being "notified" that the duration has expired. */
bool wf_duration::running()
{
    if (progress_percentage() < 1.0f)
        return true;

    if (is_running)
    {
        is_running = false;
        return true;
    }

    return false;
}
