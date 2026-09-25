cmake_minimum_required(VERSION 3.24)

project(FoamAirplaneStudio VERSION 0.1.0 LANGUAGES CXX)

option(DESIGNRC_BUILD_TESTS "Build inherited baseline regression tests" ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  # Several Debug targets compile sources concurrently into one program
  # database. Serialize PDB writes so clean/parallel builds cannot race.
  add_compile_options(/FS)
endif()

if(NOT DEFINED OpenCASCADE_DIR)
  set(_designrc_occt "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/occt/install-debug/cmake")
  if(EXISTS "${_designrc_occt}/OpenCASCADEConfig.cmake")
    set(OpenCASCADE_DIR "${_designrc_occt}" CACHE PATH "OpenCascade package directory")
  endif()
endif()

# Optional sibling installation of the matching Qt PDF add-on.
if(WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/qt-pdf-6.11.1/lib/cmake/Qt6Pdf")
  set(Qt6Pdf_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/qt-pdf-6.11.1/lib/cmake/Qt6Pdf"
      CACHE PATH "Qt PDF module directory")
endif()
find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets Pdf)
find_package(Threads REQUIRED)
find_package(OpenCASCADE CONFIG REQUIRED COMPONENTS
  FoundationClasses
  ModelingData
  ModelingAlgorithms
  Visualization
  ApplicationFramework
  DataExchange
)

set(DESIGNRC_GENERATED_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${DESIGNRC_GENERATED_INCLUDE_DIR}")
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/resources/dxf/r2000_template.dxf"
     DESIGNRC_R2000_TEMPLATE_CONTENT)
set(DESIGNRC_R2000_HEADER
    "${DESIGNRC_GENERATED_INCLUDE_DIR}/R2000Template.h")
file(WRITE "${DESIGNRC_R2000_HEADER}"
  "#pragma once\n\n#include <string_view>\n\n"
  "namespace designrc::domain::detail {\n"
  "inline constexpr char r2000TemplateData[] =\n")
string(LENGTH "${DESIGNRC_R2000_TEMPLATE_CONTENT}"
       DESIGNRC_R2000_TEMPLATE_LENGTH)
set(DESIGNRC_R2000_TEMPLATE_OFFSET 0)
while(DESIGNRC_R2000_TEMPLATE_OFFSET LESS DESIGNRC_R2000_TEMPLATE_LENGTH)
  math(EXPR DESIGNRC_R2000_REMAINING
       "${DESIGNRC_R2000_TEMPLATE_LENGTH}-${DESIGNRC_R2000_TEMPLATE_OFFSET}")
  if(DESIGNRC_R2000_REMAINING GREATER 12000)
    set(DESIGNRC_R2000_CHUNK_LENGTH 12000)
  else()
    set(DESIGNRC_R2000_CHUNK_LENGTH "${DESIGNRC_R2000_REMAINING}")
  endif()
  string(SUBSTRING "${DESIGNRC_R2000_TEMPLATE_CONTENT}"
         "${DESIGNRC_R2000_TEMPLATE_OFFSET}"
         "${DESIGNRC_R2000_CHUNK_LENGTH}" DESIGNRC_R2000_TEMPLATE_CHUNK)
  file(APPEND "${DESIGNRC_R2000_HEADER}"
       "R\"DESIGNRC_DXF(${DESIGNRC_R2000_TEMPLATE_CHUNK})DESIGNRC_DXF\"\n")
  math(EXPR DESIGNRC_R2000_TEMPLATE_OFFSET
       "${DESIGNRC_R2000_TEMPLATE_OFFSET}+${DESIGNRC_R2000_CHUNK_LENGTH}")
endwhile()
file(APPEND "${DESIGNRC_R2000_HEADER}"
  ";\ninline constexpr std::string_view r2000Template{r2000TemplateData, "
  "sizeof(r2000TemplateData) - 1};\n"
  "} // namespace designrc::domain::detail\n")

add_library(designrc_domain
  src/domain/AirfoilProfile.cpp
  src/domain/AirfoilProfile.h
  src/domain/DxfExporter.cpp
  src/domain/DxfExporter.h
  src/domain/Point2.h
  src/domain/WingDesign.cpp
  src/domain/WingDesign.h
  src/domain/WingStructure.cpp
  src/domain/WingStructure.h
)
target_include_directories(designrc_domain
  PUBLIC src
  PRIVATE "${DESIGNRC_GENERATED_INCLUDE_DIR}"
)
target_compile_features(designrc_domain PUBLIC cxx_std_23)

add_library(foam_sketch
  src/gui/WeightBalancePanel.cpp src/gui/WeightBalancePanel.h
  src/gui/SketchBoundary.cpp src/gui/SketchBoundary.h
  src/gui/FormerEditor.cpp src/gui/FormerEditor.h src/gui/FormerPanel.h
  src/gui/ServoTrayEditor.cpp src/gui/ServoTrayEditor.h src/gui/ServoTrayPanel.h
  src/gui/FuselageCutPanel.cpp src/gui/FuselageCutPanel.h
  src/gui/FuselageThickenPanel.cpp src/gui/FuselageThickenPanel.h
  src/gui/FuselageProfilePanel.cpp src/gui/FuselageProfilePanel.h
  src/gui/StabilizerOutlinePanel.cpp src/gui/StabilizerOutlinePanel.h
  src/gui/StabilizerHingePanel.cpp src/gui/StabilizerHingePanel.h
  src/gui/StabilizerCutPanel.cpp src/gui/StabilizerCutPanel.h
  src/gui/FuselageOutlinePanel.cpp src/gui/FuselageOutlinePanel.h
  src/gui/SparPanel.cpp
  src/gui/ControlSurfaceEditor.cpp src/gui/ControlSurfacePanel.cpp
  src/gui/ConstrainedLineEditor.cpp src/gui/ConstrainedLineEditor.h
  src/gui/SketchEditor.cpp src/gui/SketchEditor.h
  src/gui/WingOutlinePanel.cpp src/gui/WingOutlinePanel.h)
set_target_properties(foam_sketch PROPERTIES AUTOMOC ON)
target_include_directories(foam_sketch PUBLIC src PRIVATE ${OpenCASCADE_INCLUDE_DIR})
target_link_libraries(foam_sketch PUBLIC Qt6::Widgets PRIVATE TKernel TKMath TKG2d TKGeomAlgo TKGeomBase)

add_library(foam_dialogs src/gui/FileSelectionDialog.cpp src/gui/FileSelectionDialog.h)
target_include_directories(foam_dialogs PUBLIC src)
target_link_libraries(foam_dialogs PUBLIC Qt6::Widgets)

add_library(foam_airfoils src/gui/StabilizerAirfoilPanel.cpp src/gui/StabilizerAirfoilPanel.h src/gui/AirfoilLibrary.cpp src/gui/AirfoilLibrary.h
  src/gui/AirfoilPanel.cpp src/gui/AirfoilPanel.h
  src/gui/AirfoilSmoothing.cpp src/gui/AirfoilSmoothing.h
  src/gui/AirfoilSmoothingDialog.cpp src/gui/AirfoilSmoothingDialog.h)
set_target_properties(foam_airfoils PROPERTIES AUTOMOC ON)
target_link_libraries(foam_airfoils PUBLIC foam_sketch foam_dialogs designrc_domain)

add_library(foam_wing src/geometry/FuselageSymmetry.cpp src/geometry/FuselageEndRegistration.cpp src/geometry/WeightBalance.cpp src/geometry/Assembly.cpp src/geometry/FuselageAlignment.cpp src/geometry/StabilizerCut.cpp src/geometry/StabilizerHingeCut.cpp src/geometry/StabilizerSolidBuilder.cpp src/geometry/StabilizerSolidBuilder.h src/geometry/Formers.cpp src/geometry/ServoTray.cpp src/geometry/FuselageHoles.cpp src/geometry/FuselageCut.cpp src/geometry/FuselageWall.cpp src/geometry/FuselageSolidBuilder.cpp src/geometry/MeshOrientation.cpp src/geometry/LighteningCut.cpp src/geometry/SparCut.cpp src/geometry/ControlSurfaceCut.cpp src/geometry/WingSolidBuilder.cpp src/geometry/WingSolidBuilder.h)
target_include_directories(foam_wing PUBLIC src ${OpenCASCADE_INCLUDE_DIR})
target_link_libraries(foam_wing PUBLIC Threads::Threads foam_airfoils TKernel TKMath TKG3d TKGeomBase TKGeomAlgo TKBRep TKTopAlgo TKPrim TKBO TKOffset TKMesh)
add_library(foam_project src/gui/ProjectDocument.cpp src/gui/ProjectDocument.h src/gui/AirplaneStatistics.cpp)
target_include_directories(foam_project PUBLIC src ${OpenCASCADE_INCLUDE_DIR})
target_link_libraries(foam_project PUBLIC foam_airfoils TKernel TKMath)

add_library(foam_export
  src/gui/InspectPanel.cpp src/gui/InspectPanel.h
  src/geometry/ComponentExporter.cpp src/geometry/ComponentExporter.h
  src/geometry/StepExporter.cpp src/geometry/StepExporter.h
  src/gui/ExportPanel.cpp src/gui/ExportPanel.h)
target_include_directories(foam_export PUBLIC src ${OpenCASCADE_INCLUDE_DIR})
target_link_libraries(foam_export PUBLIC foam_wing designrc_domain Qt6::Widgets TKDESTEP TKXCAF TKDESTL)

add_executable(designrc WIN32
  resources/designrc.qrc
  src/main.cpp
  src/gui/ReferenceImage.cpp
  src/gui/ReferenceImage.h
  src/gui/ReferencePanel.cpp
  src/gui/ReferencePanel.h
  src/gui/ReferenceWorkflow.cpp
  src/gui/ReferenceWorkflow.h
  src/geometry/OcctRibBuilder.cpp
  src/geometry/OcctRibBuilder.h
  src/gui/WingWorkflow.cpp
  src/gui/WingWorkflow.h
  src/gui/MainWindow.cpp src/gui/AssemblyWorkflow.cpp
  src/gui/MainWindow.h
  src/gui/OcctViewport.cpp
  src/gui/OcctViewport.h
  src/gui/PlanViewport.cpp
  src/gui/PlanViewport.h
  src/gui/PartPdfExporter.cpp
  src/gui/PartPdfExporter.h
  src/gui/TechnicalDrawing.h
)
set_target_properties(designrc PROPERTIES
  OUTPUT_NAME foamairplanestudio
  AUTOMOC ON
  AUTORCC ON
)
if(WIN32)
  target_sources(designrc PRIVATE
    resources/windows/app.rc
  )
endif()
target_include_directories(designrc PRIVATE src ${OpenCASCADE_INCLUDE_DIR})
target_compile_definitions(designrc PRIVATE
  DESIGNRC_VERSION="${PROJECT_VERSION}"
  DESIGNRC_RELEASE_DATE="September 13, 2026"
)
target_link_libraries(designrc PRIVATE
  foam_export
  foam_sketch
  foam_dialogs
  foam_airfoils
  foam_wing
  foam_project
  designrc_domain
  Qt6::Widgets Qt6::Pdf
  TKernel TKMath TKService TKV3d TKOpenGl TKG2d TKG3d
  TKBRep TKTopAlgo TKPrim TKBO TKOffset TKMesh TKShHealing
  TKDESTEP TKXCAF
)

if(UNIX AND NOT APPLE)
  # Use an inherited ELF RPATH so OCCT shared libraries can also resolve their
  # sibling, transitive OCCT dependencies from the custom installation.
  set_target_properties(designrc PROPERTIES
    BUILD_RPATH "${OpenCASCADE_LIBRARY_DIR}"
  )
  target_link_options(designrc PRIVATE "LINKER:--disable-new-dtags")
endif()

if(MSVC)
  target_compile_options(designrc_domain PRIVATE /W4 /permissive-)
  target_compile_options(designrc PRIVATE /W4 /permissive-)
endif()

if(WIN32)
  get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
  get_filename_component(QT_BIN_DIR "${QT_QMAKE_EXECUTABLE}" DIRECTORY)
  find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${QT_BIN_DIR}" REQUIRED)
  execute_process(
    COMMAND "${QT_QMAKE_EXECUTABLE}" -query QT_INSTALL_PREFIX
    OUTPUT_VARIABLE QT_INSTALL_PREFIX
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  execute_process(
    COMMAND "${QT_QMAKE_EXECUTABLE}" -query QT_INSTALL_DOCS
    OUTPUT_VARIABLE QT_INSTALL_DOCS
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  set(DESIGNRC_OCCT_RUNTIME_DIR "" CACHE PATH
      "Directory containing the OpenCascade runtime DLLs")
  if(NOT DESIGNRC_OCCT_RUNTIME_DIR)
    if(EXISTS "${OpenCASCADE_INSTALL_PREFIX}/win64/vc14/bin")
      set(DESIGNRC_OCCT_RUNTIME_DIR
          "${OpenCASCADE_INSTALL_PREFIX}/win64/vc14/bin")
    elseif(EXISTS "${OpenCASCADE_INSTALL_PREFIX}/win64/vc14/bind")
      set(DESIGNRC_OCCT_RUNTIME_DIR
          "${OpenCASCADE_INSTALL_PREFIX}/win64/vc14/bind")
    else()
      message(FATAL_ERROR "OpenCascade runtime DLL directory was not found")
    endif()
  endif()
  set(_occt_freetype
    "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/occt/3rdparty-vc14-64/freetype-2.13.3-x64/bin/freetype.dll")
  if(OpenCASCADE_WITH_FREETYPE AND NOT EXISTS "${_occt_freetype}")
    message(FATAL_ERROR "OpenCascade requires FreeType, but freetype.dll was not found at ${_occt_freetype}")
  endif()
  # Do not overwrite identical OCCT DLLs that an existing project session has
  # loaded. This allows a rebuilt executable to coexist with that session.
  file(GLOB _foam_occt_runtime_files "${DESIGNRC_OCCT_RUNTIME_DIR}/*.dll")
  add_custom_command(TARGET designrc POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:Qt6::Pdf>" "$<TARGET_FILE_DIR:designrc>"
    COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-translations "$<TARGET_FILE:designrc>"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
      ${_foam_occt_runtime_files} "$<TARGET_FILE_DIR:designrc>"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
      "${_occt_freetype}" "$<TARGET_FILE_DIR:designrc>"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
      "${CMAKE_CURRENT_SOURCE_DIR}/resources/help" "$<TARGET_FILE_DIR:designrc>/help"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
      "${CMAKE_CURRENT_SOURCE_DIR}/resources/licenses" "$<TARGET_FILE_DIR:designrc>/licenses"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
      "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
      "$<TARGET_FILE_DIR:designrc>/licenses/DESIGNRC-GPL-3.0.txt"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
      "${CMAKE_CURRENT_SOURCE_DIR}/THIRD_PARTY_NOTICES.md"
      "$<TARGET_FILE_DIR:designrc>/licenses/THIRD_PARTY_NOTICES.md"
    COMMENT "Deploying Qt and OpenCascade runtimes"
    VERBATIM
  )
  if(EXISTS "${QT_INSTALL_PREFIX}/sbom/qtbase-${Qt6_VERSION}.spdx")
    add_custom_command(TARGET designrc POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E make_directory
        "$<TARGET_FILE_DIR:designrc>/licenses/qt-sbom"
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${QT_INSTALL_PREFIX}/sbom/qtbase-${Qt6_VERSION}.spdx"
        "$<TARGET_FILE_DIR:designrc>/licenses/qt-sbom/qtbase-${Qt6_VERSION}.spdx"
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${QT_INSTALL_PREFIX}/sbom/qtsvg-${Qt6_VERSION}.spdx"
        "$<TARGET_FILE_DIR:designrc>/licenses/qt-sbom/qtsvg-${Qt6_VERSION}.spdx"
      COMMENT "Deploying Qt software bills of materials"
      VERBATIM
    )
  endif()
  if(EXISTS "${QT_INSTALL_PREFIX}/sbom/qtpdf-${Qt6_VERSION}.spdx")
    add_custom_command(TARGET designrc POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:designrc>/licenses/qt-sbom"
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${QT_INSTALL_PREFIX}/sbom/qtpdf-${Qt6_VERSION}.spdx"
        "$<TARGET_FILE_DIR:designrc>/licenses/qt-sbom/qtpdf-${Qt6_VERSION}.spdx"
      VERBATIM)
  endif()
  if(EXISTS "${QT_INSTALL_DOCS}/qtdoc/licenses-used-in-qt.html")
    add_custom_command(TARGET designrc POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E make_directory
        "$<TARGET_FILE_DIR:designrc>/licenses/qt-attributions"
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${QT_INSTALL_DOCS}/qtdoc/licenses-used-in-qt.html"
        "$<TARGET_FILE_DIR:designrc>/licenses/qt-attributions/licenses-used-in-qt.html"
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${QT_INSTALL_DOCS}/qtdoc/qt-attribution-llvmpipe.html"
        "$<TARGET_FILE_DIR:designrc>/licenses/qt-attributions/qt-attribution-llvmpipe.html"
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${QT_INSTALL_DOCS}/qtdoc/qt-attribution-llvm.html"
        "$<TARGET_FILE_DIR:designrc>/licenses/qt-attributions/qt-attribution-llvm.html"
      COMMENT "Deploying Qt and Mesa attribution documents"
      VERBATIM
    )
  endif()
endif()

if(DESIGNRC_BUILD_TESTS)
  enable_testing()
  add_executable(project_tests tests/ProjectTests.cpp resources/designrc.qrc
    src/gui/MainWindow.cpp src/gui/AssemblyWorkflow.cpp src/gui/OcctViewport.cpp src/gui/PlanViewport.cpp
    src/gui/ReferencePanel.cpp src/gui/ReferencePanel.h src/gui/ReferenceImage.cpp
    src/gui/ReferenceWorkflow.cpp src/gui/WingWorkflow.cpp)
  set_target_properties(project_tests PROPERTIES AUTOMOC ON AUTORCC ON)
  target_include_directories(project_tests PRIVATE src ${OpenCASCADE_INCLUDE_DIR})
  get_target_property(_foam_project_app_libraries designrc LINK_LIBRARIES)
  target_link_libraries(project_tests PRIVATE ${_foam_project_app_libraries})
  add_test(NAME project_tests COMMAND project_tests)
  add_executable(view_controls_tests tests/ViewControlsTests.cpp resources/designrc.qrc
    src/gui/MainWindow.cpp src/gui/AssemblyWorkflow.cpp src/gui/OcctViewport.cpp src/gui/PlanViewport.cpp
    src/gui/ReferencePanel.cpp src/gui/ReferencePanel.h src/gui/ReferenceImage.cpp
    src/gui/ReferenceWorkflow.cpp src/gui/WingWorkflow.cpp)
  set_target_properties(view_controls_tests PROPERTIES AUTOMOC ON AUTORCC ON)
  target_include_directories(view_controls_tests PRIVATE src ${OpenCASCADE_INCLUDE_DIR})
  target_link_libraries(view_controls_tests PRIVATE ${_foam_project_app_libraries})
  add_test(NAME view_controls_tests COMMAND view_controls_tests "${CMAKE_CURRENT_BINARY_DIR}/view-controls.png")
  add_test(NAME startup_gui_tests COMMAND view_controls_tests --startup-only "${CMAKE_CURRENT_BINARY_DIR}")
  add_executable(editor_history_tests tests/EditorHistoryTests.cpp resources/designrc.qrc
    src/gui/MainWindow.cpp src/gui/AssemblyWorkflow.cpp src/gui/OcctViewport.cpp src/gui/PlanViewport.cpp
    src/gui/ReferencePanel.cpp src/gui/ReferencePanel.h src/gui/ReferenceImage.cpp
    src/gui/ReferenceWorkflow.cpp src/gui/WingWorkflow.cpp)
  set_target_properties(editor_history_tests PROPERTIES AUTOMOC ON AUTORCC ON)
  target_include_directories(editor_history_tests PRIVATE src ${OpenCASCADE_INCLUDE_DIR})
  target_link_libraries(editor_history_tests PRIVATE ${_foam_project_app_libraries})
  add_test(NAME editor_history_tests COMMAND editor_history_tests "${CMAKE_CURRENT_BINARY_DIR}/editor-history.png")
  add_executable(inspect_tests tests/InspectTests.cpp resources/designrc.qrc
    src/gui/MainWindow.cpp src/gui/AssemblyWorkflow.cpp src/gui/OcctViewport.cpp src/gui/PlanViewport.cpp
    src/gui/ReferencePanel.cpp src/gui/ReferencePanel.h src/gui/ReferenceImage.cpp
    src/gui/ReferenceWorkflow.cpp src/gui/WingWorkflow.cpp)
  set_target_properties(inspect_tests PROPERTIES AUTOMOC ON AUTORCC ON)
  target_include_directories(inspect_tests PRIVATE src ${OpenCASCADE_INCLUDE_DIR})
  target_link_libraries(inspect_tests PRIVATE ${_foam_project_app_libraries})
  add_test(NAME inspect_tests COMMAND inspect_tests "${CMAKE_CURRENT_BINARY_DIR}/inspect.png")
  add_executable(weight_balance_tests tests/WeightBalanceTests.cpp resources/designrc.qrc
    src/gui/MainWindow.cpp src/gui/AssemblyWorkflow.cpp src/gui/OcctViewport.cpp src/gui/PlanViewport.cpp
    src/gui/ReferencePanel.cpp src/gui/ReferencePanel.h src/gui/ReferenceImage.cpp
    src/gui/ReferenceWorkflow.cpp src/gui/WingWorkflow.cpp)
  set_target_properties(weight_balance_tests PROPERTIES AUTOMOC ON AUTORCC ON)
  target_include_directories(weight_balance_tests PRIVATE src ${OpenCASCADE_INCLUDE_DIR})
  target_link_libraries(weight_balance_tests PRIVATE ${_foam_project_app_libraries})
  add_test(NAME weight_balance_tests COMMAND weight_balance_tests "${CMAKE_CURRENT_BINARY_DIR}")
  if(NOT WIN32)
    set_tests_properties(weight_balance_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
  endif()
  add_executable(airplane_statistics_tests tests/AirplaneStatisticsTests.cpp resources/designrc.qrc
    src/gui/MainWindow.cpp src/gui/AssemblyWorkflow.cpp src/gui/OcctViewport.cpp src/gui/PlanViewport.cpp
    src/gui/ReferencePanel.cpp src/gui/ReferencePanel.h src/gui/ReferenceImage.cpp
    src/gui/ReferenceWorkflow.cpp src/gui/WingWorkflow.cpp)
  set_target_properties(airplane_statistics_tests PROPERTIES AUTOMOC ON AUTORCC ON)
  target_include_directories(airplane_statistics_tests PRIVATE src ${OpenCASCADE_INCLUDE_DIR})
  target_link_libraries(airplane_statistics_tests PRIVATE ${_foam_project_app_libraries})
  add_test(NAME airplane_statistics_tests COMMAND airplane_statistics_tests "${CMAKE_CURRENT_BINARY_DIR}")
  if(NOT WIN32)
    set_tests_properties(airplane_statistics_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
  endif()
  add_executable(assembly_tests tests/AssemblyTests.cpp resources/designrc.qrc
    src/gui/MainWindow.cpp src/gui/AssemblyWorkflow.cpp src/gui/OcctViewport.cpp src/gui/PlanViewport.cpp
    src/gui/ReferencePanel.cpp src/gui/ReferencePanel.h src/gui/ReferenceImage.cpp
    src/gui/ReferenceWorkflow.cpp src/gui/WingWorkflow.cpp)
  set_target_properties(assembly_tests PROPERTIES AUTOMOC ON AUTORCC ON)
  target_include_directories(assembly_tests PRIVATE src ${OpenCASCADE_INCLUDE_DIR})
  target_link_libraries(assembly_tests PRIVATE ${_foam_project_app_libraries})
  add_test(NAME assembly_tests COMMAND assembly_tests "${CMAKE_CURRENT_BINARY_DIR}")
  add_test(NAME export_tests COMMAND assembly_tests --export "${CMAKE_CURRENT_BINARY_DIR}")
  add_test(NAME project_persistence_ui_tests COMMAND assembly_tests --persistence-ui "${CMAKE_BINARY_DIR}")
  if(NOT WIN32)
    set_tests_properties(assembly_tests export_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
  endif()
  add_executable(stabilizer_outline_tests tests/StabilizerOutlineTests.cpp resources/designrc.qrc
    src/gui/MainWindow.cpp src/gui/AssemblyWorkflow.cpp src/gui/OcctViewport.cpp src/gui/PlanViewport.cpp
    src/gui/ReferencePanel.cpp src/gui/ReferencePanel.h src/gui/ReferenceImage.cpp
    src/gui/ReferenceWorkflow.cpp src/gui/WingWorkflow.cpp)
  set_target_properties(stabilizer_outline_tests PROPERTIES AUTOMOC ON AUTORCC ON)
  target_include_directories(stabilizer_outline_tests PRIVATE src ${OpenCASCADE_INCLUDE_DIR})
  target_link_libraries(stabilizer_outline_tests PRIVATE ${_foam_project_app_libraries})
  add_test(NAME stabilizer_outline_tests COMMAND stabilizer_outline_tests)
  add_executable(stabilizer_bevel_tests tests/StabilizerBevelTests.cpp)
  target_include_directories(stabilizer_bevel_tests PRIVATE src ${OpenCASCADE_INCLUDE_DIR})
  target_link_libraries(stabilizer_bevel_tests PRIVATE ${_foam_project_app_libraries})
  add_test(NAME stabilizer_bevel_tests COMMAND stabilizer_bevel_tests "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/babybuzzard-stabilizer-bevels.json")
  set_tests_properties(stabilizer_bevel_tests PROPERTIES TIMEOUT 1200)
  if(NOT WIN32)
    set_tests_properties(stabilizer_outline_tests stabilizer_bevel_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
  endif()

  add_executable(fuselage_outline_tests tests/FuselageOutlineTests.cpp resources/designrc.qrc
    src/gui/MainWindow.cpp src/gui/AssemblyWorkflow.cpp src/gui/OcctViewport.cpp src/gui/PlanViewport.cpp
    src/gui/ReferencePanel.cpp src/gui/ReferencePanel.h src/gui/ReferenceImage.cpp
    src/gui/ReferenceWorkflow.cpp src/gui/WingWorkflow.cpp)
  set_target_properties(fuselage_outline_tests PROPERTIES AUTOMOC ON AUTORCC ON)
  target_include_directories(fuselage_outline_tests PRIVATE src ${OpenCASCADE_INCLUDE_DIR})
  target_link_libraries(fuselage_outline_tests PRIVATE ${_foam_project_app_libraries})
  add_test(NAME fuselage_outline_tests COMMAND fuselage_outline_tests)
  if(NOT WIN32)
    set_tests_properties(fuselage_outline_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
  endif()

  add_executable(fuselage_profile_tests tests/FuselageProfileTests.cpp resources/designrc.qrc
    src/gui/MainWindow.cpp src/gui/AssemblyWorkflow.cpp src/gui/OcctViewport.cpp src/gui/PlanViewport.cpp
    src/gui/ReferencePanel.cpp src/gui/ReferencePanel.h src/gui/ReferenceImage.cpp
    src/gui/ReferenceWorkflow.cpp src/gui/WingWorkflow.cpp)
  set_target_properties(fuselage_profile_tests PROPERTIES AUTOMOC ON AUTORCC ON)
  target_include_directories(fuselage_profile_tests PRIVATE src ${OpenCASCADE_INCLUDE_DIR})
  target_link_libraries(fuselage_profile_tests PRIVATE ${_foam_project_app_libraries})
  add_test(NAME fuselage_profile_tests COMMAND fuselage_profile_tests)
  add_test(NAME circle_profile_tests COMMAND fuselage_profile_tests --circle-only)
  add_test(NAME profile_operations_tests COMMAND fuselage_profile_tests --operations-only)
  add_test(NAME profile_recovery_tests COMMAND fuselage_profile_tests --recovery-only)
  if(NOT WIN32)
    set_tests_properties(fuselage_profile_tests circle_profile_tests profile_operations_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
  endif()

  add_executable(former_diagnostics_tests tests/FormerDiagnosticsTests.cpp)
  target_include_directories(former_diagnostics_tests PRIVATE src)
  target_link_libraries(former_diagnostics_tests PRIVATE Qt6::Core)
  add_test(NAME former_diagnostics_tests COMMAND former_diagnostics_tests)
  add_executable(former_tests tests/FormerTests.cpp resources/designrc.qrc
    src/gui/MainWindow.cpp src/gui/AssemblyWorkflow.cpp src/gui/OcctViewport.cpp src/gui/PlanViewport.cpp
    src/gui/ReferencePanel.cpp src/gui/ReferencePanel.h src/gui/ReferenceImage.cpp
    src/gui/ReferenceWorkflow.cpp src/gui/WingWorkflow.cpp)
  set_target_properties(former_tests PROPERTIES AUTOMOC ON AUTORCC ON)
  target_include_directories(former_tests PRIVATE src ${OpenCASCADE_INCLUDE_DIR})
  target_link_libraries(former_tests PRIVATE ${_foam_project_app_libraries})
  add_test(NAME fuselage_holes_tests COMMAND former_tests --holes "${CMAKE_BINARY_DIR}")
  add_test(NAME former_tests COMMAND former_tests)
  add_test(NAME fuselage_readiness_tests COMMAND former_tests --readiness-only)
  add_test(NAME former_defaults_tests COMMAND former_tests --defaults-only)
  add_test(NAME former_geometry_tests COMMAND former_tests --geometry-only)
  if(NOT WIN32)
    set_tests_properties(fuselage_readiness_tests former_tests former_defaults_tests former_geometry_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
  endif()

  add_executable(servo_tray_tests tests/ServoTrayTests.cpp resources/designrc.qrc
    src/gui/MainWindow.cpp src/gui/AssemblyWorkflow.cpp src/gui/OcctViewport.cpp src/gui/PlanViewport.cpp
    src/gui/ReferencePanel.cpp src/gui/ReferencePanel.h src/gui/ReferenceImage.cpp
    src/gui/ReferenceWorkflow.cpp src/gui/WingWorkflow.cpp)
  set_target_properties(servo_tray_tests PROPERTIES AUTOMOC ON AUTORCC ON)
  target_include_directories(servo_tray_tests PRIVATE src ${OpenCASCADE_INCLUDE_DIR})
  target_link_libraries(servo_tray_tests PRIVATE ${_foam_project_app_libraries})
  add_test(NAME servo_tray_tests COMMAND servo_tray_tests)
  add_test(NAME servo_tray_geometry_tests COMMAND servo_tray_tests --geometry-only)
  if(NOT WIN32)
    set_tests_properties(servo_tray_tests servo_tray_geometry_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
  endif()


  add_executable(fuselage_cut_tests tests/FuselageCutTests.cpp resources/designrc.qrc
    src/gui/MainWindow.cpp src/gui/AssemblyWorkflow.cpp src/gui/OcctViewport.cpp src/gui/PlanViewport.cpp
    src/gui/ReferencePanel.cpp src/gui/ReferencePanel.h src/gui/ReferenceImage.cpp
    src/gui/ReferenceWorkflow.cpp src/gui/WingWorkflow.cpp)
  set_target_properties(fuselage_cut_tests PROPERTIES AUTOMOC ON AUTORCC ON)
  target_include_directories(fuselage_cut_tests PRIVATE src ${OpenCASCADE_INCLUDE_DIR})
  target_link_libraries(fuselage_cut_tests PRIVATE ${_foam_project_app_libraries})
  add_test(NAME fuselage_cut_tests COMMAND fuselage_cut_tests)
  add_test(NAME fuselage_alignment_tests COMMAND fuselage_cut_tests --alignment-only)
  if(NOT WIN32)
    set_tests_properties(fuselage_cut_tests fuselage_alignment_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
  endif()


  add_executable(fuselage_end_tests tests/FuselageEndTests.cpp)
  target_link_libraries(fuselage_end_tests PRIVATE foam_wing foam_project TKV3d TKService)
  add_test(NAME fuselage_end_tests COMMAND fuselage_end_tests)

  add_executable(fuselage_thicken_tests tests/FuselageThickenTests.cpp resources/designrc.qrc
    src/gui/MainWindow.cpp src/gui/AssemblyWorkflow.cpp src/gui/OcctViewport.cpp src/gui/PlanViewport.cpp
    src/gui/ReferencePanel.cpp src/gui/ReferencePanel.h src/gui/ReferenceImage.cpp
    src/gui/ReferenceWorkflow.cpp src/gui/WingWorkflow.cpp)
  set_target_properties(fuselage_thicken_tests PROPERTIES AUTOMOC ON AUTORCC ON)
  target_include_directories(fuselage_thicken_tests PRIVATE src ${OpenCASCADE_INCLUDE_DIR})
  target_link_libraries(fuselage_thicken_tests PRIVATE ${_foam_project_app_libraries})
  add_test(NAME fuselage_thicken_tests COMMAND fuselage_thicken_tests)
  add_test(NAME fuselage_thicken_ui_tests COMMAND fuselage_thicken_tests --ui-only)
  if(NOT WIN32)
    set_tests_properties(fuselage_thicken_tests fuselage_thicken_ui_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
  endif()

  add_executable(fuselage_orientation_tests tests/FuselageOrientationTests.cpp src/gui/OcctViewport.cpp)
  target_link_libraries(fuselage_orientation_tests PRIVATE foam_wing foam_project TKV3d TKService TKOpenGl)
  add_test(NAME fuselage_orientation_tests COMMAND fuselage_orientation_tests)

  add_executable(control_surface_editor_tests tests/ControlSurfaceEditorTests.cpp src/gui/PlanViewport.cpp)
  target_link_libraries(control_surface_editor_tests PRIVATE foam_sketch)
  add_test(NAME control_surface_editor_tests COMMAND control_surface_editor_tests)
  add_executable(control_surface_benchmark tests/ControlSurfaceBenchmark.cpp)
  target_link_libraries(control_surface_benchmark PRIVATE foam_wing)
  add_executable(lightening_tests tests/LighteningTests.cpp src/gui/OcctViewport.cpp)
  target_link_libraries(lightening_tests PRIVATE foam_wing foam_project TKV3d TKService TKOpenGl)
  add_test(NAME lightening_tests COMMAND lightening_tests)
  if(NOT WIN32)
    set_tests_properties(lightening_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
  endif()
  add_executable(mesh_orientation_tests tests/MeshOrientationTests.cpp)
  target_link_libraries(mesh_orientation_tests PRIVATE foam_wing)
  add_test(NAME mesh_orientation_tests COMMAND mesh_orientation_tests)
  add_executable(processing_tests tests/ProcessingTests.cpp)
  target_link_libraries(processing_tests PRIVATE foam_wing)
  add_test(NAME processing_tests COMMAND processing_tests)
  add_executable(fuselage_wall_offset_tests tests/FuselageWallOffsetTests.cpp)
  target_link_libraries(fuselage_wall_offset_tests PRIVATE foam_wing)
  add_test(NAME fuselage_wall_offset_tests COMMAND fuselage_wall_offset_tests)
  add_executable(fuselage_symmetry_tests tests/FuselageSymmetryTests.cpp)
  target_link_libraries(fuselage_symmetry_tests PRIVATE foam_wing foam_project TKV3d TKService)
  add_test(NAME fuselage_symmetry_tests COMMAND fuselage_symmetry_tests)
  add_test(NAME fuselage_cancellation_tests COMMAND fuselage_symmetry_tests --cancel-stages)
  set_tests_properties(fuselage_cancellation_tests PROPERTIES TIMEOUT 600)
  add_executable(fuselage_benchmark tests/FuselageBenchmark.cpp)
  add_executable(former_rail_cut_tests tests/FormerRailCutTests.cpp)
  target_link_libraries(former_rail_cut_tests PRIVATE foam_wing)
  add_test(NAME former_rail_cut_tests COMMAND former_rail_cut_tests "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/babybuzzard36-former4" 417.462)
  add_test(NAME former_rear_rail_cut_tests COMMAND former_rail_cut_tests "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/babybuzzard36-former4-rear" 401.579)
  target_link_libraries(fuselage_benchmark PRIVATE foam_wing foam_project TKV3d TKService)

  add_executable(regeneration_benchmark tests/RegenerationBenchmark.cpp)
  target_link_libraries(regeneration_benchmark PRIVATE foam_wing foam_project TKV3d TKService)
  add_executable(spar_tests tests/SparTests.cpp)
  target_link_libraries(spar_tests PRIVATE foam_wing foam_project TKV3d TKService)
  add_test(NAME spar_tests COMMAND spar_tests)
  add_executable(spar_panel_tests tests/SparPanelTests.cpp)
  target_link_libraries(spar_panel_tests PRIVATE foam_sketch)
  add_test(NAME spar_panel_tests COMMAND spar_panel_tests)
  if(NOT WIN32)
    set_tests_properties(spar_panel_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
  endif()
  add_executable(control_surface_tests tests/ControlSurfaceTests.cpp)
  target_link_libraries(control_surface_tests PRIVATE foam_wing)
  add_test(NAME control_surface_tests COMMAND control_surface_tests)
  add_executable(wing_solid_tests tests/WingSolidTests.cpp)
  target_link_libraries(wing_solid_tests PRIVATE foam_wing)
  add_test(NAME wing_solid_tests COMMAND wing_solid_tests)
  add_executable(airfoil_panel_tests tests/AirfoilPanelTests.cpp src/gui/PlanViewport.cpp)
  target_link_libraries(airfoil_panel_tests PRIVATE foam_airfoils)
  add_test(NAME airfoil_panel_tests COMMAND airfoil_panel_tests "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/babybuzzard-airfoil.dat")
  if(NOT WIN32)
    set_tests_properties(airfoil_panel_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
  endif()
  add_executable(station_workflow_tests tests/StationWorkflowTests.cpp
    resources/designrc.qrc
    src/gui/MainWindow.cpp src/gui/AssemblyWorkflow.cpp src/gui/OcctViewport.cpp src/gui/PlanViewport.cpp
    src/gui/ReferencePanel.cpp src/gui/ReferencePanel.h src/gui/ReferenceImage.cpp
    src/gui/ReferenceWorkflow.cpp src/gui/WingWorkflow.cpp)
  set_target_properties(station_workflow_tests PROPERTIES AUTOMOC ON AUTORCC ON)
  target_include_directories(station_workflow_tests PRIVATE src ${OpenCASCADE_INCLUDE_DIR})
  get_target_property(_foam_app_libraries designrc LINK_LIBRARIES)
  target_link_libraries(station_workflow_tests PRIVATE ${_foam_app_libraries})
  add_test(NAME station_workflow_tests COMMAND station_workflow_tests)
  if(NOT WIN32)
    set_tests_properties(station_workflow_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
  endif()
  add_executable(station_sketch_tests tests/StationSketchTests.cpp src/gui/PlanViewport.cpp)
  target_link_libraries(station_sketch_tests PRIVATE foam_sketch)
  add_test(NAME station_sketch_tests COMMAND station_sketch_tests)
  if(NOT WIN32)
    set_tests_properties(station_sketch_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
  endif()
  add_executable(file_dialog_tests tests/FileDialogTests.cpp)
  target_link_libraries(file_dialog_tests PRIVATE foam_dialogs)
  add_test(NAME file_dialog_tests COMMAND file_dialog_tests)
  if(NOT WIN32)
    set_tests_properties(file_dialog_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
  endif()
  add_executable(sketch_editor_tests tests/SketchEditorTests.cpp src/gui/PlanViewport.cpp)
  target_link_libraries(sketch_editor_tests PRIVATE foam_sketch)
  add_test(NAME sketch_editor_tests COMMAND sketch_editor_tests)
  if(NOT WIN32)
    set_tests_properties(sketch_editor_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
  endif()
  add_executable(wing_workflow_tests tests/WingWorkflowTests.cpp src/gui/WingWorkflow.cpp)
  target_include_directories(wing_workflow_tests PRIVATE src)
  target_link_libraries(wing_workflow_tests PRIVATE Qt6::Widgets)
  add_test(NAME wing_workflow_tests COMMAND wing_workflow_tests)
  if(NOT WIN32)
    set_tests_properties(wing_workflow_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
  endif()
  add_executable(reference_tests
    tests/ReferenceTests.cpp
    src/gui/ReferenceWorkflow.cpp
    src/gui/ReferenceImage.cpp
    src/gui/ReferencePanel.cpp
    src/gui/ReferencePanel.h)
  set_target_properties(reference_tests PROPERTIES AUTOMOC ON)
  target_include_directories(reference_tests PRIVATE src)
  target_link_libraries(reference_tests PRIVATE Qt6::Widgets Qt6::Pdf foam_dialogs foam_sketch)
  add_test(NAME reference_tests COMMAND reference_tests)
  if(NOT WIN32)
    set_tests_properties(reference_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
  endif()
  add_executable(designrc_domain_tests tests/AirfoilProfileTests.cpp)
  target_link_libraries(designrc_domain_tests PRIVATE designrc_domain)
  add_test(NAME designrc_domain_tests COMMAND designrc_domain_tests)

  add_executable(designrc_gui_tests
    tests/GuiViewportTests.cpp
    src/gui/PlanViewport.cpp
    src/gui/PlanViewport.h
    src/gui/PartPdfExporter.cpp
    src/gui/PartPdfExporter.h
      src/gui/TechnicalDrawing.h
      )
  set_target_properties(designrc_gui_tests PROPERTIES AUTOMOC ON)
  target_include_directories(designrc_gui_tests PRIVATE src)
  target_link_libraries(designrc_gui_tests PRIVATE designrc_domain Qt6::Widgets foam_sketch)
  add_test(NAME designrc_gui_tests COMMAND designrc_gui_tests)

  add_executable(designrc_geometry_tests tests/OcctGeometryTests.cpp)
  add_test(NAME designrc_geometry_tests COMMAND designrc_geometry_tests)
  set_tests_properties(designrc_geometry_tests PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(designrc_step_export_tests
    tests/StepExporterTests.cpp
    src/geometry/OcctRibBuilder.cpp
    src/geometry/StepExporter.cpp
    src/geometry/StepExporter.h
  )
  target_include_directories(designrc_step_export_tests PRIVATE
    src ${OpenCASCADE_INCLUDE_DIR})
  target_link_libraries(designrc_step_export_tests PRIVATE
    designrc_domain TKernel TKMath TKG2d TKG3d TKGeomBase TKGeomAlgo
    TKBRep TKTopAlgo TKPrim TKBO TKOffset TKMesh TKShHealing TKDESTEP TKXCAF)
  add_test(NAME designrc_step_export_tests COMMAND designrc_step_export_tests)
endif()








