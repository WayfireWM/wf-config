#include <wayfire/config/option.hpp>
#include <algorithm>
#include <vector>

#include "option-impl.hpp"
#include "wayfire/util/log.hpp"

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
    {
        (*call)();
    }
}

void wf::config::option_base_t::set_locked(bool locked)
{
    this->priv->lock_count += (locked ? 1 : -1);
    if (priv->lock_count < 0)
    {
        LOGE("Lock counter for option ", this->get_name(), " dropped below zero!");
    }
}

bool wf::config::option_base_t::is_locked() const
{
    return this->priv->lock_count > 0;
}

void wf::config::option_base_t::init_clone(option_base_t& other) const
{
    other.priv->xml  = this->priv->xml;
    other.priv->name = this->priv->name;
}
