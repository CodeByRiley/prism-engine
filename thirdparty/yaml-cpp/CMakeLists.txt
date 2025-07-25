# 3.5 is actually available almost everywhere.
# 3.30 as the upper policy limit avoids CMake deprecation warnings.
cmake_minimum_required(VERSION 3.5...3.30)

# enable MSVC_RUNTIME_LIBRARY target property
# see https://cmake.org/cmake/help/latest/policy/CMP0091.html
if(POLICY CMP0091)
  cmake_policy(SET CMP0091 NEW)
endif()

project(YAML_CPP VERSION 0.8.0 LANGUAGES CXX)

set(YAML_CPP_MAIN_PROJECT OFF)
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  set(YAML_CPP_MAIN_PROJECT ON)
endif()

include(CMakePackageConfigHelpers)
include(CMakeDependentOption)
include(CheckCXXCompilerFlag)
include(GNUInstallDirs)
include(CTest)

option(YAML_CPP_BUILD_CONTRIB "Enable yaml-cpp contrib in library" ON)
option(YAML_CPP_BUILD_TOOLS "Enable parse tools" ON)
option(YAML_BUILD_SHARED_LIBS "Build yaml-cpp shared library" ${BUILD_SHARED_LIBS})
option(YAML_CPP_INSTALL "Enable generation of yaml-cpp install targets" ${YAML_CPP_MAIN_PROJECT})
option(YAML_CPP_FORMAT_SOURCE "Format source" ${YAML_CPP_MAIN_PROJECT})
option(YAML_CPP_DISABLE_UNINSTALL  "Disable uninstallation of yaml-cpp" OFF)
option(YAML_USE_SYSTEM_GTEST "Use system googletest if found" OFF)
option(YAML_ENABLE_PIC "Use Position-Independent Code " ON)

cmake_dependent_option(YAML_MSVC_SHARED_RT
  "MSVC: Build yaml-cpp with shared runtime libs (/MD)" ON
  "CMAKE_SYSTEM_NAME MATCHES Windows" OFF)
set(YAML_CPP_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/yaml-cpp"
  CACHE STRING "Path to install the CMake package to")
 
if (YAML_CPP_FORMAT_SOURCE)
    find_program(YAML_CPP_CLANG_FORMAT_EXE NAMES clang-format)
endif()

if (YAML_BUILD_SHARED_LIBS)
  set(yaml-cpp-type SHARED)
  set(yaml-cpp-label-postfix "shared")
else()
  set(yaml-cpp-type STATIC)
  set(yaml-cpp-label-postfix "static")
endif()

set(build-shared $<BOOL:${YAML_BUILD_SHARED_LIBS}>)
set(build-windows-dll $<AND:$<BOOL:${CMAKE_HOST_WIN32}>,${build-shared}>)
set(not-msvc $<NOT:$<CXX_COMPILER_ID:MSVC>>)
set(msvc-shared_rt $<BOOL:${YAML_MSVC_SHARED_RT}>)

