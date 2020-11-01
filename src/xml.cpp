#include <wayfire/config/xml.hpp>
#include <wayfire/config/types.hpp>
#include <wayfire/util/log.hpp>

class xml_derived_element_t
{
  public:
    xmlNodePtr xml_node;
};

/**
 * A XML option is the same as a normal option, however it also has access to
 * the XML node which was used to initially create the option.
 */
template<class T> class xml_option_t :
  public wf::config::option_t<T>, public xml_derived_element_t
{
  public:
    using wf::config::option_t<T>::option_t;
};

/**
 * A XML section is the same as a normal section, however it also has access to
 * the XML node which was used to initially create the section.
 */
class xml_section_t : public wf::config::section_t, public xml_derived_element_t
{
  public:
    using wf::config::section_t::section_t;
};

static stdx::optional<const xmlChar*>
    extract_value(xmlNodePtr node, std::string value_name)
{
    stdx::optional<const xmlChar*> value_ptr;

    auto child_ptr = node->children;
    while (child_ptr != nullptr)
    {
        if (child_ptr->type == XML_ELEMENT_NODE &&
            std::string((const char*)child_ptr->name) == value_name)
        {
            auto child_child_ptr = child_ptr->children;
            if (child_child_ptr == nullptr) {
                value_ptr = (const xmlChar*)"";
            } else if (child_child_ptr->next == nullptr &&
                child_child_ptr->type == XML_TEXT_NODE)
            {
                value_ptr = child_child_ptr->content;
            }
        }

        child_ptr = child_ptr->next;
    }

    return value_ptr;
}

/**
 * Create a new option of type T with the given name and default value.
 * @return The new option, or nullptr if the default value is invaild.
 */
template<class T> std::shared_ptr<xml_option_t<T>>
    create_option(std::string name, std::string default_value)
{
    auto value = wf::option_type::from_string<T>(default_value);
    if (!value)
        return {};

    return std::make_shared<xml_option_t<T>> (name, value.value());
}

enum bounds_error_t
{
    BOUNDS_INVALID_MINIMUM,
    BOUNDS_INVALID_MAXIMUM,
    BOUNDS_OK,
};

template<class T> bounds_error_t set_bounds(
    std::shared_ptr<wf::config::option_base_t>& option,
    stdx::optional<const xmlChar*> min_ptr,
    stdx::optional<const xmlChar*> max_ptr)
{
    if (!option)
        return BOUNDS_OK; // there has been an earlier error

    auto typed_option =
        std::dynamic_pointer_cast<wf::config::option_t<T>> (option);
    assert(typed_option);

    if (min_ptr)
    {
        auto value = wf::option_type::from_string<T>(
            (const char*)min_ptr.value());
        if (value) {
            typed_option->set_minimum(value.value());
        } else {
            return BOUNDS_INVALID_MINIMUM;
        }
    }

    if (max_ptr)
    {
        stdx::optional<T> value = wf::option_type::from_string<T>(
            (const char*)max_ptr.value());
        if (value) {
            typed_option->set_maximum(value.value());
        } else {
            return BOUNDS_INVALID_MAXIMUM;
        }
    }

    return BOUNDS_OK;
}

