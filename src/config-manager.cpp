#include <wayfire/config/config-manager.hpp>
#include <cassert>
#include <map>

struct wf::config::config_manager_t::impl
{
    std::map<std::string, std::shared_ptr<section_t>> sections;
};

void wf::config::config_manager_t::merge_section(
    std::shared_ptr<section_t> section)
{
    assert(section);
    if (this->priv->sections.count(section->get_name()) == 0)
    {
        /* Did not exist previously, just add the new section */
        this->priv->sections[section->get_name()] = section;
        return;
    }

    /* Merge with existing config section */
    auto existing_section = get_section(section->get_name());
    auto merging_options  = section->get_registered_options();
    for (auto& option : merging_options)
    {
        auto existing_option =
            existing_section->get_option_or(option->get_name());

        if (existing_option)
        {
            existing_option->set_value_str(option->get_value_str());
        } else
        {
            existing_section->register_new_option(option);
        }
    }
}

std::shared_ptr<wf::config::section_t> wf::config::config_manager_t::get_section(
    const std::string& name) const
{
    if (this->priv->sections.count(name))
    {
        return this->priv->sections.at(name);
    }

    return nullptr;
}

std::vector<std::shared_ptr<wf::config::section_t>> wf::config::config_manager_t::
get_all_sections() const
{
    std::vector<std::shared_ptr<wf::config::section_t>> list;
    for (auto& section : this->priv->sections)
    {
        list.push_back(section.second);
    }

    return list;
}

std::shared_ptr<wf::config::option_base_t> wf::config::config_manager_t::get_option(
    const std::string& name) const
{
    size_t splitter = name.find_first_of("/");
    if (splitter == std::string::npos)
    {
        return nullptr;
    }

    auto section_name = name.substr(0, splitter);
    auto option_name  = name.substr(splitter + 1);
    if (section_name.empty() || option_name.empty())
    {
        return nullptr;
    }

    auto section_ptr = get_section(section_name);
    if (section_ptr)
    {
        return section_ptr->get_option_or(option_name);
    }

    return nullptr;
}

wf::config::config_manager_t::config_manager_t()
{
    this->priv = std::make_unique<impl>();
}

/** Default move operations */
wf::config::config_manager_t::config_manager_t(
    config_manager_t&& other) = default;
wf::config::config_manager_t& wf::config::config_manager_t::operator =(
    config_manager_t&& other) = default;

wf::config::config_manager_t::~config_manager_t() = default;
