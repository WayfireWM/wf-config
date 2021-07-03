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
using smooth_function = std::function<double (double)>;

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
template<class State>
struct generic_transition_t
{
    State start, end;
};

using transition_t = generic_transition_t<double>;

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

    /* Copy-constructor */
    duration_t(const duration_t& other);
    /* Copy-assignment */
    duration_t& operator =(const duration_t& other);

    /* Move-constructor */
    duration_t(duration_t&& other) = default;
    /* Move-assignment */
    duration_t& operator =(duration_t&& other) = default;

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

    class impl;
    /** Implementation details. */
    std::shared_ptr<impl> priv;

  private:
    template<class State>
    friend struct generic_timed_transition_t;
    static double get_priv_progress(const std::shared_ptr<const impl>& priv);
};

/**
 * A timed transition is a transition between two states which happens
 * over a period of time.
 *
 * During the transition, the current state is smoothly interpolated between
 * start and end.
 */
template<class State>
struct generic_timed_transition_t : public generic_transition_t<State>
{
    /**
     * Construct a new timed transition using the given duration to measure
     * progress.
     *
     * @duration The duration to use for time measurement
     * @start The start state.
     * @end The end state.
     */
    generic_timed_transition_t(const duration_t& dur,
        State start = {}, State end = {}) : duration(dur.priv)
    {
        this->set(start, end);
    }

    /**
     * Set the transition start to the current state and the end to the given
     * @new_end.
     */
    void restart_with_end(State new_end)
    {
        this->start = (State) * this;
        this->end   = new_end;
    }

    /**
     * Set the transition start to the current state, and don't change the end.
     */
    void restart_same_end()
    {
        this->start = (State) * this;
    }

    /**
     * Set the transition start and end state.
     * @param start The start of the transition.
     * @param end The end of the transition.
     */
    void set(State start, State end)
    {
        this->start = start;
        this->end   = end;
    }

    /**
     * Swap start and end values.
     */
    void flip()
    {
        std::swap(this->start, this->end);
    }

    /**
     * Implicitly convert the transition to its current state.
     */
    operator State() const
    {
        auto alpha = duration_t::get_priv_progress(this->duration);
        return (1 - alpha) * this->start + alpha * this->end;
    }

  private:
    std::shared_ptr<const duration_t::impl> duration;
};
using timed_transition_t = generic_timed_transition_t<double>;

template<class State>
class generic_simple_animation_t : public duration_t,
    public generic_timed_transition_t<State>
{
  public:
    generic_simple_animation_t(
        std::shared_ptr<wf::config::option_t<int>> length = nullptr,
        smoothing::smooth_function smooth = smoothing::linear) :
        duration_t(length, smooth),
        generic_timed_transition_t<State>((duration_t&)*this)
    {}

    /**
     * Set the start and the end of the animation and start the duration.
     */
    void animate(State start, State end)
    {
        this->set(start, end);
        this->duration_t::start();
    }

    /**
     * Animate from the current progress to the given end, and start the
     * duration.
     */
    void animate(State end)
    {
        this->restart_with_end(end);
        this->duration_t::start();
    }

    /**
     * Animate from the current progress to the current end, and start the
     * duration.
     */
    void animate()
    {
        this->restart_same_end();
        this->duration_t::start();
    }
};
using simple_animation_t = generic_simple_animation_t<double>;
}
}
