option(W2N_ENABLE_IOS32
  "Build 32-bit variants of iOS"
  TRUE)

if(W2N_ENABLE_IOS32)
  set(SUPPORTED_IOS_ARCHS "arm64;arm64e")
  set(SUPPORTED_IOS_SIMULATOR_ARCHS "x86_64;arm64")
else()
  set(SUPPORTED_IOS_ARCHS "arm64;arm64e")
  set(SUPPORTED_IOS_SIMULATOR_ARCHS "x86_64;arm64")
endif()

set(SUPPORTED_TVOS_ARCHS "arm64")
set(SUPPORTED_TVOS_SIMULATOR_ARCHS "x86_64;arm64")
set(SUPPORTED_WATCHOS_ARCHS "armv7k;arm64_32")
set(SUPPORTED_WATCHOS_SIMULATOR_ARCHS "i386;x86_64;arm64")
set(SUPPORTED_OSX_ARCHS "x86_64;arm64")

is_sdk_requested(OSX w2n_build_osx)

if(w2n_build_osx)
  configure_sdk_darwin(
    OSX "OS X" "${W2N_DARWIN_DEPLOYMENT_VERSION_OSX}"
    macosx macosx macos "${SUPPORTED_OSX_ARCHS}")
  configure_target_variant(OSX-DA "OS X Debug+Asserts" OSX DA "Debug+Asserts")
  configure_target_variant(OSX-RA "OS X Release+Asserts" OSX RA "Release+Asserts")
  configure_target_variant(OSX-R "OS X Release" OSX R "Release")
endif()

is_sdk_requested(FREESTANDING w2n_build_freestanding)

if(w2n_build_freestanding AND(W2N_FREESTANDING_FLAVOR STREQUAL "apple"))
  set(W2N_FREESTANDING_SDK "" CACHE STRING
    "Which SDK to use when building the FREESTANDING stdlib")
  set(W2N_FREESTANDING_TRIPLE_NAME "" CACHE STRING
    "Which triple name (e.g. 'none-macho') to use when building the FREESTANDING stdlib")
  set(W2N_FREESTANDING_MODULE_NAME "" CACHE STRING
    "Which .w2nmodule name (e.g. 'freestanding') to use when building the FREESTANDING stdlib")
  set(W2N_FREESTANDING_ARCHS "" CACHE STRING
    "Which architectures to build when building the FREESTANDING stdlib")
  configure_sdk_darwin(
    FREESTANDING "FREESTANDING" ""
    "${W2N_FREESTANDING_SDK}"
    "${W2N_FREESTANDING_TRIPLE_NAME}" "${W2N_FREESTANDING_MODULE_NAME}" "${W2N_FREESTANDING_ARCHS}")
  set(W2N_SDK_FREESTANDING_LIB_SUBDIR "freestanding")
  configure_target_variant(FREESTANDING-DA "FREESTANDING Debug+Asserts" FREESTANDING DA "Debug+Asserts")
  configure_target_variant(FREESTANDING-RA "FREESTANDING Release+Asserts" FREESTANDING RA "Release+Asserts")
  configure_target_variant(FREESTANDING-R "FREESTANDING Release" FREESTANDING R "Release")
  configure_target_variant(FREESTANDING-S "FREESTANDING MinSizeRelease" FREESTANDING S "MinSizeRelease")

  set(W2N_FREESTANDING_TEST_DEPENDENCIES "Darwin")
endif()

# Compatible cross-compile SDKS for Darwin OSes: IOS, IOS_SIMULATOR, TVOS,
# TVOS_SIMULATOR, WATCHOS, WATCHOS_SIMULATOR (archs hardcoded below).
is_sdk_requested(IOS w2n_build_ios)

if(w2n_build_ios)
  configure_sdk_darwin(
    IOS "iOS" "${W2N_DARWIN_DEPLOYMENT_VERSION_IOS}"
    iphoneos ios ios "${SUPPORTED_IOS_ARCHS}")
  configure_target_variant(IOS-DA "iOS Debug+Asserts" IOS DA "Debug+Asserts")
  configure_target_variant(IOS-RA "iOS Release+Asserts" IOS RA "Release+Asserts")
  configure_target_variant(IOS-R "iOS Release" IOS R "Release")
