cmake_minimum_required(VERSION 3.0)

if (APPLE)
  set(MACOSX_DEPLOYMENT_TARGET 10.9)
endif()

project(modextract)

set(CMAKE_PROJECT_VERSION_MAJOR 1)
set(CMAKE_PROJECT_VERSION_MINOR 0)
set(CMAKE_PROJECT_VERSION_PATCH 0)

if (NOT MSVC AND NOT ANDROID)
  set(BUILD_STATIC_DEFAULT ON)
else()
  set(BUILD_STATIC_DEFAULT OFF)
endif()

include(TestBigEndian)

option(SYSTEM_LIBSNDFILE "Use a system-installed version of libsndfile instead of the vendored one" OFF)
option(BUILD_STATIC "Build a static binary" ${BUILD_STATIC_DEFAULT})

set(DEPENDENCIES_INCLUDE_DIRS "")

set(DEPENDENCIES_DEFINES "")

TEST_BIG_ENDIAN(IS_BIG_ENDIAN)

if (IS_BIG_ENDIAN)
  list(APPEND DEPENDENCIES_DEFINES "TA_BIG_ENDIAN")
endif()

set(DEPENDENCIES_COMPILE_OPTIONS "")
set(DEPENDENCIES_LIBRARIES "")
set(DEPENDENCIES_LIBRARY_DIRS "")
set(DEPENDENCIES_LINK_OPTIONS "")
set(DEPENDENCIES_LEGACY_LDFLAGS "")

if (SYSTEM_LIBSNDFILE)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(LIBSNDFILE REQUIRED sndfile)
  list(APPEND DEPENDENCIES_INCLUDE_DIRS ${LIBSNDFILE_INCLUDE_DIRS})
  list(APPEND DEPENDENCIES_COMPILE_OPTIONS ${LIBSNDFILE_CFLAGS_OTHER})
  list(APPEND DEPENDENCIES_LIBRARIES ${LIBSNDFILE_LIBRARIES})
  list(APPEND DEPENDENCIES_LIBRARY_DIRS ${LIBSNDFILE_LIBRARY_DIRS})
  list(APPEND DEPENDENCIES_LINK_OPTIONS ${LIBSNDFILE_LDFLAGS_OTHER})
  list(APPEND DEPENDENCIES_LEGACY_LDFLAGS ${LIBSNDFILE_LDFLAGS})
  message(STATUS "Using system-installed libsndfile")
else()
  set(BUILD_TESTING OFF CACHE BOOL "aaaaaa" FORCE)
  set(BUILD_PROGRAMS OFF CACHE BOOL "aaa" FORCE)
  set(BUILD_EXAMPLES OFF CACHE BOOL "a" FORCE)
  set(ENABLE_EXTERNAL_LIBS OFF CACHE BOOL "come on" FORCE)
  set(ENABLE_MPEG OFF CACHE BOOL "come on" FORCE)
  add_subdirectory(libsndfile EXCLUDE_FROM_ALL)
  list(APPEND DEPENDENCIES_LIBRARIES sndfile)
  message(STATUS "Using vendored libsndfile")
endif()

add_executable(modextract main.c)

if (BUILD_STATIC)
  list(APPEND DEPENDENCIES_LIBRARIES -static)
endif()

target_include_directories(modextract SYSTEM PRIVATE ${DEPENDENCIES_INCLUDE_DIRS})
target_compile_definitions(modextract PRIVATE ${DEPENDENCIES_DEFINES})
target_compile_options(modextract PRIVATE ${DEPENDENCIES_COMPILE_OPTIONS})
target_link_libraries(modextract PRIVATE ${DEPENDENCIES_LIBRARIES})
if (PKG_CONFIG_FOUND AND SYSTEM_LIBSNDFILE)
  if ("${CMAKE_VERSION}" VERSION_LESS "3.13")
    message(WARNING
      "CMake version is <3.13, using old pkg-config LDFLAGS. "
      "You may encounter linking problems with these!"
    )
    target_link_libraries(modextract PRIVATE ${DEPENDENCIES_LEGACY_LDFLAGS})
  else()
    target_link_directories(modextract PRIVATE ${DEPENDENCIES_LIBRARY_DIRS})
    target_link_options(modextract PRIVATE ${DEPENDENCIES_LINK_OPTIONS})
  endif()
endif()

install(TARGETS modextract RUNTIME DESTINATION bin)
