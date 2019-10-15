#include <config/types.hpp>
#include <vector>
#include <map>
#include <iostream>

#include <libevdev/libevdev.h>

/* ----------------------------- wf::color_t -------------------------------- */
wf::color_t::color_t()
    : color_t(0.0, 0.0, 0.0, 0.0) {}

wf::color_t::color_t(double r, double g, double b, double a)
{
    this->r = r;
    this->g = g;
    this->b = b;
    this->a = a;
}

wf::color_t::color_t(const glm::vec4& value)
    : color_t(value.r, value.g, value.b, value.a) {}

static double hex_to_double(std::string value)
{
    char *dummy;
    return std::strtol(value.c_str(), &dummy, 16);
}

wf::color_t::color_t(const std::string& value)
{
    if (!is_valid(value))
    {
        this->r = this->g = this->b = this->a = 0;
        return;
    }

    /* #RRGGBBAA case */
    if (value.size() == 9)
    {
        this->r = hex_to_double(value.substr(1, 2)) / 255.0;
        this->g = hex_to_double(value.substr(3, 2)) / 255.0;
        this->b = hex_to_double(value.substr(5, 2)) / 255.0;
        this->a = hex_to_double(value.substr(7, 2)) / 255.0;
    } else {
        assert(value.size() == 5);
        this->r = hex_to_double(value.substr(1, 1)) / 15.0;
        this->g = hex_to_double(value.substr(2, 1)) / 15.0;
        this->b = hex_to_double(value.substr(3, 1)) / 15.0;
        this->a = hex_to_double(value.substr(4, 1)) / 15.0;
    }
}

bool wf::color_t::is_valid(const std::string& value)
{
    /* Either #RGBA or #RRGGBBAA */
    if (value.size() != 5 && value.size() != 9)
        return false;

    if (value[0] != '#')
        return false;

    const std::string permissible = "0123456789ABCDEF";
    if (value.find_first_not_of(permissible, 1) != std::string::npos)
        return false;

    return true;
}

/* ------------------------- wf::keybinding_t ------------------------------- */
struct general_binding_t
{
    uint32_t mods;
    uint32_t value;
};

/**
 * Split @value at non-empty tokens when encountering any of the characters
 * in @at
 */
static std::vector<std::string> split_at(std::string value, std::string at)
{
    /* Trick: add a delimiter at position 0 and at the end
     * to avoid special casing */
    value = at[0] + value + at[0];

    size_t current = 0;
    std::vector<size_t> split_positions = {0};
    while (current < value.size() - 1)
    {
        size_t next_split = value.find_first_of(at, current + 1);
        split_positions.push_back(next_split);
        current = next_split;
    }

    assert(split_positions.size() >= 2);
    std::vector<std::string> tokens;
    for (size_t i = 1; i < split_positions.size(); i++)
    {
        if (split_positions[i] == split_positions[i - 1] + 1)
            continue; // skip empty tokens

        tokens.push_back(value.substr(split_positions[i - 1] + 1,
                split_positions[i] - split_positions[i - 1] - 1));
    }

    return tokens;
}

static general_binding_t parse_binding(std::string binding_description)
{
    /* Strategy: split the binding at modifier begin/end markings and spaces,
     * and then drop empty tokens. The tokens that are left should be either a
     * binding or something recognizable by evdev. */
    static const std::string delims = "<> \t\n\r\v\b";
    auto tokens = split_at(binding_description, delims);
    if (tokens.empty())
        return {0, 0};

    static std::map<std::string, wf::keyboard_modifier_t> modifier_names =
    {
        {"ctrl", wf::KEYBOARD_MODIFIER_CTRL},
        {"alt", wf::KEYBOARD_MODIFIER_ALT},
        {"shift", wf::KEYBOARD_MODIFIER_SHIFT},
        {"super", wf::KEYBOARD_MODIFIER_LOGO},
    };

    general_binding_t result = {0, 0};
    for (size_t i = 0; i < tokens.size() - 1; i++)
    {
        if (modifier_names.count(tokens[i])) {
            result.mods |= modifier_names[tokens[i]];
        } else {
            return {0, 0}; // invalid modifier
        }
    }

    int code = libevdev_event_code_from_name(EV_KEY, tokens.back().c_str());
    if (code == -1)
        return {0, 0}; // not found

    result.value = code;
    return result;
}

wf::keybinding_t::keybinding_t(uint32_t modifier, uint32_t keyval)
{
    this->mod = modifier;
    this->keyval = keyval;
}

wf::keybinding_t::keybinding_t(const std::string& description)
{
    auto parsed = parse_binding(description);
    this->mod = parsed.mods;
    this->keyval = parsed.value;
}

bool wf::keybinding_t::is_valid(const std::string& value)
{
    auto parsed = parse_binding(value);
    return (parsed.mods != 0) || (parsed.value != 0);
}

bool wf::keybinding_t::operator == (const keybinding_t& other) const
{
    return this->mod == other.mod && this->keyval == other.keyval;
}

/** @return The modifiers of the keybinding */
uint32_t wf::keybinding_t::get_modifiers() const
{
    return this->mod;
}

/** @return The key of the keybinding */
uint32_t wf::keybinding_t::get_key() const
{
    return this->keyval;
}

