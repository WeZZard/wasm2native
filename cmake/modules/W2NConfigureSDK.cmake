# Variable that tracks the set of configured SDKs.
#
# Each element in this list is an SDK for which the various
# W2N_SDK_${name}_* variables are defined. wasm2native libraries will be
# built for each variant.
set(W2N_CONFIGURED_SDKS)

# Report the given SDK to the user.
function(_report_sdk prefix)
  message(STATUS "${W2N_SDK_${prefix}_NAME} SDK:")
  message(STATUS "  Object File Format: ${W2N_SDK_${prefix}_OBJECT_FORMAT}")
  message(STATUS "  Swift Standard Library Path: ${W2N_SDK_${prefix}_LIB_SUBDIR}")
  message(STATUS "  Threading Package: ${W2N_SDK_${prefix}_THREADING_PACKAGE}")

  if(prefix IN_LIST W2N_DARWIN_PLATFORMS)
    message(STATUS "  Version: ${W2N_SDK_${prefix}_VERSION}")
    message(STATUS "  Build number: ${W2N_SDK_${prefix}_BUILD_NUMBER}")
    message(STATUS "  Deployment version: ${W2N_SDK_${prefix}_DEPLOYMENT_VERSION}")
    message(STATUS "  Triple name: ${W2N_SDK_${prefix}_TRIPLE_NAME}")
    message(STATUS "  Simulator: ${W2N_SDK_${prefix}_IS_SIMULATOR}")
  endif()

  if(W2N_SDK_${prefix}_MODULE_ARCHITECTURES)
    message(STATUS "  Module Architectures: ${W2N_SDK_${prefix}_MODULE_ARCHITECTURES}")
  endif()

  message(STATUS "  Architectures: ${W2N_SDK_${prefix}_ARCHITECTURES}")

  foreach(arch ${W2N_SDK_${prefix}_ARCHITECTURES})
    message(STATUS "  ${arch} triple: ${W2N_SDK_${prefix}_ARCH_${arch}_TRIPLE}")
    message(STATUS "  Module triple: ${W2N_SDK_${prefix}_ARCH_${arch}_MODULE}")
  endforeach()

  foreach(arch ${W2N_SDK_${prefix}_ARCHITECTURES})
    message(STATUS "  ${arch} Path: ${W2N_SDK_${prefix}_ARCH_${arch}_PATH}")
  endforeach()

  message(STATUS "")
endfunction()

# Remove architectures not supported by the SDK from the given list.
function(remove_sdk_unsupported_archs name os sdk_path architectures_var)
  execute_process(COMMAND
    /usr/libexec/PlistBuddy -c "Print :SupportedTargets:${os}:Archs" ${sdk_path}/SDKSettings.plist
    OUTPUT_VARIABLE sdk_supported_archs
    RESULT_VARIABLE plist_error)

  if(NOT plist_error EQUAL 0)
    message(STATUS "${os} SDK at ${sdk_path} does not publish its supported architectures")
    return()
  endif()

  set(architectures)

  foreach(arch ${${architectures_var}})
    if(sdk_supported_archs MATCHES "${arch}\n")
      list(APPEND architectures ${arch})
    elseif(arch STREQUAL "i386" AND os STREQUAL "iphonesimulator")
      # 32-bit iOS simulator is not listed explicitly in SDK settings.
      message(STATUS "Assuming ${name} SDK at ${sdk_path} supports architecture ${arch}")
      list(APPEND architectures ${arch})
    elseif(arch STREQUAL "armv7k" AND os STREQUAL "watchos")
      # 32-bit watchOS is not listed explicitly in SDK settings.
      message(STATUS "Assuming ${name} SDK at ${sdk_path} supports architecture ${arch}")
      list(APPEND architectures ${arch})
    elseif(arch STREQUAL "i386" AND os STREQUAL "watchsimulator")
      # 32-bit watchOS simulator is not listed explicitly in SDK settings.
      message(STATUS "Assuming ${name} SDK at ${sdk_path} supports architecture ${arch}")
      list(APPEND architectures ${arch})
    else()
      message(STATUS "${name} SDK at ${sdk_path} does not support architecture ${arch}")
    endif()
  endforeach()

  set("${architectures_var}" ${architectures} PARENT_SCOPE)
endfunction()

