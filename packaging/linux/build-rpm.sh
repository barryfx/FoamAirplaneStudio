#!/bin/sh
set -eu

project_root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
version=$(sed -n 's/^project(FoamAirplaneStudio VERSION \([^ ]*\).*/\1/p' \
  "$project_root/CMakeLists.txt")
test -n "$version"
case "$version" in *.*.*) ;; *) version="$version.0" ;; esac
build_dir="$project_root/build/fedora-release"
package_stage=$(mktemp -d)
trap 'rm -rf -- "$package_stage"' EXIT HUP INT TERM

cmake -S "$project_root" -B "$build_dir" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DDESIGNRC_BUILD_TESTS=OFF \
  -DFOAM_LINUX_PACKAGE_GENERATOR=RPM \
  -DOpenCASCADE_DIR="${OpenCASCADE_DIR:-$project_root/../third_party/occt/install-linux-release/cmake}"
cmake --build "$build_dir" --parallel "${CMAKE_BUILD_PARALLEL_LEVEL:-8}"
cpack --config "$build_dir/CPackConfig.cmake" \
  -G RPM -B "$package_stage"

# Export source without build output or ignored user artwork. Also support builds
# from an extracted source archive, where no Git metadata is present.
if test -d "$project_root/.git" || test -f "$project_root/.git"; then
  git -C "$project_root" ls-files --cached --others --exclude-standard -z > "$package_stage/sources.list"
else
  (cd "$project_root" && find . -type d \( -name build -o -name dist -o -name .git \) -prune -o -type f -print0) > "$package_stage/sources.list"
fi
(cd "$project_root" && tar --null -T "$package_stage/sources.list" \
  -czf "$package_stage/FoamAirplaneStudio-$version-Linux-RPM-source.tar.gz")
(cd "$package_stage" && sha256sum *.rpm "FoamAirplaneStudio-$version-Linux-RPM-source.tar.gz" \
  > "FoamAirplaneStudio-$version-Linux-RPM-x64.sha256")

mkdir -p "$project_root/dist"
cp "$package_stage"/*.rpm "$package_stage"/*.tar.gz \
  "$package_stage"/*.sha256 "$project_root/dist/"
