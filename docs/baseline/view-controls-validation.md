# 3D View controls validation

Date: 2026-09-21. Windows Debug.

Built the Debug application and `view_controls_tests`. Ran
`ctest --preset windows-debug -R '^view_controls_tests$' --timeout 120`:
1/1 passed in 2.93 seconds (3.02 seconds total).

The test opens the real MainWindow and OCCT viewport without generating model
components. It verifies all eight buttons share the corresponding View menu
actions, clicks each button and compares its camera result with the menu command,
checks action disabling, processing lock/unlock, tab visibility, and bottom-center
placement at three window sizes. Navigation does not mark the project modified
or start generation jobs.

Inspected the native-window screenshot `build/debug/view-controls.png`. Fit View,
Reset, Top, Bottom, Front, Back, Left and Right are visible as one centered row
above the bottom edge. The toolbar stays above the OCCT rendering surface after
camera changes and resizing. The image and build/test logs are local artifacts.

No component-generation or geometry regression tests were run. Linux/macOS and
alternate display scaling were not exercised.
