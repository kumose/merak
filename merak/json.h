// Copyright (C) 2026 Kumo inc. and its affiliates. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#pragma once

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)

#pragma GCC diagnostic push

#pragma GCC diagnostic ignored "-Wunused-local-typedefs"

#endif

#include <merak/json/allocators.h>
#include <merak/json/document.h>
#include <merak/json/encodedstream.h>
#include <merak/json/encodings.h>
#include <merak/json/filereadstream.h>
#include <merak/json/filewritestream.h>
#include <merak/json/filewritestream.h>
#include <merak/json/prettywriter.h>
#include <merak/json/json_internal.h>
#include <merak/json/reader.h>
#include <merak/json/stringbuffer.h>
#include <merak/json/writer.h>
#include <merak/json/error/en.h>  // GetErrorCode_En
#include <merak/json/error/error.h>
#include <merak/json/schema.h>
#include <merak/json/std_output_stream.h>

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)
#pragma GCC diagnostic pop
#endif

