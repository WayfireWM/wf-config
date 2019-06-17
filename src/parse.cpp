#include "parse.hpp"
#include <libevdev/libevdev.h>
#include <cstring>
#include <sstream>
#include <cassert>
#include <iostream>
#include <map>

int parse_int(const std::string& value)
{
    return std::atoi(value.c_str());
}

double parse_double(const std::string& value)
{
    auto old = std::locale::global(std::locale::classic());
    auto val = std::atof(value.c_str());
    std::locale::global(old);
    return val;
}

uint32_t extract_modifiers(const std::vector<std::string>& tokens)
{
    uint32_t mods = 0;
    for (auto token : tokens)
    {
        if (token == "<alt>")
            mods |= WF_MODIFIER_ALT;
        else if (token == "<ctrl>")
            mods |= WF_MODIFIER_CTRL;
        else if (token == "<shift>")
            mods |= WF_MODIFIER_SHIFT;
        else if (token == "<super>")
            mods |= WF_MODIFIER_LOGO;
    }

    return mods;
}

std::vector<std::string> tokenize(const std::string& value)
{
    std::stringstream ss(value);

    std::vector<std::string> tokens;
    std::string token;

    while(ss >> token)
        tokens.push_back(token);

    return tokens;
}

wf_key parse_key(const std::string& value)
{
    if (value == "none" || value.empty())
        return {0, 0};

    wf_key ans;
    auto tokens = tokenize(value);

    ans.mod = extract_modifiers(tokens);
    ans.keyval = libevdev_event_code_from_name(EV_KEY, tokens.back().c_str());

    if (ans.keyval == (uint32_t)-1)
        ans.keyval = 0;

    return ans;
}

wf_button parse_button(const std::string& value)
{
    if (value == "none" || value.empty())
        return {0, 0};

    auto tokens = tokenize(value);
    wf_button ans = {0, 0};

    ans.mod = extract_modifiers(tokens);

    ans.button = libevdev_event_code_from_name(EV_KEY, tokens.back().c_str());

    if (ans.button == (uint32_t)-1)
        ans.button = 0;

    return ans;
}

std::map<uint32_t, std::string> direction_string_map = {
    {GESTURE_DIRECTION_UP, "up"},
    {GESTURE_DIRECTION_DOWN, "down"},
    {GESTURE_DIRECTION_LEFT, "left"},
    {GESTURE_DIRECTION_RIGHT, "right"}
};

uint32_t parse_single_direction(const std::string& direction)
{
    for (auto& kv : direction_string_map)
    {
        if (kv.second == direction)
            return kv.first;
    }

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

        return parse_single_direction(first) | parse_single_direction(second);
    }
}

wf_touch_gesture parse_gesture(const std::string& value)
{
    if (value == "none" || value.empty())
        return {GESTURE_NONE, 0, 0};

    auto tokens = tokenize(value);
    assert(!tokens.empty());

    try {
        wf_touch_gesture gesture;
        if (tokens[0] == "pinch")
        {
            // we allow for garbage afterwards, same for other gestures
            assert(tokens.size() >= 3);

            gesture.type = GESTURE_PINCH;
            gesture.direction = (tokens[1] == "in" ?
                GESTURE_DIRECTION_IN : GESTURE_DIRECTION_OUT);
            gesture.finger_count = parse_int(tokens[2]);

            return gesture;
        }
        else if (tokens[0] == "swipe" || tokens[0] == "edge-swipe")
        {
            assert(tokens.size() >= 3);

            gesture.type = (tokens[0] == "swipe" ?
                GESTURE_SWIPE : GESTURE_EDGE_SWIPE);
            gesture.direction = parse_direction(tokens[1]);
            gesture.finger_count = parse_int(tokens[2]);

            return gesture;
        }
    } catch (...)
    {
        // ignore it, will return GESTURE_NONE
    }

    return {GESTURE_NONE, 0, 0};
}

wf_color parse_color(const std::string& value)
{
    wf_color ans = {0, 0, 0, 0};
    std::stringstream ss(value);

    auto old = std::locale::global(std::locale::classic());
    ss >> ans.r >> ans.g >> ans.b >> ans.a;
    std::locale::global(old);


    return ans;
}

std::string mods_to_string(uint32_t mods)
{
    std::string result;
    if (mods & WF_MODIFIER_ALT)
        result += "<alt> ";
    if (mods & WF_MODIFIER_CTRL)
        result += "<ctrl> ";
    if (mods & WF_MODIFIER_LOGO)
        result += "<super> ";
    if (mods & WF_MODIFIER_SHIFT)
        result += "<shift> ";

    return result;
}

std::string to_string(const wf_key& key)
{
    return mods_to_string(key.mod) +
        libevdev_event_code_get_name(EV_KEY, key.keyval);
}

std::string to_string(const wf_color& color)
{
    auto old = std::locale::global(std::locale::classic());

    auto conv = std::to_string(color.r) + " " + std::to_string(color.g) + " "
              + std::to_string(color.b) + " " + std::to_string(color.a);

    std::locale::global(old);

    return conv;
}

std::string to_string(const wf_button& button)
{
    return mods_to_string(button.mod) +
        libevdev_event_code_get_name(EV_KEY, button.button);
}

static std::string direction_to_string(uint32_t direction)
{
    if (__builtin_popcount(direction) == 1)
    {
        if (direction_string_map.count(direction))
            return direction_string_map[direction];

        throw std::domain_error("unknown swipe direction");
    } else
    {
        uint32_t bit1 = direction & (direction - 1);
        uint32_t bit2 = direction ^ bit1;

        return direction_to_string(bit1) + "-" + direction_to_string(bit2);
    }
}

std::string to_string(const wf_touch_gesture& gesture)
{
    if (gesture.type == GESTURE_PINCH)
    {
        return std::string("pinch ")
            + (gesture.direction == GESTURE_DIRECTION_IN ? "in" : "out")
            + " " + std::to_string(gesture.finger_count);
    }

    if (gesture.type == GESTURE_SWIPE)
    {
        return std::string("swipe ")
            + direction_to_string(gesture.direction)
            + " " + std::to_string(gesture.finger_count);
    }

    if (gesture.type == GESTURE_EDGE_SWIPE)
    {
        return std::string("edge-swipe ")
            + direction_to_string(gesture.direction)
            + " " + std::to_string(gesture.finger_count);
    }

    throw new std::domain_error("unsupported gesture type");
}
