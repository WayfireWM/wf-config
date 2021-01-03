#pragma once

#include <wayfire/config/section.hpp>
#include <libxml/tree.h>
#include <map>

struct wf::config::section_t::impl
{
  public:
    std::map<std::string, std::shared_ptr<option_base_t>> options;
    std::string name;

    // Associated XML node
    xmlNode *xml = NULL;
};
