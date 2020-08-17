#
# - Try to find the fakekey library
# Once done this will define
#
#  LibFakeKey_FOUND - system has LibFakeKey
#  LibFakeKey_INCLUDE_DIRS - the LibFakeKey include directory
#  LibFakeKey_LIBRARIES - The libraries needed to use LibFakeKey
#  LibFakeKey_VERSION - The LibFakeKey version

# SPDX-FileCopyrightText: 2014 Christophe Giboudeaux <cgiboudeaux@gmx.com>
#
# SPDX-License-Identifier: BSD-3-Clause


find_package(PkgConfig QUIET REQUIRED)
pkg_check_modules(PC_LibFakeKey libfakekey)

find_path(LibFakeKey_INCLUDE_DIRS
  NAMES fakekey/fakekey.h
  HINTS ${PC_LibFakeKey_LIBRARY_DIRS}
)

find_library(LibFakeKey_LIBRARIES
  NAMES fakekey
  HINTS ${PC_LibFakeKey_LIBRARY_DIRS}
)

set(LibFakeKey_VERSION ${PC_LibFakeKey_VERSION})

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(LibFakeKey
    FOUND_VAR LibFakeKey_FOUND
    REQUIRED_VARS LibFakeKey_LIBRARIES LibFakeKey_INCLUDE_DIRS
    VERSION_VAR LibFakeKey_VERSION
)

mark_as_advanced(LibFakeKey_LIBRARIES LibFakeKey_INCLUDE_DIRS LibFakeKey_VERSION)
