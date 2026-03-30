# Copyright (C) Kumo inc. and its affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
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
kmcmake_cc_test(
        NAME mock_test
        MODULE base
        SOURCES mock_test.cc
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