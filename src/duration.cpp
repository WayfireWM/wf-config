#include <wayfire/util/duration.hpp>
#include <wayfire/util/log.hpp>
#include <chrono>
#include <cmath>

namespace wf
{
namespace animation
{
namespace smoothing
{
smooth_function linear  =
    [] (double x) -> double { return x; };
smooth_function circle  =
    [] (double x) -> double { return std::sqrt(2 * x - x * x); };

const double sigmoid_max = 1 + std::exp(-6);
smooth_function sigmoid =
    [] (double x) -> double { return sigmoid_max / (1 + exp(-12 * x + 6)); };
}
}
}

struct wf::animation::duration_t::impl
{
    decltype(std::chrono::system_clock::now()) start_point;

    std::shared_ptr<wf::config::option_t<int>> length;
    smoothing::smooth_function smooth_function;
    bool is_running = false;

    long get_elapsed() const
    {
        using namespace std::chrono;
        auto now = system_clock::now();
        return duration_cast<milliseconds>(now - start_point).count();
    }

    int get_duration() const
    {
        if (length)
            return std::max(1, length->get_value());

        LOGD("Calling methods on wf::animation::duration_t without a length");
        return 1;
    }

    bool is_ready() const
    {
        return get_elapsed() >= get_duration();
    }

    double get_progress_percentage() const
    {
        if (!length || is_ready())
            return 1.0;

        return 1.0 * get_elapsed() / get_duration();
    }
};


wf::animation::duration_t::duration_t(
    std::shared_ptr<wf::config::option_t<int>> length,
    smoothing::smooth_function smooth)
{
    this->priv = std::make_unique<impl> ();
    this->priv->length = length;
    this->priv->is_running = false;
    this->priv->smooth_function = smooth;
}

wf::animation::duration_t::duration_t(duration_t&& other) = default;
wf::animation::duration_t& wf::animation::duration_t::operator = (
    duration_t&& other) = default;

wf::animation::duration_t::~duration_t() = default;

/**
 * Start the duration.
 * This means that the progress will get reset to 0.
 */
void wf::animation::duration_t::start()
{
    this->priv->is_running = 1;
    this->priv->start_point = std::chrono::system_clock::now();
}

double wf::animation::duration_t::progress() const
{
    if (this->priv->is_ready())
        return 1.0;

    return this->priv->smooth_function(
        this->priv->get_progress_percentage());
}

bool wf::animation::duration_t::running()
{
    if (this->priv->is_ready())
    {
        bool was_running = this->priv->is_running;
        this->priv->is_running = false;
        return was_running;
    }

    return true;
}

wf::animation::timed_transition_t::timed_transition_t(
    const duration_t& dur, double start, double end) : duration(dur)
{
    this->set(start, end);
}

void wf::animation::timed_transition_t::restart_with_end(double new_end)
{
    this->start = (double)*this;
    this->end = new_end;
}

void wf::animation::timed_transition_t::restart_same_end()
{
    this->start = (double)*this;
}

void wf::animation::timed_transition_t::set(double start, double end)
{
    this->start = start;
    this->end = end;
}

void wf::animation::timed_transition_t::flip()
{
    std::swap(this->start, this->end);
}

wf::animation::timed_transition_t::operator double() const
{
    double alpha = this->duration.get().progress();
    return (1 - alpha) * start + alpha * end;
}
