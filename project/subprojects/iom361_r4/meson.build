project('iom361_r4', 'c',
    version : '4.0',
    default_options : ['c_std=c11'],
)

iom361_lib = static_library('iom361_r4', 'iom361_r4.c')

iom361_dep = declare_dependency(
    link_with : iom361_lib,
    include_directories : include_directories('.'),
)
meson.override_dependency('iom361_r4', iom361_dep)
