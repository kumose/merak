#
# Copyright 2025 The titan-search Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
include(CheckCXXCompilerFlag)

set(JSON_UNIT_TEST_SOURCES
        allocators_test
        biginteger_test
        clzll_test
        cursorstreamwrapper_test
        document_test
        dtoa_test
        encodedstream_test
        encodings_test
        fwd_test
        filestream_test
        itoa_test
        jsonchecker_test
        pointer_test
        platform_test
        prettywriter_test
        std_input_stream_test
        std_output_stream_test
        reader_test
        regex_test
        schema_test
        simd_test
        strfunc_test
        stringbuffer_test
        strtod_test
        uri_test
        value_test
        writer_test)
foreach (unittest IN LISTS JSON_UNIT_TEST_SOURCES)
    kmcmake_cc_test(
            NAME ${unittest}
            MODULE json
            SOURCES ${unittest}.cc unit_test.cc
            CXXOPTS
            ${KMCMAKE_CXX_OPTIONS}
            LINKS
            GTest::gtest
            GTest::gtest_main
            merak::merak_static
            ${KMCMAKE_DEPS_LINK}
    )
endforeach ()

