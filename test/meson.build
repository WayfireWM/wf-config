types_test = executable(
    'types_test',
    'types_test.cpp',
    dependencies: wfconfig,
    install: false)
test('Types test', types_test)

option_base_test = executable(
    'option_base_test',
    'option_base_test.cpp',
    dependencies: wfconfig,
    install: false)
test('OptionBase test', option_base_test)

option_test = executable(
    'option_test',
    'option_test.cpp',
    dependencies: wfconfig,
    install: false)
test('Option test', option_test)

option_wrapper_test = executable(
    'option_wrapper_test',
    'option_wrapper_test.cpp',
    dependencies: wfconfig,
    install: false)
test('Option wrapper test', option_wrapper_test)

section_test = executable(
    'section_test',
    'section_test.cpp',
    dependencies: wfconfig,
    install: false)
test('Section test', section_test)

xml_test = executable(
    'xml_test',
    'xml_test.cpp',
    include_directories: wfconfig_inc,
    dependencies: [wfconfig, libxml2],
    install: false)
test('XML test', xml_test)

config_manager_test = executable(
    'config_manager_test',
    'config_manager_test.cpp',
    dependencies: wfconfig,
    install: false)
test('ConfigManager test', config_manager_test)

file_parse_test = executable(
    'file_test',
    'file_test.cpp',
    dependencies: wfconfig,
    install: false)
test('File parsing test', file_parse_test)

# Utils
log_test = executable(
    'log_test',
    'log_test.cpp',
    dependencies: wfconfig,
    install: false)
test('Log test', log_test)

duration_test = executable(
    'duration_test',
    'duration_test.cpp',
    dependencies: wfconfig,
    install: false)
test('Duration test', duration_test)
