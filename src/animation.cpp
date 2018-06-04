#include "wayfire/animation.hpp"
#include <cmath>

namespace wf_animation
{
    smooth_function linear = [] (double x) -> double { return x; };
    smooth_function circle = [] (double x) -> double { return std::sqrt(2 * x - x * x); };
};

wf_duration::wf_duration(wf_option dur, wf_animation::smooth_function smooth)
    : smooth_function(smooth), duration(dur) {}

void wf_duration::start(double x, double y)
{
    begin = std::chrono::system_clock::now();

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
    auto duration = duration_cast<milliseconds> (now - begin);

    double p = 1. * duration.count() / msec;
    return smooth_function(std::min(p, 1.0));
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

bool wf_duration::running()
{
    return progress_percentage() <= 0.99;
}
