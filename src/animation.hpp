#include "config.hpp"
#include <chrono>

/* Useful structures for "linear" animations,
 * i.e animations that depend on the linear change of a quantity */
namespace wf_animation
{
    using smooth_function = std::function<double(double)>;

    /* TODO: maybe add sine, bounce and others? */
    extern smooth_function linear, circle;
}

struct wf_transition
{
    double start, end;
};

struct wf_duration
{
    protected:
        decltype(std::chrono::system_clock::now()) begin;
        wf_animation::smooth_function smooth_function;
        wf_option duration;
        uint32_t msec;

    public:
        wf_duration(wf_option option = nullptr,
                    wf_animation::smooth_function smooth = wf_animation::circle);

        double start_value, end_value;
        void start(double x = 0, double y = 0);

        double progress_percentage();

        double progress();
        double progress(double x, double y);
        double progress(const wf_transition& transition);

        bool running();
};
