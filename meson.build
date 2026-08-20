project('toys', 'c', version: '0.1', meson_version: '>= 1.1.0')

add_project_arguments('-Wall', '-Wextra', language: 'c')

if get_option('debug')
    add_project_arguments('-DTOYS_DEBUG', language: 'c')
endif

inc = include_directories(
    'include',
)

cc = meson.get_compiler('c')

subdir('include')
subdir('src')

if get_option('tests')
    subdir('tests')
endif

if get_option('examples')
    subdir('examples')
endif
