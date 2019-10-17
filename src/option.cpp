#include <config/option.hpp>
#include <algorithm>
#include <vector>

struct wf::config::option_base_t::impl
{
    std::string name;
    std::vector<updated_callback_t*> updated_handlers;
};

std::string wf::config::option_base_t::get_name() const
{
    return this->priv->name;
}

void wf::config::option_base_t::add_updated_handler(
    updated_callback_t *callback)
{
    this->priv->updated_handlers.push_back(callback);
}

void wf::config::option_base_t::rem_updated_handler(
    updated_callback_t *callback)
{
    auto it = std::remove(priv->updated_handlers.begin(),
        priv->updated_handlers.end(), callback);
    priv->updated_handlers.erase(it, priv->updated_handlers.end());
}

wf::config::option_base_t::option_base_t(const std::string& name)
{
    this->priv = std::make_unique<impl>();
    this->priv->name = name;
}
wf::config::option_base_t::~option_base_t() = default;

void wf::config::option_base_t::notify_updated() const
{
    auto to_call = priv->updated_handlers;
    for (auto& call : to_call)
        (*call)();
}
