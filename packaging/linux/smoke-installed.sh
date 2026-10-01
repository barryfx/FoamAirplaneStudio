#!/bin/sh
# Run as a normal user with DISPLAY set, after installing the native package.
set -eu
app_root=/usr/lib/foamairplanestudio
# Match the installed launcher when checking stand-alone plugin dependencies.
export LD_LIBRARY_PATH="$app_root${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
test -x "$app_root/foamairplanestudio"
test -x /usr/bin/foamairplanestudio
test -s /usr/share/foamairplanestudio/Example/BabyBuzzard36.foam
test -s /usr/share/foamairplanestudio/Example/Baby_Buzzard_Plan_559_New.pdf
test -f /usr/share/applications/foamairplanestudio.desktop
test -f "$app_root/licenses/GPL-3.0.txt"
test -f /usr/share/foamairplanestudio/occt/resources/Shaders/Declarations.glsl
ldd "$app_root/foamairplanestudio" > /tmp/foam-installed-ldd.txt
ldd "$app_root/qt6/plugins/platforms/libqxcb.so" >> /tmp/foam-installed-ldd.txt
ldd "$app_root/qt6/plugins/imageformats/libqjpeg.so" >> /tmp/foam-installed-ldd.txt
if grep 'not found' /tmp/foam-installed-ldd.txt; then exit 1; fi
foamairplanestudio > /tmp/foam-installed-launch.log 2>&1 &
app_pid=$!
trap 'kill "$app_pid" 2>/dev/null || true' EXIT HUP INT TERM
sleep 8
kill -0 "$app_pid"
xwininfo -root -tree > /tmp/foam-installed-windows.txt
grep 'FoamAirplaneStudio' /tmp/foam-installed-windows.txt
# Record actual loaded private runtimes, not just package metadata.
grep /usr/lib/foamairplanestudio /proc/"$app_pid"/maps > /tmp/foam-installed-maps.txt
for library in libQt6Core libQt6Pdf libTKernel; do
  grep -q "/usr/lib/foamairplanestudio/$library.so" /tmp/foam-installed-maps.txt
done
cat /tmp/foam-installed-launch.log
printf '%s\n' 'Installed application launch passed.'
