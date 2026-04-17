test_signing = executable('test_signing',
  'test_signing.cpp',
  dependencies: [ingot_dep, doctest_dep],
)

test('signing', test_signing)

test_units = executable('test_units',
  'test_units.cpp',
  dependencies: [ingot_dep, doctest_dep],
)

test('units', test_units)

# Cross-validation binary (used by the viem comparison test)
executable('test_cross_validate',
  'test_cross_validate.cpp',
  dependencies: [ingot_dep],
)