endif()

is_sdk_requested(IOS_SIMULATOR w2n_build_ios_simulator)

if(w2n_build_ios_simulator)
  configure_sdk_darwin(
    IOS_SIMULATOR "iOS Simulator" "${W2N_DARWIN_DEPLOYMENT_VERSION_IOS}"
    iphonesimulator ios ios-simulator
    "${SUPPORTED_IOS_SIMULATOR_ARCHS}")
  configure_target_variant(
    IOS_SIMULATOR-DA "iOS Debug+Asserts" IOS_SIMULATOR DA "Debug+Asserts")
  configure_target_variant(
    IOS_SIMULATOR-RA "iOS Release+Asserts" IOS_SIMULATOR RA "Release+Asserts")
  configure_target_variant(
    IOS_SIMULATOR-R "iOS Release" IOS_SIMULATOR R "Release")
endif()

is_sdk_requested(TVOS w2n_build_tvos)

if(w2n_build_tvos)
  configure_sdk_darwin(
    TVOS "tvOS" "${W2N_DARWIN_DEPLOYMENT_VERSION_TVOS}"
    appletvos tvos tvos "${SUPPORTED_TVOS_ARCHS}")
  configure_target_variant(TVOS-DA "tvOS Debug+Asserts" TVOS DA "Debug+Asserts")
  configure_target_variant(TVOS-RA "tvOS Release+Asserts" TVOS RA "Release+Asserts")
  configure_target_variant(TVOS-R "tvOS Release" TVOS R "Release")
endif()

is_sdk_requested(TVOS_SIMULATOR w2n_build_tvos_simulator)

if(w2n_build_tvos_simulator)
  configure_sdk_darwin(
    TVOS_SIMULATOR "tvOS Simulator" "${W2N_DARWIN_DEPLOYMENT_VERSION_TVOS}"
    appletvsimulator tvos tvos-simulator
    "${SUPPORTED_TVOS_SIMULATOR_ARCHS}")
  configure_target_variant(
    TVOS_SIMULATOR-DA "tvOS Debug+Asserts" TVOS_SIMULATOR DA "Debug+Asserts")
  configure_target_variant(
    TVOS_SIMULATOR-RA "tvOS Release+Asserts" TVOS_SIMULATOR RA "Release+Asserts")
  configure_target_variant(
    TVOS_SIMULATOR-R "tvOS Release" TVOS_SIMULATOR R "Release")
endif()

is_sdk_requested(WATCHOS w2n_build_watchos)

if(w2n_build_watchos)
  configure_sdk_darwin(
    WATCHOS "watchOS" "${W2N_DARWIN_DEPLOYMENT_VERSION_WATCHOS}"
    watchos watchos watchos "${SUPPORTED_WATCHOS_ARCHS}")
  configure_target_variant(WATCHOS-DA "watchOS Debug+Asserts" WATCHOS DA "Debug+Asserts")
  configure_target_variant(WATCHOS-RA "watchOS Release+Asserts" WATCHOS RA "Release+Asserts")
  configure_target_variant(WATCHOS-R "watchOS Release" WATCHOS R "Release")
endif()

is_sdk_requested(WATCHOS_SIMULATOR w2n_build_watchos_simulator)

if(w2n_build_watchos_simulator)
  configure_sdk_darwin(
    WATCHOS_SIMULATOR "watchOS Simulator" "${W2N_DARWIN_DEPLOYMENT_VERSION_WATCHOS}"
    watchsimulator watchos watchos-simulator
    "${SUPPORTED_WATCHOS_SIMULATOR_ARCHS}")
  configure_target_variant(WATCHOS_SIMULATOR-DA "watchOS Debug+Asserts" WATCHOS_SIMULATOR DA "Debug+Asserts")
  configure_target_variant(WATCHOS_SIMULATOR-RA "watchOS Release+Asserts" WATCHOS_SIMULATOR RA "Release+Asserts")
  configure_target_variant(WATCHOS_SIMULATOR-R "watchOS Release" WATCHOS_SIMULATOR R "Release")
endif()
