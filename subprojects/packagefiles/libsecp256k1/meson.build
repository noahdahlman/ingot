project('libsecp256k1', 'c',
  version: '0.6.0',
  default_options: ['c_std=c11'],
)

cc = meson.get_compiler('c')

secp256k1_defines = [
  '-DENABLE_MODULE_RECOVERY=1',
  '-DENABLE_MODULE_ECDH=1',
]

# Check for __int128 support
if cc.has_type('__int128')
  secp256k1_defines += '-DHAVE___INT128=1'
endif

secp256k1_inc = include_directories('include', 'src')

secp256k1_lib = static_library('secp256k1',
  'src/secp256k1.c',
  'src/precomputed_ecmult.c',
  'src/precomputed_ecmult_gen.c',
  include_directories: secp256k1_inc,
  c_args: secp256k1_defines,
)

libsecp256k1_dep = declare_dependency(
  link_with: secp256k1_lib,
  include_directories: include_directories('include'),
)
