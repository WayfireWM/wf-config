#include "parse.hpp"
#include <libevdev/libevdev.h>
#include <cstring>
#include <sstream>
#include <iostream>

int parse_int(std::string value)
{
    return std::atoi(value.c_str());
}

double parse_double(std::string value)
{
    return std::atof(value.c_str());
}

uint32_t extract_modifiers(std::vector<std::string> tokens)
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

std::vector<std::string> tokenize(std::string value)
{
    std::stringstream ss(value);

    std::vector<std::string> tokens;
    std::string token;

    while(ss >> token)
        tokens.push_back(token);

    return tokens;
}

wf_key parse_key(std::string value)
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

wf_button parse_button(std::string value)
{
    if (value == "none" || value.empty())
        return {0, 0};

    auto tokens = tokenize(value);
    wf_button ans = {0, 0};

    ans.mod = extract_modifiers(tokens);
    auto button = tokens.back();

    ans.button = libevdev_event_code_from_name(EV_KEY, tokens.back().c_str());

    if (ans.button == (uint32_t)-1)
        ans.button = 0;

    return ans;
}

wf_color parse_color(std::string value)
{
    wf_color ans = {0, 0, 0, 0};
    std::stringstream ss(value);
    ss >> ans.r >> ans.g >> ans.b >> ans.a;

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
