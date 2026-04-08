# Changelog

## 2026.4.8 changes

### bug fixed

- `merak/flatten/pb_to_flat.cc`: fix repeated string flatten bug in repeated non-bytes branch (avoid overwriting indexed key like `names.[0]`).
- `merak/flatten/json_to_flat.cc`: fix `json_to_flat(const merak::json::Value&, ...)` error path to return `cc.error()` instead of empty message.
- `merak/flatten/json_to_flat.cc`: adjust array handling for flat constraints (reject anonymous arrays, allow named array flattening).
- `merak/flatten/pb_to_flat.cc`: remove `std::cout` debug prints in `Any` branch to avoid noisy output.

### promote 

- `merak/flatten/pb_to_flat.h`: change `proto_message_to_flat_json(...)` and all `proto_message_to_flat(...)` overloads from `void` to `turbo::Status`.
- `merak/flatten/pb_to_flat.cc`: propagate conversion failures with `turbo::invalid_argument_error(converter.ErrorText())` instead of ignoring return values.
- `tests/mock_test.cc`: add explicit `.ok()` assertions for `proto_message_to_flat_json(...)` and `proto_message_to_flat(...)`.

### functional add

- add `tests/proto/flat2_test.proto` and `tests/proto/flat3_test.proto` based flat conversion test coverage.
- add `tests/flat_test.cc` with proto2/proto3 `pb -> flat` and `json -> flat` cases.
- add `flat_map_to_string(...)` helper in `tests/flat_test.cc` for assertion-time full map debug output.
- register `flat_test` in `tests/CMakeLists.txt`.
