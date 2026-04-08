# Changelog

## 2026.4.9 changes

### functional add

- `benchmark/pb_json_bench.cc`: Google Benchmark comparing protobuf native JSON (`google::protobuf::util::MessageToJsonString` / `JsonStringToMessage`) versus merak (`merak::proto_message_to_json` / `merak::json_to_proto_message`) on the same `addressbook::AddressBook` payload as `tests/mock_test.cc`.
- `benchmark/proto/addressbook.proto`: bench-local copy of the addressbook schema; C++ output generated under `benchmark/proto/` via `kmcmake_cc_proto` / `merak::bench_proto_static`.
- `benchmark/CMakeLists.txt`: register `bench_proto`, link `merak_pb_json_bench` (`kmcmake_cc_bm`) against Google Benchmark and `merak::bench_proto_static`; run `configure_file` for `config.h` at the top of the file.

### convention

- `google.protobuf.Any` (merak **JSON ↔ protobuf** and the matching **flat** representation): treat the payload as **opaque**—no unpacking the inner type into structured JSON inside `value`. Encoding uses `type_url` and `value` (or JSON key `@type` when `Pb2JsonOptions::using_a_type_url` / `Pb2FlatOptions::using_a_type_url` is set). `value` is always a **JSON string** of **base64** over the inner message’s **binary** serialized bytes (not a JSON object). Decoding requires that string form; a JSON object for `value` is **rejected**. Option notes live in `merak/options.h` (`Pb2JsonOptions`, `Json2PbOptions`, `Pb2FlatOptions`).

## 2026.4.8 changes

### bug fixed

- `merak/flatten/pb_to_flat.cc`: fix repeated string flatten bug in repeated non-bytes branch (avoid overwriting indexed key like `names.[0]`).
- `merak/flatten/json_to_flat.cc`: fix `json_to_flat(const merak::json::Value&, ...)` error path to return `cc.error()` instead of empty message.
- `merak/flatten/json_to_flat.cc`: adjust array handling for flat constraints (reject anonymous arrays, allow named array flattening).
- `merak/flatten/pb_to_flat.cc`: remove `std::cout` debug prints in `Any` branch to avoid noisy output.
- `merak/container_traits.h`: relax `HasPushBackType` to accept `push_back(...) -> void`, enabling standard sequence containers (e.g. `std::vector`) in flat container traits checks.

### promote 

- `merak/flatten/pb_to_flat.h`: change `proto_message_to_flat_json(...)` and all `proto_message_to_flat(...)` overloads from `void` to `turbo::Status`.
- `merak/flatten/pb_to_flat.cc`: propagate conversion failures with `turbo::invalid_argument_error(converter.ErrorText())` instead of ignoring return values.
- `tests/mock_test.cc`: add explicit `.ok()` assertions for `proto_message_to_flat_json(...)` and `proto_message_to_flat(...)`.

### functional add

- add `tests/proto/flat2_test.proto` and `tests/proto/flat3_test.proto` based flat conversion test coverage.
- add `tests/flat_test.cc` with proto2/proto3 `pb -> flat` and `json -> flat` cases.
- add `flat_map_to_string(...)` helper in `tests/flat_test.cc` for assertion-time full map debug output.
- register `flat_test` in `tests/CMakeLists.txt`.
