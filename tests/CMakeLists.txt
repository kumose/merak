# Copyright (C) Kumo inc. and its affiliates.
# Author: Jeff.li lijippy@163.com
# All rights reserved.
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#


configure_file(${PROJECT_SOURCE_DIR}/tests/config.h.in ${PROJECT_SOURCE_DIR}/tests/config.h @ONLY)

configure_file(data/paths.h.in ${PROJECT_SOURCE_DIR}/tests/data/paths.h @ONLY)



set(TEST_PROTO_FILES
        proto/addressbook1.proto
        proto/addressbook_encode_decode.proto
        proto/addressbook_map.proto
        proto/addressbook.proto
        proto/message.proto
        proto/repeated.proto
)

kmcmake_cc_proto(
        NAME test_proto
        PROTOS
        ${TEST_PROTO_FILES}
        OUTDIR
        ${PROJECT_SOURCE_DIR}
)
kmcmake_cc_library(
        NAMESPACE merak
        NAME test_proto
        SOURCES
        ${test_proto_SRCS}
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
)



find_library(LIBPROFILER NAMES libprofiler.a)
if (NOT LIBPROFILER)
    message(FATAL_ERROR "not found gperftools")
endif ()
find_package(PkgConfig)
pkg_check_modules(libunwind REQUIRED IMPORTED_TARGET GLOBAL libunwind)
find_package(GTest REQUIRED)
list(APPEND MERAK_UNITTESTS
        repeated_field_test
        protobuf_json_test
        )
foreach(unittest IN LISTS MERAK_UNITTESTS)
kmcmake_cc_test(
        NAME ${unittest}
        MODULE base
        SOURCES ${unittest}.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        LINKS
        GTest::gtest
        GTest::gtest_main
        merak::merak_static
        merak::test_proto_static
        ${LIBPROFILER}
        PkgConfig::libunwind
        ${KMCMAKE_DEPS_LINK}
)
endforeach()
add_subdirectory(json)
add_subdirectory(pertf)
