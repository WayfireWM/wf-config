#pragma once
#include <wayfire/config/option.hpp>

namespace wf
{
namespace animation
{
namespace smoothing
{
/**
 * A smooth function is a function which takes a double in [0, 1] and returns
 * another double in R. Both ranges represent percentage of a progress of
 * an animation.
 */
using smooth_function = std::function<double(double)>;

/** linear smoothing function, i.e x -> x */
extern smooth_function linear;
/** "circle" smoothing function, i.e x -> sqrt(2x - x*x) */
extern smooth_function circle;
/** "sigmoid" smoothing function, i.e x -> 1.0 / (1 + exp(-12 * x + 6)) */
extern smooth_function sigmoid;
}

/**
 * A transition from start to end.
 */
struct transition_t
{
    double start, end;
};

/**
 * duration_t is a class which can be used to track progress over a specific
 * time interval.
 */
class duration_t
{
  public:
    /**
     * Construct a new duration.
     * Initially, the duration is not running and its progress is 1.
     *
     * @param length The length of the duration in milliseconds.
     * @param smooth The smoothing function for transitions.
     */
    duration_t(std::shared_ptr<wf::config::option_t<int>> length = nullptr,
        smoothing::smooth_function smooth = smoothing::circle);
    ~duration_t();

    duration_t(duration_t&& other) = default;
    duration_t& operator = (duration_t&& other) = default;

    /**
     * Start the duration.
     * This means that the progress will get reset to 0.
     */
    void start();

    /**
     * Get the progress of the duration in percentage.
     * The progress will be smoothed using the smoothing function.
     *
     * @return The current progress after smoothing. It is guaranteed that when
     *   the duration starts, progress will be close to 0, and when it is
     *   finished, it will be close to 1.
     */
    double progress() const;

    /**
     * Check if the duration is still running.
     * Note that even when the duration first finishes, this function will
     * still return that the function is running one time.
     *
     * @return Whether the duration still has not elapsed.
     */
    bool running();
  protected:
    struct impl;
    std::unique_ptr<impl> priv;
};

/**
 * A timed transition is a transition between two states which happens
 * over a period of time.
 *
 * During the transition, the current state is smoothly interpolated between
 * start and end.
 */
struct timed_transition_t : public transition_t
{
    /**
     * Construct a new timed transition using the given duration to measure
     * progress.
     *
     * @duration The duration to use for time measurement
     * @start The start state.
     * @end The end state.
     */
    timed_transition_t(const duration_t& duration,
        double start = 0, double end = 0);

    /**
     * Set the transition start to the current state and the end to the given
     * @new_end.
     */
    void restart_with_end(double new_end);

    /**
     * Set the transition start and end state.
     * @param start The start of the transition.
     * @param end The end of the transition.
     */
    void set(double start, double end);

    /**
     * Implicitly convert the transition to its current state.
     */
    operator double();

  private:
    const duration_t& duration;
};
}
}
