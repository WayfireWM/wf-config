#include <stdexcept>
#include "section-impl.hpp"

wf::config::section_t::section_t(const std::string& name)
{
    this->priv = std::make_unique<impl>();
    this->priv->name = name;
}

wf::config::section_t::~section_t() = default;

std::string wf::config::section_t::get_name() const
{
    return this->priv->name;
}

std::shared_ptr<wf::config::section_t> wf::config::section_t::clone_with_name(
    const std::string name) const
{
    auto result = std::make_shared<wf::config::section_t>(name);
    for (auto& option : priv->options)
    {
        result->register_new_option(option.second->clone_option());
    }

    result->priv->xml = this->priv->xml;
    return result;
}

std::shared_ptr<wf::config::option_base_t> wf::config::section_t::get_option_or(
    const std::string& name)
{
    if (this->priv->options.count(name))
    {
        return this->priv->options[name];
    }

    return nullptr;
}

std::shared_ptr<wf::config::option_base_t> wf::config::section_t::get_option(
    const std::string& name)
{
    auto option = get_option_or(name);
    if (!option)
    {
        throw std::invalid_argument("Non-existing option " + name +
            " in config section " + this->get_name());
    }

    return option;
}

wf::config::section_t::option_list_t wf::config::section_t::get_registered_options()
const
{
    option_list_t list;
    for (auto& option : priv->options)
    {
        list.push_back(option.second);
    }

    return list;
}

void wf::config::section_t::register_new_option(
    std::shared_ptr<option_base_t> option)
{
    if (!option)
    {
        throw std::invalid_argument(
            "Cannot add null option to section " + this->get_name());
    }

    this->priv->options[option->get_name()] = option;
}

void wf::config::section_t::unregister_option(
    std::shared_ptr<option_base_t> option)
{
    if (!option)
    {
        return;
    }

    auto it = this->priv->options.find(option->get_name());
    if ((it != this->priv->options.end()) && (it->second == option))
    {
        this->priv->options.erase(it);
    }
}
