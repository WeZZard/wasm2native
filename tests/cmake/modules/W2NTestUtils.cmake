# W2NTestUtils.cmake
#
# Utility functions for wasm2native testing targets

# Get the possible build flavors for testing
function(get_w2n_test_build_flavors build_flavors_out_var sdk)
  set(build_flavors "default")
  set(${build_flavors_out_var} ${build_flavors} PARENT_SCOPE)
endfunction()

# Get the variant suffix for test targets and folders
function(get_w2n_test_variant_suffix variant_suffix_out_var sdk arch build_flavor)
  set(variant_suffix "-${W2N_SDK_${sdk}_LIB_SUBDIR}-${arch}")
  set(${variant_suffix_out_var} "${variant_suffix}" PARENT_SCOPE)
endfunction()

# Get the variant triple for test targets
function(get_w2n_test_versioned_target_triple variant_triple_out_var sdk arch build_flavor)
  get_versioned_target_triple(variant_triple ${sdk} ${arch} "${W2N_SDK_${sdk}_DEPLOYMENT_VERSION}")
  set(${variant_triple_out_var} "${variant_triple}" PARENT_SCOPE)
endfunction()
