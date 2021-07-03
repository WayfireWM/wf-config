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
smooth_function linear =
    [] (double x) -> double { return x; };
smooth_function circle =
    [] (double x) -> double { return std::sqrt(2 * x - x * x); };

const double sigmoid_max = 1 + std::exp(-6);
smooth_function sigmoid  =
    [] (double x) -> double { return sigmoid_max / (1 + exp(-12 * x + 6)); };
}
}
}

class wf::animation::duration_t::impl
{
  public:
    decltype(std::chrono::system_clock::now()) start_point;

    std::shared_ptr<wf::config::option_t<int>> length;
    smoothing::smooth_function smooth_function;
    bool is_running = false;

    int64_t get_elapsed() const
    {
        using namespace std::chrono;
        auto now = system_clock::now();
        return duration_cast<milliseconds>(now - start_point).count();
    }

    int get_duration() const
    {
        if (length)
        {
            return std::max(1, length->get_value());
        }

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
        {
            return 1.0;
        }

        return 1.0 * get_elapsed() / get_duration();
    }

    double progress() const
    {
        if (is_ready())
        {
            return 1.0;
        }

        return smooth_function(get_progress_percentage());
    }
};


wf::animation::duration_t::duration_t(
    std::shared_ptr<wf::config::option_t<int>> length,
    smoothing::smooth_function smooth)
{
    this->priv = std::make_shared<impl>();
    this->priv->length     = length;
    this->priv->is_running = false;
    this->priv->smooth_function = smooth;
}

wf::animation::duration_t::duration_t(const duration_t& other)
{
    this->priv = std::make_shared<impl>(*other.priv);
}

wf::animation::duration_t& wf::animation::duration_t::operator =(
    const duration_t& other)
{
    if (&other != this)
    {
        this->priv = std::make_shared<impl>(*other.priv);
    }

    return *this;
}

void wf::animation::duration_t::start()
{
    this->priv->is_running  = 1;
    this->priv->start_point = std::chrono::system_clock::now();
}

double wf::animation::duration_t::progress() const
{
    return this->priv->progress();
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

double wf::animation::duration_t::get_priv_progress(
    const std::shared_ptr<const impl>& priv)
{
    return priv->progress();
}
