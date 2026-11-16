# Contributing

Help make the first reading easy to send and easy to understand. Changes should preserve the electrical and protocol requirements that make the result trustworthy.

**Start here:** reproduce the behavior with [the sample](Telemetry_Station_SDK/docs/GETTING_STARTED.md), then make one focused change and run the checks below.

Thanks for helping. This project aims to stay small and readable.

- Keep the driver in portable C99 with no dynamic memory, no stdio in `src/` (apart from `ts_activation.c`), and no hardware access outside the platform interface. A test enforces the first two.
- Use fixed-width integer types in public interfaces, named constants instead of magic numbers, and bound protocol waits and service work; retain the documented nonblocking fault hold.
- Add a regression test at the relevant layer for behavior changes; documentation-only edits need link and example checks. From `Telemetry_Station_SDK/`, run `cmake -S . -B build -DTS_WERROR=ON -DTS_SANITIZE=ON && cmake --build build && ctest --test-dir build --output-on-failure`, and again with `-DTS_ENABLE_MT=OFF`. Builds must be free of warnings. Failure paths go in `tests/test_faults.c`, and `python3 tests/coverage.py` must pass (100% required line and branch coverage across the core, checkout, F169 port and firmware; unreachable branches need a documented reason).
- The two `ts_activation` files in the SDK and the Arduino kit must stay identical. CI checks it.
- Read the [design notes](Telemetry_Station_SDK/docs/DESIGN.md) before changing the driver's state machine.
- Add a line to [CHANGELOG.md](CHANGELOG.md) for any change a user would notice.
- Say in your pull request what you tested and on which hardware. Hardware validation on the BEAR Module 9704 is tracked in the [roadmap](BEAR_Module/ROADMAP.md).
- Use the name "BEAR Module 9704" (family: "BEAR Module").

## Public repository boundary

Everything committed here, including tests, fixtures, documentation and commit messages, is public. Include only material authorized for public distribution. Public BEAR interface specifications and approved component summaries belong here; they must be usable without access to additional design files.

Before committing, review `git diff --cached` and the staged filenames for credentials, real customer/device identifiers, local machine paths and nonpublic links. Use synthetic test data and repository-relative paths. Hardware file ignore rules are an additional safeguard, not a substitute for review; never force-add confidential files. Sanitized configuration examples must contain placeholders only. Any approved hardware deliverable needs explicit publication review and applicable licensing/attribution before inclusion.

## Documentation acceptance

Every page should answer: what is this, what does it mean for me, what can I do next, and how will I know it worked? Put the reader’s outcome before implementation details. Give a primary next action with a useful link, prerequisites before instructions, and expected results beside commands.

Keep a single first-use path: **try the sample → check support → connect → activate → send once → decode → add the sensor**. Keep engineering reference tables accurate and explain unfamiliar terms. Do not replace measured limits, fault rules or pending validation with reassuring marketing copy. Avoid duplicate setup procedures that drift apart.

Run every changed command you can, check relative links and heading anchors, and identify physical or external steps that were not tested. Update controlled-document revisions for changed requirements. Keep release claims separate from host tests and simulated output.

**Next:** open a focused pull request describing the user-visible result, validation performed and any remaining hardware limitations.
