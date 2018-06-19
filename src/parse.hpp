#include <string>
#include "config.hpp"

int parse_int(std::string value);
double parse_double(std::string value);

wf_key parse_key(std::string value);
wf_button parse_button(std::string value);
wf_color parse_color(std::string value);

std::string to_string(const wf_key&    key);
std::string to_string(const wf_color&  color);
std::string to_string(const wf_button& button);