# Work out which threading package to use by consulting W2N_THREADING_PACKAGE
function(get_threading_package sdk default package_var)
  set(global_override)

  foreach(elt ${W2N_THREADING_PACKAGE})
    string(REPLACE ":" ";" elt_list "${elt}")
    list(LENGTH elt_list elt_list_len)

    if(elt_list_len EQUAL 1)
      list(GET elt_list 0 global_override)
      string(TOLOWER "${global_override}" global_override)
    else()
      list(GET elt_list 0 elt_sdk)
      list(GET elt_list 1 elt_package)
      string(TOUPPER "${elt_sdk}" elt_sdk)
      string(TOLOWER "${elt_package}" elt_package)

      if("${elt_sdk}" STREQUAL "${sdk}")
        set("${package_var}" "${elt_package}" PARENT_SCOPE)
        return()
      endif()
    endif()
  endforeach()

  if(global_override)
    set("${package_var}" "${global_override}" PARENT_SCOPE)
  else()
    set("${package_var}" "${default}" PARENT_SCOPE)
  endif()
endfunction()

# Configure an SDK
#
# Usage:
# configure_sdk_darwin(
# prefix             # Prefix to use for SDK variables (e.g., OSX)
# name               # Display name for this SDK
# deployment_version # Deployment version
# xcrun_name         # SDK name to use with xcrun
# triple_name        # The name used in Swift's -triple
# architectures      # A list of architectures this SDK supports
# )
#
# Sadly there are three OS naming conventions.
# xcrun SDK name:   macosx iphoneos iphonesimulator (+ version)
# -mOS-version-min: macosx ios      ios-simulator
# swift -triple:    macosx ios      ios
#
# This macro attempts to configure a given SDK. When successful, it
# defines a number of variables:
#
# W2N_SDK_${prefix}_NAME                    Display name for the SDK
# W2N_SDK_${prefix}_VERSION                 SDK version number (e.g., 10.9, 7.0)
# W2N_SDK_${prefix}_BUILD_NUMBER            SDK build number (e.g., 14A389a)
# W2N_SDK_${prefix}_DEPLOYMENT_VERSION      Deployment version (e.g., 10.9, 7.0)
# W2N_SDK_${prefix}_LIB_SUBDIR              Library subdir for this SDK
# W2N_SDK_${prefix}_TRIPLE_NAME             Triple name for this SDK
# W2N_SDK_${prefix}_OBJECT_FORMAT           The object file format (e.g. MACHO)
# W2N_SDK_${prefix}_USE_ISYSROOT            Whether to use -isysroot
# W2N_SDK_${prefix}_SHARED_LIBRARY_PREFIX   Shared library prefix for this SDK (e.g. 'lib')
# W2N_SDK_${prefix}_SHARED_LIBRARY_SUFFIX   Shared library suffix for this SDK (e.g. 'dylib')
# W2N_SDK_${prefix}_ARCHITECTURES           Architectures (as a list)
# W2N_SDK_${prefix}_IS_SIMULATOR            Whether this is a simulator target.
# W2N_SDK_${prefix}_ARCH_${ARCH}_TRIPLE     Triple name
# W2N_SDK_${prefix}_ARCH_${ARCH}_MODULE     Module triple name for this SDK
macro(configure_sdk_darwin
  prefix name deployment_version xcrun_name
  triple_name module_name architectures)
  # Note: this has to be implemented as a macro because it sets global
  # variables.

  # Find the SDK
  set(W2N_SDK_${prefix}_PATH "" CACHE PATH "Path to the ${name} SDK")

  if(NOT W2N_SDK_${prefix}_PATH)
    execute_process(
      COMMAND "xcrun" "--sdk" "${xcrun_name}" "--show-sdk-path"
      OUTPUT_VARIABLE W2N_SDK_${prefix}_PATH
      OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif()

  if(NOT EXISTS "${W2N_SDK_${prefix}_PATH}/SDKSettings.plist")
    message(FATAL_ERROR "${name} SDK not found at W2N_SDK_${prefix}_PATH.")
  endif()

  # Determine the SDK version we found.
  execute_process(
    COMMAND "defaults" "read" "${W2N_SDK_${prefix}_PATH}/SDKSettings.plist" "Version"
    OUTPUT_VARIABLE W2N_SDK_${prefix}_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  execute_process(
    COMMAND "xcodebuild" "-sdk" "${W2N_SDK_${prefix}_PATH}" "-version" "ProductBuildVersion"
    OUTPUT_VARIABLE W2N_SDK_${prefix}_BUILD_NUMBER
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  # Set other variables.
  set(W2N_SDK_${prefix}_NAME "${name}")
  set(W2N_SDK_${prefix}_DEPLOYMENT_VERSION "${deployment_version}")
  set(W2N_SDK_${prefix}_LIB_SUBDIR "${xcrun_name}")
  set(W2N_SDK_${prefix}_TRIPLE_NAME "${triple_name}")
  set(W2N_SDK_${prefix}_OBJECT_FORMAT "MACHO")
  set(W2N_SDK_${prefix}_USE_ISYSROOT TRUE)
  set(W2N_SDK_${prefix}_SHARED_LIBRARY_PREFIX "lib")
  set(W2N_SDK_${prefix}_SHARED_LIBRARY_SUFFIX ".dylib")
  set(W2N_SDK_${prefix}_STATIC_LIBRARY_PREFIX "lib")
  set(W2N_SDK_${prefix}_STATIC_LIBRARY_SUFFIX ".a")
  set(W2N_SDK_${prefix}_IMPORT_LIBRARY_PREFIX "")
  set(W2N_SDK_${prefix}_IMPORT_LIBRARY_SUFFIX "")
  get_threading_package(${prefix} "darwin" W2N_SDK_${prefix}_THREADING_PACKAGE)

  set(W2N_SDK_${prefix}_ARCHITECTURES ${architectures})

  if(W2N_DARWIN_SUPPORTED_ARCHS)
    list_intersect(
      "${architectures}" # lhs
      "${W2N_DARWIN_SUPPORTED_ARCHS}" # rhs
      W2N_SDK_${prefix}_ARCHITECTURES) # result
  endif()

  # Remove any architectures not supported by the SDK.
  remove_sdk_unsupported_archs(${name} ${xcrun_name} ${W2N_SDK_${prefix}_PATH} W2N_SDK_${prefix}_ARCHITECTURES)

  list_intersect(
    "${W2N_DARWIN_MODULE_ARCHS}" # lhs
    "${architectures}" # rhs
    W2N_SDK_${prefix}_MODULE_ARCHITECTURES) # result

  # Ensure the architectures and module-only architectures lists are mutually
  # exclusive.
  list_subtract(
    "${W2N_SDK_${prefix}_MODULE_ARCHITECTURES}" # lhs
    "${W2N_SDK_${prefix}_ARCHITECTURES}" # rhs
    W2N_SDK_${prefix}_MODULE_ARCHITECTURES) # result

  # Determine whether this is a simulator SDK.
  if(${xcrun_name} MATCHES "simulator")
    set(W2N_SDK_${prefix}_IS_SIMULATOR TRUE)
  else()
    set(W2N_SDK_${prefix}_IS_SIMULATOR FALSE)
  endif()

  # Configure variables for _all_ architectures even if we aren't "building"
  # them because they aren't supported.
  foreach(arch ${architectures})
    # On Darwin, all archs share the same SDK path.
    set(W2N_SDK_${prefix}_ARCH_${arch}_PATH "${W2N_SDK_${prefix}_PATH}")

    set(W2N_SDK_${prefix}_ARCH_${arch}_TRIPLE
      "${arch}-apple-${W2N_SDK_${prefix}_TRIPLE_NAME}")

    set(W2N_SDK_${prefix}_ARCH_${arch}_MODULE
      "${arch}-apple-${module_name}")

    # If this is a simulator target, append -simulator.
    if(W2N_SDK_${prefix}_IS_SIMULATOR)
      set(W2N_SDK_${prefix}_ARCH_${arch}_TRIPLE
        "${W2N_SDK_${prefix}_ARCH_${arch}_TRIPLE}-simulator")
    endif()

    if(W2N_ENABLE_MACCATALYST AND "${prefix}" STREQUAL "OSX")
      # For macCatalyst append the '-macabi' environment to the target triple.
      set(W2N_SDK_MACCATALYST_ARCH_${arch}_TRIPLE "${arch}-apple-ios-macabi")
      set(W2N_SDK_MACCATALYST_ARCH_${arch}_MODULE "${arch}-apple-ios-macabi")

      # For macCatalyst, the xcrun_name is "macosx" since it uses that sdk.
      # Hard code the library subdirectory to "maccatalyst" in that case.
      set(W2N_SDK_MACCATALYST_LIB_SUBDIR "maccatalyst")
    endif()
  endforeach()

  # Add this to the list of known SDKs.
  list(APPEND W2N_CONFIGURED_SDKS "${prefix}")

  _report_sdk("${prefix}")
endmacro()

macro(configure_sdk_unix name architectures)
  # Note: this has to be implemented as a macro because it sets global
  # variables.
  string(TOUPPER ${name} prefix)
  string(TOLOWER ${name} platform)

  set(W2N_SDK_${prefix}_NAME "${name}")
  set(W2N_SDK_${prefix}_LIB_SUBDIR "${platform}")
  set(W2N_SDK_${prefix}_ARCHITECTURES "${architectures}")

  if("${prefix}" STREQUAL "CYGWIN")
    set(W2N_SDK_${prefix}_OBJECT_FORMAT "COFF")
    set(W2N_SDK_${prefix}_SHARED_LIBRARY_PREFIX "")
    set(W2N_SDK_${prefix}_SHARED_LIBRARY_SUFFIX ".dll")
    set(W2N_SDK_${prefix}_STATIC_LIBRARY_PREFIX "")
    set(W2N_SDK_${prefix}_STATIC_LIBRARY_SUFFIX ".lib")
    set(W2N_SDK_${prefix}_IMPORT_LIBRARY_PREFIX "")
    set(W2N_SDK_${prefix}_IMPORT_LIBRARY_SUFFIX ".lib")
  elseif("${prefix}" STREQUAL "WASI")
    set(W2N_SDK_${prefix}_OBJECT_FORMAT "WASM")
    set(W2N_SDK_${prefix}_SHARED_LIBRARY_PREFIX "")
    set(W2N_SDK_${prefix}_SHARED_LIBRARY_SUFFIX ".wasm")
    set(W2N_SDK_${prefix}_STATIC_LIBRARY_PREFIX "")
    set(W2N_SDK_${prefix}_STATIC_LIBRARY_SUFFIX ".a")
    set(W2N_SDK_${prefix}_IMPORT_LIBRARY_PREFIX "")
    set(W2N_SDK_${prefix}_IMPORT_LIBRARY_SUFFIX "")
  else()
    set(W2N_SDK_${prefix}_OBJECT_FORMAT "ELF")
    set(W2N_SDK_${prefix}_SHARED_LIBRARY_PREFIX "lib")
    set(W2N_SDK_${prefix}_SHARED_LIBRARY_SUFFIX ".so")
    set(W2N_SDK_${prefix}_STATIC_LIBRARY_PREFIX "lib")
    set(W2N_SDK_${prefix}_STATIC_LIBRARY_SUFFIX ".a")
    set(W2N_SDK_${prefix}_IMPORT_LIBRARY_PREFIX "")
    set(W2N_SDK_${prefix}_IMPORT_LIBRARY_SUFFIX "")
  endif()

  set(W2N_SDK_${prefix}_USE_ISYSROOT FALSE)

  # GCC on Linux is usually located under `/usr`.
  # However, Ubuntu 20.04 ships with another GCC installation under `/`, which
  # does not include libstdc++. Swift build scripts pass `--sysroot=/` to
  # Clang. By default, Clang tries to find GCC installation under sysroot, and
  # if it doesn't exist, under `{sysroot}/usr`. On Ubuntu 20.04 and newer, it
  # attempts to use the GCC without the C++ stdlib, which causes a build
  # failure. To fix that, we tell Clang explicitly to use GCC from `/usr`.
  # FIXME: This is a compromise. The value might depend on the architecture
  # but add_swift_target_library does not allow passing different values
  # depending on the architecture, so having a single value is the only
  # possibility right now.
  set(W2N_SDK_${prefix}_CXX_OVERLAY_W2N_COMPILE_FLAGS
    -Xcc --gcc-toolchain=/usr
    CACHE STRING "Extra flags for compiling the C++ overlay")

  set(_default_threading_package "pthreads")

  if("${prefix}" STREQUAL "LINUX")
    set(_default_threading_package "linux")
  elseif("${prefix}" STREQUAL "WASI")
    set(_default_threading_package "none")
  endif()

  get_threading_package(${prefix} ${_default_threading_package}
    W2N_SDK_${prefix}_THREADING_PACKAGE)

  foreach(arch ${architectures})
    if("${prefix}" STREQUAL "ANDROID")
      swift_android_sysroot(android_sysroot)
      set(W2N_SDK_ANDROID_ARCH_${arch}_PATH "${android_sysroot}")

      if("${arch}" STREQUAL "armv7")
        set(W2N_SDK_ANDROID_ARCH_${arch}_NDK_TRIPLE "arm-linux-androideabi")
        set(W2N_SDK_ANDROID_ARCH_${arch}_ALT_SPELLING "arm")
        set(W2N_SDK_ANDROID_ARCH_${arch}_TRIPLE "armv7-unknown-linux-androideabi")

        # The Android ABI isn't part of the module triple.
        set(W2N_SDK_ANDROID_ARCH_${arch}_MODULE "armv7-unknown-linux-android")
        set(W2N_SDK_ANDROID_ARCH_${arch}_ABI "armeabi-v7a")
      elseif("${arch}" STREQUAL "aarch64")
        set(W2N_SDK_ANDROID_ARCH_${arch}_NDK_TRIPLE "aarch64-linux-android")
        set(W2N_SDK_ANDROID_ARCH_${arch}_ALT_SPELLING "aarch64")
        set(W2N_SDK_ANDROID_ARCH_${arch}_TRIPLE "aarch64-unknown-linux-android")
        set(W2N_SDK_ANDROID_ARCH_${arch}_ABI "arm64-v8a")
      elseif("${arch}" STREQUAL "i686")
        set(W2N_SDK_ANDROID_ARCH_${arch}_NDK_TRIPLE "i686-linux-android")
        set(W2N_SDK_ANDROID_ARCH_${arch}_ALT_SPELLING "i686")
        set(W2N_SDK_ANDROID_ARCH_${arch}_TRIPLE "i686-unknown-linux-android")
        set(W2N_SDK_ANDROID_ARCH_${arch}_ABI "x86")
      elseif("${arch}" STREQUAL "x86_64")
        set(W2N_SDK_ANDROID_ARCH_${arch}_NDK_TRIPLE "x86_64-linux-android")
        set(W2N_SDK_ANDROID_ARCH_${arch}_ALT_SPELLING "x86_64")
        set(W2N_SDK_ANDROID_ARCH_${arch}_TRIPLE "x86_64-unknown-linux-android")
        set(W2N_SDK_ANDROID_ARCH_${arch}_ABI "x86_64")
      else()
        message(FATAL_ERROR "unknown arch for android SDK: ${arch}")
      endif()
    else()
      set(W2N_SDK_${prefix}_ARCH_${arch}_PATH "/" CACHE STRING "CMAKE_SYSROOT for ${prefix} ${arch}")

      if("${prefix}" STREQUAL "LINUX")
        if(arch MATCHES "(armv5)")
          set(W2N_SDK_LINUX_ARCH_${arch}_TRIPLE "${arch}-unknown-linux-gnueabi")
        elseif(arch MATCHES "(armv6|armv7)")
          set(W2N_SDK_LINUX_ARCH_${arch}_TRIPLE "${arch}-unknown-linux-gnueabihf")
        elseif(arch MATCHES "(aarch64|i686|powerpc|powerpc64|powerpc64le|s390x|x86_64|riscv64)")
          set(W2N_SDK_LINUX_ARCH_${arch}_TRIPLE "${arch}-unknown-linux-gnu")
        else()
          message(FATAL_ERROR "unknown arch for ${prefix}: ${arch}")
        endif()
      elseif("${prefix}" STREQUAL "FREEBSD")
        if(NOT arch STREQUAL x86_64)
          message(FATAL_ERROR "unsupported arch for FreeBSD: ${arch}")
        endif()

        if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL FreeBSD)
          message(WARNING "CMAKE_SYSTEM_VERSION will not match target")
        endif()

        string(REGEX REPLACE "[-].*" "" freebsd_system_version ${CMAKE_SYSTEM_VERSION})
        message(STATUS "FreeBSD Version: ${freebsd_system_version}")

        set(W2N_SDK_FREEBSD_ARCH_x86_64_TRIPLE "x86_64-unknown-freebsd${freebsd_system_version}")
      elseif("${prefix}" STREQUAL "OPENBSD")
        if(NOT arch STREQUAL amd64)
          message(FATAL_ERROR "unsupported arch for OpenBSD: ${arch}")
        endif()

        set(openbsd_system_version ${CMAKE_SYSTEM_VERSION})
        message(STATUS "OpenBSD Version: ${openbsd_system_version}")

        set(W2N_SDK_OPENBSD_ARCH_amd64_TRIPLE "amd64-unknown-openbsd${openbsd_system_version}")

        if(CMAKE_SYSROOT)
          set(W2N_SDK_OPENBSD_ARCH_${arch}_PATH "${CMAKE_SYSROOT}${W2N_SDK_OPENBSD_ARCH_${arch}_PATH}" CACHE INTERNAL "sysroot path" FORCE)
        endif()
      elseif("${prefix}" STREQUAL "CYGWIN")
        if(NOT arch STREQUAL x86_64)
          message(FATAL_ERROR "unsupported arch for cygwin: ${arch}")
        endif()

        set(W2N_SDK_CYGWIN_ARCH_x86_64_TRIPLE "x86_64-unknown-windows-cygnus")
      elseif("${prefix}" STREQUAL "HAIKU")
        if(NOT arch STREQUAL x86_64)
          message(FATAL_ERROR "unsupported arch for Haiku: ${arch}")
        endif()

        set(W2N_SDK_HAIKU_ARCH_x86_64_TRIPLE "x86_64-unknown-haiku")
      elseif("${prefix}" STREQUAL "WASI")
        if(NOT arch STREQUAL wasm32)
          message(FATAL_ERROR "unsupported arch for WebAssembly: ${arch}")
        endif()

        set(W2N_SDK_WASI_ARCH_wasm32_PATH "${W2N_WASI_SYSROOT_PATH}")
        set(W2N_SDK_WASI_ARCH_wasm32_TRIPLE "wasm32-unknown-wasi")
      else()
        message(FATAL_ERROR "unknown Unix OS: ${prefix}")
      endif()
    endif()

    # If the module triple wasn't set explicitly, it's the same as the triple.
    if(NOT W2N_SDK_${prefix}_ARCH_${arch}_MODULE)
      set(W2N_SDK_${prefix}_ARCH_${arch}_MODULE "${W2N_SDK_${prefix}_ARCH_${arch}_TRIPLE}")
    endif()
  endforeach()

  # Add this to the list of known SDKs.
  list(APPEND W2N_CONFIGURED_SDKS "${prefix}")

  _report_sdk("${prefix}")
endmacro()

macro(configure_sdk_windows name environment architectures)
  # Note: this has to be implemented as a macro because it sets global
  # variables.
  swift_windows_cache_VCVARS()

  string(TOUPPER ${name} prefix)
  string(TOLOWER ${name} platform)

  set(W2N_SDK_${prefix}_NAME "${name}")
  set(W2N_SDK_${prefix}_LIB_SUBDIR "windows")
  set(W2N_SDK_${prefix}_ARCHITECTURES "${architectures}")
  set(W2N_SDK_${prefix}_OBJECT_FORMAT "COFF")
  set(W2N_SDK_${prefix}_USE_ISYSROOT FALSE)
  set(W2N_SDK_${prefix}_SHARED_LIBRARY_PREFIX "")
  set(W2N_SDK_${prefix}_SHARED_LIBRARY_SUFFIX ".dll")
  set(W2N_SDK_${prefix}_STATIC_LIBRARY_PREFIX "")
  set(W2N_SDK_${prefix}_STATIC_LIBRARY_SUFFIX ".lib")
  set(W2N_SDK_${prefix}_IMPORT_LIBRARY_PREFIX "")
  set(W2N_SDK_${prefix}_IMPORT_LIBRARY_SUFFIX ".lib")
  get_threading_package(${prefix} "win32" W2N_SDK_${prefix}_THREADING_PACKAGE)

  foreach(arch ${architectures})
    if(arch STREQUAL armv7)
      set(W2N_SDK_${prefix}_ARCH_${arch}_TRIPLE
        "thumbv7-unknown-windows-${environment}")
    else()
      set(W2N_SDK_${prefix}_ARCH_${arch}_TRIPLE
        "${arch}-unknown-windows-${environment}")
    endif()

    set(W2N_SDK_${prefix}_ARCH_${arch}_MODULE "${W2N_SDK_${prefix}_ARCH_${arch}_TRIPLE}")

    # NOTE: set the path to / to avoid a spurious `--sysroot` from being passed
    # to the driver -- rely on the `INCLUDE` AND `LIB` environment variables
    # instead.
    set(W2N_SDK_${prefix}_ARCH_${arch}_PATH "/")

    # NOTE(compnerd) workaround incorrectly extensioned import libraries from
    # the Windows SDK on case sensitive file systems.
    swift_windows_arch_spelling(${arch} WinSDKArchitecture)
    set(WinSDK${arch}UMDir "${UniversalCRTSdkDir}/Lib/${UCRTVersion}/um/${WinSDKArchitecture}")
    set(OverlayDirectory "${CMAKE_BINARY_DIR}/winsdk_lib_${arch}_symlinks")

    if(NOT EXISTS "${UniversalCRTSdkDir}/Include/${UCRTVersion}/um/WINDOWS.H")
      file(MAKE_DIRECTORY ${OverlayDirectory})

      file(GLOB libraries RELATIVE "${WinSDK${arch}UMDir}" "${WinSDK${arch}UMDir}/*")

      foreach(library ${libraries})
        get_filename_component(name_we "${library}" NAME_WE)
        get_filename_component(ext "${library}" EXT)
        string(TOLOWER "${ext}" lowercase_ext)
        set(lowercase_ext_symlink_name "${name_we}${lowercase_ext}")

        if(NOT library STREQUAL lowercase_ext_symlink_name)
          execute_process(COMMAND
            "${CMAKE_COMMAND}" -E create_symlink "${WinSDK${arch}UMDir}/${library}" "${OverlayDirectory}/${lowercase_ext_symlink_name}")
        endif()
      endforeach()
    endif()
  endforeach()

  # Add this to the list of known SDKs.
  list(APPEND W2N_CONFIGURED_SDKS "${prefix}")

  _report_sdk("${prefix}")
endmacro()

# Configure a variant of a certain SDK
#
# In addition to the SDK and architecture, a variant determines build settings.
#
# FIXME: this is not wired up with anything yet.
function(configure_target_variant prefix name sdk build_config lib_subdir)
  set(W2N_VARIANT_${prefix}_NAME ${name})
  set(W2N_VARIANT_${prefix}_SDK_PATH ${W2N_SDK_${sdk}_PATH})
  set(W2N_VARIANT_${prefix}_VERSION ${W2N_SDK_${sdk}_VERSION})
  set(W2N_VARIANT_${prefix}_BUILD_NUMBER ${W2N_SDK_${sdk}_BUILD_NUMBER})
  set(W2N_VARIANT_${prefix}_DEPLOYMENT_VERSION ${W2N_SDK_${sdk}_DEPLOYMENT_VERSION})
  set(W2N_VARIANT_${prefix}_LIB_SUBDIR "${lib_subdir}/${W2N_SDK_${sdk}_LIB_SUBDIR}")
  set(W2N_VARIANT_${prefix}_TRIPLE_NAME ${W2N_SDK_${sdk}_TRIPLE_NAME})
  set(W2N_VARIANT_${prefix}_ARCHITECTURES ${W2N_SDK_${sdk}_ARCHITECTURES})
  set(W2N_VARIANT_${prefix}_SHARED_LIBRARY_PREFIX ${W2N_SDK_${sdk}_SHARED_LIBRARY_PREFIX})
  set(W2N_VARIANT_${prefix}_SHARED_LIBRARY_SUFFIX ${W2N_SDK_${sdk}_SHARED_LIBRARY_SUFFIX})
  set(W2N_VARIANT_${prefix}_STATIC_LIBRARY_PREFIX ${W2N_SDK_${sdk}_STATIC_LIBRARY_PREFIX})
  set(W2N_VARIANT_${prefix}_STATIC_LIBRARY_SUFFIX ${W2N_SDK_${sdk}_STATIC_LIBRARY_SUFFIX})
  set(W2N_VARIANT_${prefix}_IMPORT_LIBRARY_PREFIX ${W2N_SDK_${sdk}_IMPORT_LIBRARY_PREFIX})
  set(W2N_VARIANT_${prefix}_IMPORT_LIBRARY_SUFFIX ${W2N_SDK_${sdk}_IMPORT_LIBRARY_SUFFIX})
  get_threading_package(${prefix} ${W2N_SDK_${sdk}_THREADING_PACKAGE} W2N_VARIANT_${prefix}_THREADING_PACKAGE)
endfunction()