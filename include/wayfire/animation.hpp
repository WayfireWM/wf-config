#ifndef ANIMATION_HPP
#define ANIMATION_HPP

#include "config.hpp"
#include <chrono>

/* Useful structures for "linear" animations,
 * i.e animations that depend on the linear change of a quantity */
namespace wf_animation
{
    using smooth_function = std::function<double(double)>;

    /* TODO: maybe add sine, bounce and others? */
    extern smooth_function linear, circle, sigmoid;
}

struct wf_transition
{
    double start, end;
};

/* A helper class to create smooth animations.
 * It works best with animations that update each frame, and loses precision
 * if the animation update happens less frequenty */
struct wf_duration
{
    protected:
        decltype(std::chrono::system_clock::now()) begin;
        wf_animation::smooth_function smooth_function;

        wf_option duration;
        uint32_t msec;
        bool is_running = false;

    public:
        /* @param option - the number of milliseconds this duration should run for */
        wf_duration(wf_option option = nullptr,
                    wf_animation::smooth_function smooth = wf_animation::circle);

        wf_duration(const wf_duration& other) = delete;
        wf_duration& operator = (const wf_duration& other) = delete;

        wf_duration(wf_duration&& other) = default;
        wf_duration& operator = (wf_duration&& other) = default;

        double start_value, end_value;
        /* Restarts the timer for this duration */
        void start(double x = 0, double y = 0);

        /* Returns how much of the time has elapsed */
        double progress_percentage();

        /* The function below return the interpolated values of the animation
         * start and end attributes, based on how much time has elapsed since
         * the start of the duration */
        double progress();
        double progress(double x, double y);
        double progress(const wf_transition& transition);

        /* Returns if the animation is still in progress or it has finished */
        bool running();
};

#endif /* end of include guard: ANIMATION_HPP */
