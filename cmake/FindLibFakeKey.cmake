#
# - Try to find the fakekey library
# Once done this will define
#
#  LibFakeKey_FOUND - system has LibFakeKey
#  LibFakeKey_INCLUDE_DIRS - the LibFakeKey include directory
#  LibFakeKey_LIBRARIES - The libraries needed to use LibFakeKey
#  LibFakeKey_VERSION - The LibFakeKey version

# Copyright (c) 2014 Christophe Giboudeaux <cgiboudeaux@gmx.com>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. The name of the author may not be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


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
