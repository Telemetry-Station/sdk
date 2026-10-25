# Contributing

Thanks for helping. This project aims to stay small and readable.

- Keep the driver in portable C99 with no dynamic memory, no stdio in `src/` (apart from `ts_activation.c`), and no hardware access outside the platform interface. A test enforces the first two.
- Use fixed-width integer types in public interfaces, named constants instead of magic numbers, and give every wait a timeout.
- Add a test in `Telemetry_Station_SDK/tests/test_sdk.c` for any behavior you change. From `Telemetry_Station_SDK/`, run `cmake -S . -B build -DTS_WERROR=ON -DTS_SANITIZE=ON && cmake --build build && ctest --test-dir build --output-on-failure`, and again with `-DTS_ENABLE_MT=OFF`. Builds must be free of warnings. Failure paths go in `tests/test_faults.c`, and `sh tests/coverage.sh` must pass (100% line coverage of `src/`).
- The two `ts_activation` files in the SDK and the Arduino kit must stay identical. CI checks it.
- Read the [design notes](Telemetry_Station_SDK/docs/DESIGN.md) before changing the driver's state machine.
- Add a line to [CHANGELOG.md](CHANGELOG.md) for any change a user would notice.
- Say in your pull request what you tested and on which hardware. Hardware validation on the BEAR Module 9704 is tracked in the [roadmap](BEAR_Module/ROADMAP.md).
- Use the name "BEAR Module 9704" (family: "BEAR Module").
