#include <wayfire/config/compound-option.hpp>
#include <wayfire/config/xml.hpp>
#include "option-impl.hpp"

using namespace wf::config;

static bool begins_with(const std::string& a, const std::string& b)
{
    return a.substr(0, b.size()) == b;
}

compound_option_t::compound_option_t(const std::string& name,
    entries_t&& entries, std::string type_hint) : option_base_t(name),
    list_type_hint(
        type_hint)
{
    this->entries = std::move(entries);
}

void wf::config::update_compound_from_section(
    compound_option_t& compound,
    const std::shared_ptr<section_t>& section)
{
    const auto& options = section->get_registered_options();

    const auto& should_ignore_option = [] (const std::shared_ptr<wf::config::option_base_t>& opt)
    {
        return xml::get_option_xml_node(opt) || !opt->priv->option_in_config_file;
    };

    const auto& entries = compound.get_entries();

    std::map<std::string, std::vector<std::string>> new_values;

    // find possible suffixes
    for (const auto& opt : options)
    {
        if (should_ignore_option(opt))
        {
            continue;
        }

        // iterating from the end ta handle cases where there is a prefix which is a prefix of another prefix
        // for instance, if there are entries with prefixes `prefix_` and `prefix_smth_` (in that order),
        // then option with name `prefix_smth_suffix` will be recognised with prefix `prefix_smth_`.
        for (auto it = entries.rbegin(); it != entries.rend(); ++it)
        {
            const auto& entry = *it;
            if (begins_with(opt->get_name(), entry->get_prefix()))
            {
                new_values.emplace(opt->get_name().substr(entry->get_prefix().size()), entries.size() + 1);
                break;
            }
        }
    }

    compound_option_t::stored_type_t stored_value;
    for (auto& [suffix, value] : new_values)
    {
        value[0] = suffix;
        for (size_t i = 0; i < entries.size(); ++i)
        {
            if (const auto & entry_option =
                    section->get_option_or(entries[i]->get_prefix() + suffix);
                entry_option && !should_ignore_option(entry_option))
            {
                if (entries[i]->is_parsable(entry_option->get_value_str()))
                {
                    value[i + 1] = entry_option->get_value_str();
                    continue;
                } else
                {
                    LOGE("Failed parsing option ",
                        section->get_name() + "/" + entry_option->get_name(),
                        " as part of the list option ",
                        section->get_name() + "/" + compound.get_name(),
                        ". Trying to use the default value.");
                }
            }

            if (const auto& default_value = entries[i]->get_default_value())
            {
                value[i + 1] = *default_value;
            } else
            {
                LOGE("The option ",
                    section->get_name() + "/" + entries[i]->get_prefix() + suffix,
                    " is neither specified nor has a default value");
                value.clear();
                break;
            }
        }

        if (!value.empty())
        {
            stored_value.push_back(std::move(value));
        }
    }

    compound.set_value_untyped(stored_value);
}

compound_option_t::stored_type_t compound_option_t::get_value_untyped()
{
    return this->value;
}

bool compound_option_t::set_value_untyped(stored_type_t value)
{
    for (auto& e : value)
    {
        if (e.size() != this->entries.size() + 1)
        {
            return false;
        }

        for (size_t i = 1; i <= this->entries.size(); i++)
        {
            if (!entries[i - 1]->is_parsable(e[i]))
            {
                return false;
            }
        }
    }

    this->value = value;
    notify_updated();
    return true;
}

const compound_option_t::entries_t& compound_option_t::get_entries() const
{
    return this->entries;
}

/* --------------------------- option_base_t impl --------------------------- */
std::shared_ptr<option_base_t> compound_option_t::clone_option() const
{
    entries_t cloned;
    for (auto& e : this->entries)
    {
        cloned.push_back(
            std::unique_ptr<compound_option_entry_base_t>(e->clone()));
    }

    auto result = std::make_shared<compound_option_t>(get_name(), std::move(cloned));
    result->value = this->value;
    return result;
}

bool wf::config::compound_option_t::set_value_str(const std::string&)
{
    // XXX: not supported yet
    return false;
}

void wf::config::compound_option_t::reset_to_default()
{
    this->value.clear();
}

bool wf::config::compound_option_t::set_default_value_str(const std::string&)
{
    // XXX: not supported yet
    return false;
}

std::string wf::config::compound_option_t::get_value_str() const
{
    // XXX: not supported yet
    return "";
}

std::string wf::config::compound_option_t::get_default_value_str() const
{
    // XXX: not supported yet
    return "";
}
