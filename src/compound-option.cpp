#include <wayfire/config/compound-option.hpp>
#include <wayfire/config/xml.hpp>
#include "option-impl.hpp"

using namespace wf::config;

static bool begins_with(const std::string& a, const std::string& b)
{
    return a.substr(0, b.size()) == b;
}

compound_option_t::compound_option_t(const std::string& name,
    entries_t&& entries) : option_base_t(name)
{
    this->entries = std::move(entries);
}

void wf::config::update_compound_from_section(
    compound_option_t& compound,
    const std::shared_ptr<section_t>& section)
{
    auto options = section->get_registered_options();
    std::vector<std::vector<std::string>> new_value;

    struct tuple_in_construction_t
    {
        // How many of the tuple elements were initialized
        size_t initialized = 0;
        std::vector<std::string> values;
    };

    std::map<std::string, std::vector<std::string>> new_values;
    const auto& entries = compound.get_entries();

    for (size_t n = 0; n < entries.size(); n++)
    {
        const auto& prefix = entries[n]->get_prefix();
        for (auto& opt : options)
        {
            if (xml::get_option_xml_node(opt) ||
                !opt->priv->option_in_config_file)
            {
                continue;
            }

            if (begins_with(opt->get_name(), prefix))
            {
                // We have found a match.
                // Find the suffix we should store values in.
                std::string suffix = opt->get_name().substr(prefix.size());
                if (!new_values.count(suffix) && (n > 0))
                {
                    // Skip entries which did not have their first value set,
                    // because these will not be fully constructed in the end.
                    continue;
                }

                auto& tuple = new_values[suffix];

                // Parse the value from the option, with the n-th type.
                if (!entries[n]->is_parsable(opt->get_value_str()))
                {
                    LOGE("Failed parsing option ",
                        section->get_name() + "/" + opt->get_name(),
                        " as part of the list option ",
                        section->get_name() + "/" + compound.get_name());
                    new_values.erase(suffix);
                    continue;
                }

                if (n == 0)
                {
                    // Push the suffix first
                    tuple.push_back(suffix);
                }

                // Update the Nth entry in the tuple (+1 because the first entry
                // is the amount of initialized entries).
                tuple.push_back(opt->get_value_str());
            }
        }
    }

    compound_option_t::stored_type_t value;
    for (auto& e : new_values)
    {
        // Ignore entires which do not have all entries set
        if (e.second.size() != entries.size() + 1)
        {
            continue;
        }

        value.push_back(std::move(e.second));
    }

    compound.set_value_untyped(value);
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