std::shared_ptr<wf::config::option_base_t>
    wf::config::xml::create_option_from_xml_node(xmlNodePtr node)
{
    if (node->type != XML_ELEMENT_NODE ||
        (const char*)node->name != std::string{"option"})
    {
        LOGE("Could not parse ", node->doc->URL,
            ": line ", node->line, " is not an option element.");
        return nullptr;
    }

    auto name_ptr = xmlGetProp(node, (const xmlChar*)"name");
    if (!name_ptr)
    {
        LOGE("Could not parse ", node->doc->URL,
            ": option at line ", node->line, " is missing \"name\" attribute.");
        return nullptr;
    }

    auto type_ptr = xmlGetProp(node, (const xmlChar*)"type");
    if (!type_ptr)
    {
        LOGE("Could not parse ", node->doc->URL,
            ": option at line ", node->line, " is missing \"type\" attribute.");
        return nullptr;
    }

    auto default_value_ptr = extract_value(node, "default");
    if (!default_value_ptr)
    {
        LOGE("Could not parse ", node->doc->URL,
            ": option at line ", node->line, " has no default value specified.");
        return nullptr;
    }

    auto min_value_ptr = extract_value(node, "min");
    auto max_value_ptr = extract_value(node, "max");

    std::string name = (const char*)name_ptr;
    std::string type = (const char*)type_ptr;
    std::string default_value = (const char*)default_value_ptr.value();

    std::shared_ptr<wf::config::option_base_t> option;
    bounds_error_t bounds_error = BOUNDS_OK;

    if (type == "int") {
        option = create_option<int> (name, default_value);
        bounds_error = set_bounds<int>(option,
            min_value_ptr, max_value_ptr);
    } else if (type == "double") {
        option = create_option<double> (name, default_value);
        bounds_error = set_bounds<double>(option,
            min_value_ptr, max_value_ptr);
    } else if (type == "bool") {
        option = create_option<bool> (name, default_value);
    } else if (type == "string") {
        option = create_option<std::string> (name, default_value);
    } else if (type == "key") {
        option = create_option<wf::keybinding_t> (name, default_value);
    } else if (type == "button") {
        option = create_option<wf::buttonbinding_t> (name, default_value);
    } else if (type == "gesture") {
        option = create_option<wf::touchgesture_t> (name, default_value);
    } else if (type == "color") {
        option = create_option<wf::color_t> (name, default_value);
    } else if (type == "activator") {
        option = create_option<wf::activatorbinding_t> (name, default_value);
    } else
    {
        LOGE("Could not parse ", node->doc->URL,
            ": option at line ", node->line,
            " has invalid type \"", type, "\"");
        return nullptr;
    }

    if (!option)
    {
        /* This can only happen if default value was invalid */
        LOGE("Could not parse ", node->doc->URL,
            ": option at line ", node->line,
            " has invalid default value \"", default_value, "\" for type ",
            type);
        return nullptr;
    }

    switch (bounds_error)
    {
        case BOUNDS_INVALID_MINIMUM:
            assert(min_value_ptr);
            LOGE("Could not parse ", node->doc->URL,
                ": option at line ", node->line,
                " has invalid minimum value \"", min_value_ptr.value(), "\"",
                "for type ", type);
            return nullptr;

        case BOUNDS_INVALID_MAXIMUM:
            assert(max_value_ptr);
            LOGE("Could not parse ", node->doc->URL,
                ": option at line ", node->line,
                " has invalid maximum value \"", max_value_ptr.value(), "\"",
                "for type ", type);
            return nullptr;
        default:
            break;
    }

    std::dynamic_pointer_cast<xml_derived_element_t> (option)->xml_node = node;
    return option;
}

static void recursively_parse_section_node(xmlNodePtr node,
    std::shared_ptr<wf::config::section_t> section)
{
    auto child_ptr = node->children;
    while (child_ptr != nullptr)
    {
        if (child_ptr->type == XML_ELEMENT_NODE &&
            std::string((const char*)child_ptr->name) == "option")
        {
            auto option = wf::config::xml::create_option_from_xml_node(
                child_ptr);
            if (option)
                section->register_new_option(option);
        }

        if (child_ptr->type == XML_ELEMENT_NODE &&
            std::string((const char*)child_ptr->name) == "group")
        {
            recursively_parse_section_node(child_ptr, section);
        }

        if (child_ptr->type == XML_ELEMENT_NODE &&
            std::string((const char*)child_ptr->name) == "subgroup")
        {
            recursively_parse_section_node(child_ptr, section);
        }

        child_ptr = child_ptr->next;
    }
}

std::shared_ptr<wf::config::section_t>
    wf::config::xml::create_section_from_xml_node(xmlNodePtr node)
{
    if (node->type != XML_ELEMENT_NODE ||
        ((const char*)node->name != std::string{"plugin"} &&
         (const char*)node->name != std::string{"object"}))
    {
        LOGE("Could not parse ", node->doc->URL,
            ": line ", node->line, " is not a plugin/object element.");
        return nullptr;
    }

    auto name_ptr = xmlGetProp(node, (const xmlChar*)"name");
    if (!name_ptr)
    {
        LOGE("Could not parse ", node->doc->URL,
            ": section at line ", node->line, " is missing \"name\" attribute.");
        return nullptr;
    }

    auto section = std::make_shared<xml_section_t> ((const char*)name_ptr);
    section->xml_node = node;
    recursively_parse_section_node(node, section);
    return section;
}

xmlNodePtr wf::config::xml::get_option_xml_node(
    std::shared_ptr<wf::config::option_base_t> option)
{
    auto xml_opt = std::dynamic_pointer_cast<xml_derived_element_t> (option);
    return xml_opt ? xml_opt->xml_node : nullptr;
}

xmlNodePtr wf::config::xml::get_section_xml_node(
    std::shared_ptr<wf::config::section_t> section)
{
    auto xml_section = std::dynamic_pointer_cast<xml_derived_element_t> (section);
    return xml_section ? xml_section->xml_node : nullptr;
}
