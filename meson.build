project('ingot', 'cpp', 'c',
  version: '0.1.0',
  default_options: [
    'cpp_std=c++20',
    'c_std=c11',
    'warning_level=3',
  ],
)

# Dependencies
secp256k1_dep = dependency('libsecp256k1',
  fallback: ['libsecp256k1', 'libsecp256k1_dep'],
)
doctest_dep = dependency('doctest', required: true)

# Library
ingot_inc = include_directories('include')

ingot_lib = static_library('ingot',
  'src/keccak.cpp',
  include_directories: ingot_inc,
  dependencies: [secp256k1_dep],
)

ingot_dep = declare_dependency(
  link_with: ingot_lib,
  include_directories: ingot_inc,
  dependencies: [secp256k1_dep],
)

# Tests
subdir('tests')
