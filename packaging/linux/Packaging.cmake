# Private runtime layout follows the sibling DesignRC Linux packages.
set(FOAM_LINUX_PACKAGE_GENERATOR "DEB" CACHE STRING "Linux package format")
set_property(CACHE FOAM_LINUX_PACKAGE_GENERATOR PROPERTY STRINGS DEB RPM)
if(UNIX AND NOT APPLE)
  if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64)$")
    message(FATAL_ERROR "Linux packages currently target x86-64 only")
  endif()
  set_target_properties(designrc PROPERTIES
    BUILD_RPATH "${OpenCASCADE_LIBRARY_DIR}" INSTALL_RPATH "$ORIGIN")
  # Inherit the private runtime path for transitive OCCT dependencies too.
  target_link_options(designrc PRIVATE "LINKER:--disable-new-dtags")
  if(OpenCASCADE_MAJOR_VERSION LESS 8)
    message(FATAL_ERROR "FoamAirplaneStudio Linux packages require OCCT 8 or newer")
  endif()

  install(TARGETS designrc RUNTIME DESTINATION lib/foamairplanestudio)
  install(PROGRAMS packaging/linux/foamairplanestudio DESTINATION bin)
  install(FILES packaging/linux/qt.conf DESTINATION lib/foamairplanestudio)
  install(FILES packaging/linux/foamairplanestudio.desktop
    DESTINATION share/applications)
  install(FILES
    resources/graphics/FoamAirplaneStudio.png
    DESTINATION share/foamairplanestudio/graphics)

  install(DIRECTORY resources/help DESTINATION lib/foamairplanestudio)
  install(DIRECTORY resources/Example DESTINATION share/foamairplanestudio)
  install(DIRECTORY licenses DESTINATION lib/foamairplanestudio)
  install(FILES LICENSE
    DESTINATION lib/foamairplanestudio/licenses
    RENAME FOAM-GPL-3.0.txt)
  install(FILES THIRD_PARTY_NOTICES.md
    DESTINATION lib/foamairplanestudio/licenses)
  install(FILES packaging/linux/copyright DESTINATION share/doc/foamairplanestudio)
  find_program(GZIP_EXECUTABLE gzip REQUIRED)
  execute_process(
    COMMAND "${GZIP_EXECUTABLE}" -9 -n -c
      "${CMAKE_SOURCE_DIR}/packaging/linux/changelog"
    OUTPUT_FILE "${CMAKE_BINARY_DIR}/changelog.gz"
    COMMAND_ERROR_IS_FATAL ANY
  )
  install(FILES "${CMAKE_BINARY_DIR}/changelog.gz"
    DESTINATION share/doc/foamairplanestudio)

  # Executable library permissions let RPM scan transitive ELF requirements.
  file(GLOB _foamairplanestudio_occt_libraries
    "${OpenCASCADE_LIBRARY_DIR}/libTK*.so.*")
  if(NOT _foamairplanestudio_occt_libraries)
    message(FATAL_ERROR "No Linux OCCT runtime libraries were found")
  endif()
  install(PROGRAMS ${_foamairplanestudio_occt_libraries} DESTINATION lib/foamairplanestudio)
  install(DIRECTORY "${OpenCASCADE_RESOURCE_DIR}/"
    DESTINATION share/foamairplanestudio/occt/resources)

  get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
  execute_process(
    COMMAND "${QT_QMAKE_EXECUTABLE}" -query QT_INSTALL_LIBS
    OUTPUT_VARIABLE QT_INSTALL_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  execute_process(
    COMMAND "${QT_QMAKE_EXECUTABLE}" -query QT_INSTALL_PLUGINS
    OUTPUT_VARIABLE QT_INSTALL_PLUGINS
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  set(_foamairplanestudio_qt_modules Core DBus Gui Widgets XcbQpa Pdf Network)
  set(_foamairplanestudio_qt_libraries)
  foreach(_module IN LISTS _foamairplanestudio_qt_modules)
    file(GLOB _module_libraries
      "${QT_INSTALL_LIBS}/libQt6${_module}.so.6*")
    list(APPEND _foamairplanestudio_qt_libraries ${_module_libraries})
  endforeach()
  if(NOT _foamairplanestudio_qt_libraries OR
      NOT EXISTS "${QT_INSTALL_PLUGINS}/platforms/libqxcb.so")
    message(FATAL_ERROR "The Linux Qt runtime or XCB platform plugin was not found")
  endif()
  install(PROGRAMS ${_foamairplanestudio_qt_libraries} DESTINATION lib/foamairplanestudio)
  set(_foamairplanestudio_qt_platform_plugins
    "${QT_INSTALL_PLUGINS}/platforms/libqxcb.so")
  if(EXISTS "${QT_INSTALL_PLUGINS}/platforms/libqoffscreen.so")
    list(APPEND _foamairplanestudio_qt_platform_plugins
      "${QT_INSTALL_PLUGINS}/platforms/libqoffscreen.so")
  endif()
  install(PROGRAMS ${_foamairplanestudio_qt_platform_plugins}
    DESTINATION lib/foamairplanestudio/qt6/plugins/platforms)

  # Reference artwork also supports JPEG; PNG support is built into QtGui.
  if(NOT EXISTS "${QT_INSTALL_PLUGINS}/imageformats/libqjpeg.so")
    message(FATAL_ERROR "Qt JPEG image plugin is required for reference images")
  endif()
  install(PROGRAMS "${QT_INSTALL_PLUGINS}/imageformats/libqjpeg.so"
    DESTINATION lib/foamairplanestudio/qt6/plugins/imageformats)

  # Install the ICU ABI used by the bundled Qt libraries. This keeps the
  # package portable across distributions whose repositories use another
  # ICU ABI package name.
  set(_foamairplanestudio_icu_libraries)
  foreach(_icu_component icudata icui18n icuuc)
    file(GLOB _foamairplanestudio_icu_component_libraries
      "${QT_INSTALL_LIBS}/lib${_icu_component}.so.*")
    if(NOT _foamairplanestudio_icu_component_libraries)
      message(FATAL_ERROR
        "The Linux ${_icu_component} runtime library was not found")
    endif()
    list(APPEND _foamairplanestudio_icu_libraries
      ${_foamairplanestudio_icu_component_libraries})
  endforeach()
  list(REMOVE_DUPLICATES _foamairplanestudio_icu_libraries)
  install(PROGRAMS ${_foamairplanestudio_icu_libraries} DESTINATION lib/foamairplanestudio)

  # Keep the distro's version-matched notices alongside the upstream collection.
  # The collection's Windows inventory is historical, not this package's manifest.
  if(EXISTS "/etc/debian_version")
    file(GLOB _foam_distro_notices
      "/usr/share/doc/libqt6core*/copyright"
      "/usr/share/doc/libqt6gui*/copyright"
      "/usr/share/doc/libqt6widgets*/copyright"
      "/usr/share/doc/libqt6network*/copyright"
      "/usr/share/doc/libqt6dbus*/copyright"
      "/usr/share/doc/libqt6pdf*/copyright"
      "/usr/share/doc/libicu*/copyright")
    foreach(_notice IN LISTS _foam_distro_notices)
      get_filename_component(_notice_dir "${_notice}" DIRECTORY)
      get_filename_component(_notice_package "${_notice_dir}" NAME)
      install(FILES "${_notice}" DESTINATION
        "lib/foamairplanestudio/licenses/linux/${_notice_package}")
    endforeach()
  else()
    file(GLOB _foam_distro_notices "/usr/share/licenses/qt6-qtbase*"
      "/usr/share/licenses/qt6-qtpdf*" "/usr/share/licenses/libicu*"
      "/usr/share/licenses/opencascade*")
    install(DIRECTORY ${_foam_distro_notices}
      DESTINATION lib/foamairplanestudio/licenses/linux)
  endif()
  if(NOT _foam_distro_notices)
    message(FATAL_ERROR "No distribution Qt/ICU license notices found")
  endif()
  file(READ "/etc/os-release" _foam_os_release)
  file(WRITE "${CMAKE_BINARY_DIR}/linux-runtime.txt"
    "${_foam_os_release}\nQt: ${Qt6_VERSION}\nOCCT: ${OpenCASCADE_MAJOR_VERSION}.${OpenCASCADE_MINOR_VERSION}.${OpenCASCADE_MAINTENANCE_VERSION}\nPrivate Qt/OCCT/ICU shared libraries; other dependencies managed by the distribution.\n")
  install(FILES "${CMAKE_BINARY_DIR}/linux-runtime.txt"
    DESTINATION share/doc/foamairplanestudio)

  string(TOUPPER "${FOAM_LINUX_PACKAGE_GENERATOR}"
    _foamairplanestudio_linux_package_generator)
  if(NOT _foamairplanestudio_linux_package_generator MATCHES "^(DEB|RPM)$")
    message(FATAL_ERROR
      "FOAM_LINUX_PACKAGE_GENERATOR must be DEB or RPM")
  endif()
  set(CPACK_GENERATOR "${_foamairplanestudio_linux_package_generator}")
  set(CPACK_PACKAGE_NAME foamairplanestudio)
  set(CPACK_PACKAGE_VENDOR "FoamAirplaneStudio")
  set(CPACK_PACKAGE_CONTACT
    "Barry Foust <barryfx@users.noreply.github.com>")
  set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "Foam RC airplane designer")
  set(CPACK_PACKAGE_DESCRIPTION "Sketch foam aircraft, inspect assemblies, calculate weight and balance, and export STEP, STL, DXF and SVG manufacturing parts.")
  set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/barryfx/FoamAirplaneStudio")
  set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
  if(PROJECT_VERSION_PATCH STREQUAL "")
    string(APPEND CPACK_PACKAGE_VERSION ".0")
  endif()
  set(CPACK_PACKAGE_DIRECTORY "${CMAKE_SOURCE_DIR}/dist")
  set(CPACK_PACKAGING_INSTALL_PREFIX /usr)
  set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
  if(_foamairplanestudio_linux_package_generator STREQUAL "DEB")
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE amd64)
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER
      "Barry Foust <barryfx@users.noreply.github.com>")
    set(CPACK_DEBIAN_PACKAGE_SECTION graphics)
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS_PRIVATE_DIRS /usr/lib/foamairplanestudio)
  else()
    set(CPACK_RPM_FILE_NAME
      "foamairplanestudio-${CPACK_PACKAGE_VERSION}-1.x86_64.rpm")
    set(CPACK_RPM_PACKAGE_ARCHITECTURE x86_64)
    set(CPACK_RPM_PACKAGE_RELEASE 1)
    set(CPACK_RPM_PACKAGE_LICENSE "GPL-3.0-only")
    set(CPACK_RPM_PACKAGE_GROUP "Applications/Engineering")
    set(CPACK_RPM_PACKAGE_AUTOREQ ON)
    set(CPACK_RPM_PACKAGE_AUTOPROV ON)
  endif()
  set(CPACK_STRIP_FILES ON)
  include(CPack)
endif()
