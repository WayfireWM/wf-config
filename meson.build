project(
	'wf-config',
	'cpp',
	version: '0.10.0',
	license: 'MIT',
	meson_version: '>=0.47.0',
	default_options: [
        'cpp_std=c++17',
		'warning_level=2',
		'werror=false',
	],
)

add_project_arguments(['-Wno-deprecated-declarations'], language: ['cpp'])

glm = dependency('glm', required: false)
if not glm.found() and not meson.get_compiler('cpp').check_header('glm/glm.hpp')
  error('GLM not found, and directly using the header \'glm/glm.hpp\' is not possible.')
endif

evdev = dependency('libevdev')
libxml2 = dependency('libxml-2.0')

sources = [
'src/types.cpp',
'src/option.cpp',
'src/section.cpp',
'src/log.cpp',
'src/xml.cpp',
'src/config-manager.cpp',
'src/file.cpp',
'src/duration.cpp',
'src/compound-option.cpp',
]

wfconfig_inc = include_directories('include')

lib_wfconfig = library('wf-config',
    sources,
    dependencies: [evdev, glm, libxml2],
    include_directories: wfconfig_inc,
    install: true,
    version: meson.project_version(),
    soversion: '1')

pkgconfig = import('pkgconfig')
pkgconfig.generate(
    libraries:    lib_wfconfig,
    version:      meson.project_version(),
    filebase:     meson.project_name(),
    name:         meson.project_name(),
    description: 'Dynamic file-based configuration library for Wayfire')

install_headers([], subdir: 'wayfire/config')

wfconfig = declare_dependency(link_with: lib_wfconfig,
    include_directories: wfconfig_inc,
    dependencies: glm)

# Install headers
subdir('include')

# Unit tests
doctest = dependency('doctest', required: get_option('tests'))

if doctest.found()
    subdir('test')
endif
