// Copyright (C) 2024 Kumo inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
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