if (NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
  set(CMAKE_MSVC_RUNTIME_LIBRARY
    MultiThreaded$<$<CONFIG:Debug>:Debug>$<${msvc-shared_rt}:DLL>)
endif()

set(contrib-pattern "src/contrib/*.cpp")
set(src-pattern "src/*.cpp")
if (CMAKE_VERSION VERSION_GREATER 3.12)
  list(INSERT contrib-pattern 0 CONFIGURE_DEPENDS)
  list(INSERT src-pattern 0 CONFIGURE_DEPENDS)
endif()

file(GLOB yaml-cpp-contrib-sources ${contrib-pattern})
file(GLOB yaml-cpp-sources ${src-pattern})

set(msvc-rt $<TARGET_PROPERTY:MSVC_RUNTIME_LIBRARY>)

set(msvc-rt-mtd-static $<STREQUAL:${msvc-rt},MultiThreadedDebug>)
set(msvc-rt-mt-static $<STREQUAL:${msvc-rt},MultiThreaded>)

set(msvc-rt-mtd-dll $<STREQUAL:${msvc-rt},MultiThreadedDebugDLL>)
set(msvc-rt-mt-dll $<STREQUAL:${msvc-rt},MultiThreadedDLL>)

set(backport-msvc-runtime $<VERSION_LESS:${CMAKE_VERSION},3.15>)

add_library(yaml-cpp ${yaml-cpp-type} "")
add_library(yaml-cpp::yaml-cpp ALIAS yaml-cpp)

set_property(TARGET yaml-cpp
  PROPERTY
    MSVC_RUNTIME_LIBRARY ${CMAKE_MSVC_RUNTIME_LIBRARY})
set_property(TARGET yaml-cpp
  PROPERTY
    CXX_STANDARD_REQUIRED ON)

if (NOT YAML_BUILD_SHARED_LIBS)
      set_property(TARGET yaml-cpp PROPERTY POSITION_INDEPENDENT_CODE ${YAML_ENABLE_PIC})
endif()

target_include_directories(yaml-cpp
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>)

if (NOT DEFINED CMAKE_CXX_STANDARD)
  set_target_properties(yaml-cpp
    PROPERTIES
      CXX_STANDARD 11)
endif()

if(YAML_CPP_MAIN_PROJECT)
  target_compile_options(yaml-cpp
    PRIVATE
      $<${not-msvc}:-Wall -Wextra -Wshadow -Weffc++ -Wno-long-long>
      $<${not-msvc}:-pedantic -pedantic-errors>)
endif()

target_compile_options(yaml-cpp
  PRIVATE
    $<$<AND:${backport-msvc-runtime},${msvc-rt-mtd-static}>:-MTd>
    $<$<AND:${backport-msvc-runtime},${msvc-rt-mt-static}>:-MT>
    $<$<AND:${backport-msvc-runtime},${msvc-rt-mtd-dll}>:-MDd>
    $<$<AND:${backport-msvc-runtime},${msvc-rt-mt-dll}>:-MD>

    # /wd4127 = disable warning C4127 "conditional expression is constant"
    # http://msdn.microsoft.com/en-us/library/6t66728h.aspx
    # /wd4355 = disable warning C4355 "'this' : used in base member initializer list
    # http://msdn.microsoft.com/en-us/library/3c594ae3.aspx
    $<$<CXX_COMPILER_ID:MSVC>:/W3 /wd4127 /wd4355>)

target_compile_definitions(yaml-cpp
  PUBLIC
    $<$<NOT:$<BOOL:${YAML_BUILD_SHARED_LIBS}>>:YAML_CPP_STATIC_DEFINE>
  PRIVATE
    $<${build-windows-dll}:${PROJECT_NAME}_DLL>
    $<$<NOT:$<BOOL:${YAML_CPP_BUILD_CONTRIB}>>:YAML_CPP_NO_CONTRIB>)

target_sources(yaml-cpp
  PRIVATE
    $<$<BOOL:${YAML_CPP_BUILD_CONTRIB}>:${yaml-cpp-contrib-sources}>
    ${yaml-cpp-sources})

if (NOT DEFINED CMAKE_DEBUG_POSTFIX)
  set(CMAKE_DEBUG_POSTFIX "d")
endif()

set_target_properties(yaml-cpp PROPERTIES
  VERSION "${PROJECT_VERSION}"
  SOVERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
  PROJECT_LABEL "yaml-cpp ${yaml-cpp-label-postfix}"
  DEBUG_POSTFIX "${CMAKE_DEBUG_POSTFIX}")

set(EXPORT_TARGETS yaml-cpp::yaml-cpp)
configure_package_config_file(
  "${PROJECT_SOURCE_DIR}/yaml-cpp-config.cmake.in"
  "${PROJECT_BINARY_DIR}/yaml-cpp-config.cmake"
  INSTALL_DESTINATION "${YAML_CPP_INSTALL_CMAKEDIR}"
  PATH_VARS CMAKE_INSTALL_INCLUDEDIR CMAKE_INSTALL_LIBDIR)
unset(EXPORT_TARGETS)

write_basic_package_version_file(
  "${PROJECT_BINARY_DIR}/yaml-cpp-config-version.cmake"
  COMPATIBILITY AnyNewerVersion)

configure_file(yaml-cpp.pc.in yaml-cpp.pc @ONLY)

if (YAML_CPP_INSTALL)
  install(TARGETS yaml-cpp
    EXPORT yaml-cpp-targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
  install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
                FILES_MATCHING PATTERN "*.h")
  install(EXPORT yaml-cpp-targets
    NAMESPACE yaml-cpp::
    DESTINATION "${YAML_CPP_INSTALL_CMAKEDIR}")
  install(FILES
      "${PROJECT_BINARY_DIR}/yaml-cpp-config.cmake"
      "${PROJECT_BINARY_DIR}/yaml-cpp-config-version.cmake"
    DESTINATION "${YAML_CPP_INSTALL_CMAKEDIR}")
  install(FILES "${PROJECT_BINARY_DIR}/yaml-cpp.pc"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endif()

if (YAML_CPP_FORMAT_SOURCE AND YAML_CPP_CLANG_FORMAT_EXE)
  add_custom_target(format
    COMMAND clang-format --style=file -i $<TARGET_PROPERTY:yaml-cpp,SOURCES>
    COMMAND_EXPAND_LISTS
    COMMENT "Running clang-format"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    VERBATIM)
endif()

# uninstall target
if(YAML_CPP_INSTALL AND NOT YAML_CPP_DISABLE_UNINSTALL AND NOT TARGET uninstall)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

  add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()
