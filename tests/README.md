# Regression tests

Use `TEST_CHECK` from `TestCheck.h` for new test checks. It evaluates its expression
once in every configuration and reports the expression, file and line on failure.
It exits with a failure status, including when called from a Qt event callback.
Do not use standard `assert` for regression checks or setup: NDEBUG removes both
in Release builds. Existing local CHECK macros that always evaluate remain valid.

`test_check_tests` and `test_check_failure_tests` verify success, expression
side effects and intentional failure with NDEBUG explicitly defined in every
configuration. The latter is expected to exit nonzero and uses CTest WILL_FAIL.

The Inspect fixture supplies root outline/stations before injecting cached wing
geometry, matching the root-center rotation pivot requirement. The fuselage
orientation test checks both sides of the guide's centered width, consistent
with ADR-0045, while retaining its narrow-shoulder and profile-orientation checks.

The Release preset disables tests by default. To build and run the full Release
suite when model-generation testing has been authorized:

```powershell
cmake --preset windows-release -DDESIGNRC_BUILD_TESTS=ON
cmake --build --preset windows-release --parallel 8
ctest --test-dir build/release -C Release --output-on-failure
```

Follow AGENTS.md for generation-test authorization. Run GUI tests sequentially
unless they have been explicitly isolated for concurrent execution.

`fiberglass_tests` uses analytical OCCT primitives, editor inputs and project
snapshots; it does not invoke Wing/Fuselage generation. `weight_balance_tests`
also covers covering mass/CG, the Material Densities dialog and area-cache reuse
using injected Assembly fixtures. Their screenshots go to the Debug build folder.
