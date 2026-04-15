test_signing = executable('test_signing',
  'test_signing.cpp',
  dependencies: [ingot_dep, doctest_dep],
)

test('signing', test_signing)

# Cross-validation binary (used by the viem comparison test)
executable('test_cross_validate',
  'test_cross_validate.cpp',
  dependencies: [ingot_dep],
)
