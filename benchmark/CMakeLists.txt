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

configure_file(${PROJECT_SOURCE_DIR}/benchmark/config.h.in ${PROJECT_SOURCE_DIR}/benchmark/config.h @ONLY)

kmcmake_cc_proto(
        NAME bench_proto
        PROTOS
        proto/addressbook.proto
        OUTDIR
        ${PROJECT_SOURCE_DIR}
)

kmcmake_cc_library(
        NAMESPACE merak
        NAME bench_proto
        SOURCES
        ${bench_proto_SRCS}
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
)

find_package(benchmark REQUIRED CONFIG)

kmcmake_cc_bm(
        NAME pb_json_bench
        MODULE merak
        SOURCES pb_json_bench.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        LINKS
        benchmark::benchmark
        benchmark::benchmark_main
        merak::merak_static
        merak::bench_proto_static
        ${KMCMAKE_DEPS_LINK}
)
