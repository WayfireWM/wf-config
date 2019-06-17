#include <string>
#include "config.hpp"

int parse_int(const std::string& value);
double parse_double(const std::string& value);

wf_key parse_key(const std::string& value);
wf_button parse_button(const std::string& value);
wf_touch_gesture parse_gesture(const std::string& value);
wf_color parse_color(const std::string& value);

std::string to_string(const wf_key&    key);
std::string to_string(const wf_color&  color);
std::string to_string(const wf_button& button);
std::string to_string(const wf_touch_gesture& gesture);
