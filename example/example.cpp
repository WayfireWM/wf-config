/* This example shows the basic usage of the library. Compile with:
 *
 * g++ example.cpp -o example `pkg-config --cflags --libs wf-config`
 *
 * after you've installed wf-config.
 *
 * It first prints several values from the config file, then adds a new section
 * and several options and * writes the updated config to a new file test2.ini */

#include <iostream>
#include "config.hpp"

const char *file = "test.ini";

int main()
{
    /* initialize and load the configuration from file */
    auto conf = new wayfire_config(file);

    /* return a pointer to the section with name "section1" */
    auto section1 = conf->get_section("section1");

    /* return a pointer to the section with name "section2" */
    auto section2 = conf->get_section("section2");

    /* get handles to the options in the corresponding sections.
     * if they don't exist, they will be created */
    auto opt1_sect1 = section1->get_option("option1", "");
    auto opt2_sect1 = section1->get_option("option2", "0");
    auto opt3_sect1 = section1->get_option("option3", "0 0 0 0");

    auto opt1_sect2 = section2->get_option("option1", "");

    /* use the option::as_* methods to get the value of the string in the appropriate format */
    std::cout << "option option1 in section1: " << opt1_sect1->as_string() << std::endl;
    std::cout << "option option3 in section1 green value: " << opt1_sect1->as_color().g << std::endl;

    auto key = opt1_sect1->as_key();
    std::cout << "option option1 in section2: (" << key.mod << ", " << key.keyval << ")" << std::endl;

    /* set the value of an option */
    opt2_sect1->set_value("50");
    opt1_sect2->set_value("<super> <shift> KEY_L");

    /* Write the all the sections and their options to the given file,
     * erasing all previous contents.
     *
     * It will skip options that were programatically added by the library user
     * and have their default values */
    conf->save_config("test2.ini");
}
