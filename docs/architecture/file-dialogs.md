# File and directory dialog history

All file/directory choosers use FileSelectionDialog rather than QFileDialog's
static helpers. Reference Image, Airfoil DAT and project Open/Save As use it.
New Open, Save, import, export and directory choosers must use this shared class
with a stable purpose key and the appropriate file/accept modes.

Each purpose remembers its last accepted absolute file or directory path in
QSettings under the FoamAirplaneStudio application/organization identity. Opening
it again selects that file in its parent folder, or opens the selected directory.
History survives New and application restarts. Cancel and mere navigation do not
change history. A file selection is remembered even if subsequent loading fails.

A chooser without its own history starts in the most recently accepted folder
from another chooser, then Documents or Home if no history exists. Missing paths
fall back to the nearest existing parent. Save dialogs also retain the filename
of a not-yet-created output. Native platform dialogs remain the default.

QSettings keys are fileDialogs/<purpose>/selection and fileDialogs/lastDirectory.
This is per-user UI preference storage, separate from project data; New does not
clear it. Project Open/Save As share the projectFile purpose key. Project content
is stored separately in `.foam` files (projects.md).

Tests use temporary INI settings so validation never overwrites user preferences.
They exercise actual dialog acceptance for file, folder and save modes,
recreation, cancellation, per-purpose history, shared fallback and removed paths.

Windows Debug validation (2026-09-13): full Debug build passed; file_dialog_tests
and reference_tests passed (2/2, 1.19 seconds). The rebuilt normal Debug app was
launched. Automated chooser tests use non-native Qt dialogs; native OS dialog
interaction and Linux/macOS were not tested.