/* -------------------------- wf::buttonbinding_t --------------------------- */
wf::buttonbinding_t::buttonbinding_t(uint32_t modifier, uint32_t buttonval)
{
    this->mod = modifier;
    this->button = buttonval;
}

wf::buttonbinding_t::buttonbinding_t(const std::string& description)
{
    auto parsed = parse_binding(description);
    this->mod = parsed.mods;
    this->button = parsed.value;
}

bool wf::buttonbinding_t::is_valid(const std::string& value)
{
    auto parsed = parse_binding(value);
    return (parsed.mods != 0) || (parsed.value != 0);
}

bool wf::buttonbinding_t::operator == (const buttonbinding_t& other) const
{
    return this->mod == other.mod && this->button == other.button;
}

uint32_t wf::buttonbinding_t::get_modifiers() const
{
    return this->mod;
}

uint32_t wf::buttonbinding_t::get_button() const
{
    return this->button;
}

wf::touchgesture_t::touchgesture_t(touch_gesture_type_t type, uint32_t direction,
        int finger_count)
{
    this->type = type;
    this->direction = direction;
    this->finger_count = finger_count;
}

static wf::touch_gesture_direction_t parse_single_direction(
    const std::string& direction)
{
    static const std::map<std::string, wf::touch_gesture_direction_t>
        direction_string_map =
        {
            {"up",    wf::GESTURE_DIRECTION_UP},
            {"down",  wf::GESTURE_DIRECTION_DOWN},
            {"left",  wf::GESTURE_DIRECTION_LEFT},
            {"right", wf::GESTURE_DIRECTION_RIGHT}
        };

    if (direction_string_map.count(direction))
        return direction_string_map.at(direction);

    throw std::domain_error("invalid swipe direction");
}

uint32_t parse_direction(const std::string& direction)
{
    size_t hyphen = direction.find("-");
    if (hyphen == std::string::npos)
    {
        return parse_single_direction(direction);
    } else
    {
        /* we support up to 2 directions, because >= 3 will be invalid anyway */
        auto first = direction.substr(0, hyphen);
        auto second = direction.substr(hyphen + 1);

        uint32_t mask =
            parse_single_direction(first) | parse_single_direction(second);

        const uint32_t both_horiz =
            wf::GESTURE_DIRECTION_LEFT | wf::GESTURE_DIRECTION_RIGHT;
        const uint32_t both_vert =
            wf::GESTURE_DIRECTION_UP | wf::GESTURE_DIRECTION_DOWN;

        if (((mask & both_horiz) == both_horiz) ||
            ((mask & both_vert) == both_vert))
        {
            throw std::domain_error("Cannot have two opposing directions in the"
                "same gesture");
        }

        return mask;
    }
}

wf::touchgesture_t parse_gesture(const std::string& value)
{
    if (value == "none" || value.empty())
        return {wf::GESTURE_TYPE_NONE, 0, 0};

    auto tokens = split_at(value, " \t\v\b\n\r");
    assert(!tokens.empty());

    if (tokens.size() != 3)
        return {wf::GESTURE_TYPE_NONE, 0, 0};

    try {
        wf::touch_gesture_type_t type;
        uint32_t direction = 0;
        int32_t finger_count;

        if (tokens[0] == "pinch")
        {
            type = wf::GESTURE_TYPE_PINCH;
            if (tokens[1] == "in") {
                direction = wf::GESTURE_DIRECTION_IN;
            } else if (tokens[1] == "out") {
                direction = wf::GESTURE_DIRECTION_OUT;
            } else {
                throw std::domain_error("Invalid pinch direction: " + tokens[1]);
            }
        }
        else if (tokens[0] == "swipe")
        {
            type = wf::GESTURE_TYPE_SWIPE;
            direction = parse_direction(tokens[1]);
        }
        else if (tokens[0] == "edge-swipe")
        {
            type = wf::GESTURE_TYPE_EDGE_SWIPE;
            direction = parse_direction(tokens[1]);
        }
        else
        {
            throw std::domain_error("Invalid gesture type:" + tokens[0]);
        }

        // TODO: instead of atoi, check properly
        finger_count = std::atoi(tokens[2].c_str());
        return wf::touchgesture_t{type, direction, finger_count};
    } catch (std::exception& e)
    {
        std::cout << "error " << e.what() << std::endl;
        // XXX: show error?
        // ignore it, will return GESTURE_NONE
    }

    return wf::touchgesture_t{wf::GESTURE_TYPE_NONE, 0, 0};
}

wf::touchgesture_t::touchgesture_t(const std::string& description)
{
    *this = parse_gesture(description);
}

bool wf::touchgesture_t::is_valid(const std::string& description)
{
    return parse_gesture(description).get_type() != wf::GESTURE_TYPE_NONE;
}

wf::touch_gesture_type_t wf::touchgesture_t::get_type() const
{
    return this->type;
}

int wf::touchgesture_t::get_finger_count() const
{
    return this->finger_count;
}

uint32_t wf::touchgesture_t::get_direction() const
{
    return this->direction;
}

bool wf::touchgesture_t::operator == (const touchgesture_t& other) const
{
    return type == other.type && finger_count == other.finger_count &&
        (direction == 0 || other.direction == 0 || direction == other.direction);
}
