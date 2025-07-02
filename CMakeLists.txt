cmake_minimum_required(VERSION 3.18)

#///////////////////////////////////////////////////////////////////#
#                              PROJECT                              #
#///////////////////////////////////////////////////////////////////#

project(nvtop VERSION 3.2.0
  LANGUAGES C CXX)

set(default_build_type "Release")
# Default build type
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
    STRING "Choose the type of build." FORCE)
endif()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
  "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#///////////////////////////////////////////////////////////////////#
#                           DEPENDENCIES                            #
#///////////////////////////////////////////////////////////////////#

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Prefer static libraries over shared libraries
set(CMAKE_FIND_LIBRARY_SUFFIXES .a .so)

set(CURSES_NEED_NCURSES TRUE)
# Try to find ncurses with unicode support first
set(CURSES_NEED_WIDE TRUE)
find_package(Curses QUIET)
if (NOT CURSE_FOUND)
  # Fallback to regular ncurses library, which may also support unicode!
  set(CURSES_NEED_WIDE FALSE)
  find_package(Curses REQUIRED)
endif()

# Find static versions of required libraries
find_library(LIBDRM_STATIC
  NAMES libdrm.a drm
  PATHS /usr/lib /usr/lib64 /usr/local/lib /usr/local/lib64
  HINTS ${CMAKE_INSTALL_PREFIX}/lib ${CMAKE_INSTALL_PREFIX}/lib64
  PATH_SUFFIXES x86_64-linux-gnu i386-linux-gnu
  NO_DEFAULT_PATH
)

if(NOT LIBDRM_STATIC)
  find_library(LIBDRM_STATIC
    NAMES libdrm.a drm
  )
endif()

find_library(LIBSYSTEMD_STATIC
  NAMES libsystemd.a systemd
  PATHS /usr/lib /usr/lib64 /usr/local/lib /usr/local/lib64
  HINTS ${CMAKE_INSTALL_PREFIX}/lib ${CMAKE_INSTALL_PREFIX}/lib64
  PATH_SUFFIXES x86_64-linux-gnu i386-linux-gnu
  NO_DEFAULT_PATH
)

if(NOT LIBSYSTEMD_STATIC)
  find_library(LIBSYSTEMD_STATIC
    NAMES libsystemd.a systemd
  )
endif()

# Find static ncurses library
find_library(LIBNCURSES_STATIC
  NAMES libncurses.a libncursesw.a ncurses ncursesw
  PATHS /usr/lib /usr/lib64 /usr/local/lib /usr/local/lib64
  HINTS ${CMAKE_INSTALL_PREFIX}/lib ${CMAKE_INSTALL_PREFIX}/lib64
  PATH_SUFFIXES x86_64-linux-gnu i386-linux-gnu
  NO_DEFAULT_PATH
)

if(NOT LIBNCURSES_STATIC)
  find_library(LIBNCURSES_STATIC
    NAMES libncurses.a libncursesw.a ncurses ncursesw
  )
endif()

# Create static library targets
add_library(ncurses INTERFACE IMPORTED)
set_property(TARGET ncurses PROPERTY
  INTERFACE_INCLUDE_DIRECTORIES ${CURSES_INCLUDE_DIRS})

if(LIBNCURSES_STATIC)
  message(STATUS "Using static ncurses library: ${LIBNCURSES_STATIC}")
  set_property(TARGET ncurses PROPERTY
    INTERFACE_LINK_LIBRARIES ${LIBNCURSES_STATIC})
else()
  message(WARNING "Static ncurses library not found, falling back to dynamic linking")
  set_property(TARGET ncurses PROPERTY
    INTERFACE_LINK_LIBRARIES ${CURSES_LIBRARIES})
endif()

if(LIBDRM_STATIC)
  message(STATUS "Using static libdrm: ${LIBDRM_STATIC}")
  add_library(drm_static INTERFACE IMPORTED)
  set_property(TARGET drm_static PROPERTY
    INTERFACE_LINK_LIBRARIES ${LIBDRM_STATIC})
else()
  message(WARNING "Static libdrm not found")
endif()

if(LIBSYSTEMD_STATIC)
  message(STATUS "Using static libsystemd: ${LIBSYSTEMD_STATIC}")
  add_library(systemd_static INTERFACE IMPORTED)
  set_property(TARGET systemd_static PROPERTY
    INTERFACE_LINK_LIBRARIES ${LIBSYSTEMD_STATIC})
  # libsystemd often requires additional dependencies when statically linked
  find_library(LIBLZMA NAMES lzma)
  find_library(LIBLZ4 NAMES lz4)
  find_library(LIBZSTD NAMES zstd)
  find_library(LIBCAP NAMES cap)
  if(LIBLZMA)
    set_property(TARGET systemd_static PROPERTY
      INTERFACE_LINK_LIBRARIES ${LIBSYSTEMD_STATIC} ${LIBLZMA} ${LIBLZ4} ${LIBZSTD} ${LIBCAP})
  endif()
else()
  message(WARNING "Static libsystemd not found")
endif()

add_compile_definitions(NCURSES_ENABLE_STDBOOL_H=1)

# Force static linking for the final executable
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")

#///////////////////////////////////////////////////////////////////#
#                        COMPILATION OPTIONS                        #
#///////////////////////////////////////////////////////////////////#

# Use full RPATH on build tree
set(CMAKE_SKIP_BUILD_RPATH FALSE)
# Do not build with install RPATH
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
# Set the RPATH when install
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
# Only set RPATH if the installation directory is not a system directory
LIST(FIND
  CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib"
  isSystemDir)
if("${isSystemDir}" STREQUAL "-1")
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
else()
  set(CMAKE_INSTALL_RPATH "")
endif()

if(APPLE)
  set(APPLE_SUPPORT_DEFAULT ON)
  set(NVIDIA_SUPPORT_DEFAULT OFF)
  set(AMDGPU_SUPPORT_DEFAULT OFF)
  set(INTEL_SUPPORT_DEFAULT OFF)
  set(MSM_SUPPORT_DEFAULT OFF)
  set(PANFROST_SUPPORT_DEFAULT OFF)
  set(PANTHOR_SUPPORT_DEFAULT OFF)
  set(ASCEND_SUPPORT_DEFAULT OFF)
elseif(ASCEND_SUPPORT)
  set(APPLE_SUPPORT_DEFAULT OFF)
  set(NVIDIA_SUPPORT_DEFAULT OFF)
  set(AMDGPU_SUPPORT_DEFAULT OFF)
  set(INTEL_SUPPORT_DEFAULT OFF)
  set(MSM_SUPPORT_DEFAULT OFF)
  set(PANFROST_SUPPORT_DEFAULT OFF)
  set(PANTHOR_SUPPORT_DEFAULT OFF)
  set(ASCEND_SUPPORT_DEFAULT ON)
else()
  set(APPLE_SUPPORT_DEFAULT OFF)
  set(NVIDIA_SUPPORT_DEFAULT ON)
  set(AMDGPU_SUPPORT_DEFAULT ON)
  set(INTEL_SUPPORT_DEFAULT ON)
  set(V3D_SUPPORT_DEFAULT ON)
  set(MSM_SUPPORT_DEFAULT ON)
  set(PANFROST_SUPPORT_DEFAULT ON)
  set(PANTHOR_SUPPORT_DEFAULT ON)
  set(ASCEND_SUPPORT_DEFAULT OFF)
endif()

# TPU support is only available on Linux
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  # Check for libtpuinfo.so to set the default for TPU support
  find_library(LIBTPUINFO
    NAMES libtpuinfo.so
    PATHS /usr/lib /usr/lib64 /usr/local/lib /usr/local/lib64
    HINTS ${CMAKE_INSTALL_PREFIX}/lib ${CMAKE_INSTALL_PREFIX}/lib64 lib lib64
  )
  if (NOT LIBTPUINFO)
    set(TPU_SUPPORT_DEFAULT OFF)
  else()
    set(TPU_SUPPORT_DEFAULT ON)
  endif()
else()
  set(TPU_SUPPORT_DEFAULT OFF)
endif()

option(NVIDIA_SUPPORT "Build support for NVIDIA GPUs through libnvml" ${NVIDIA_SUPPORT_DEFAULT})
option(AMDGPU_SUPPORT "Build support for AMD GPUs through amdgpu driver" ${AMDGPU_SUPPORT_DEFAULT})
option(INTEL_SUPPORT "Build support for Intel GPUs through i915 or xe driver" ${INTEL_SUPPORT_DEFAULT})
option(MSM_SUPPORT "Build support for Adreno GPUs through msm driver" ${MSM_SUPPORT_DEFAULT})
option(APPLE_SUPPORT "Build support for Apple GPUs through Metal" ${APPLE_SUPPORT_DEFAULT})
option(PANFROST_SUPPORT "Build support for Mali GPUs through panfrost driver" ${PANFROST_SUPPORT_DEFAULT})
option(PANTHOR_SUPPORT "Build support for Mali GPUs through panthor driver" ${PANTHOR_SUPPORT_DEFAULT})
option(ASCEND_SUPPORT "Build support for Ascend NPUs through Ascend DCMI" ${ASCEND_SUPPORT_DEFAULT})
option(V3D_SUPPORT "Build support for Raspberrypi through v3d" ${V3D_SUPPORT_DEFAULT})
option(TPU_SUPPORT "Build support for Google TPUs through GRPC" ${TPU_SUPPORT_DEFAULT})

add_subdirectory(src)

#///////////////////////////////////////////////////////////////////#
#                             INSTALL                               #
#///////////////////////////////////////////////////////////////////#

string(TIMESTAMP TODAY_MANPAGE "%B %Y")
string(TIMESTAMP TODAY_ISO_8601 "%Y-%m-%d")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/manpage/nvtop.in"
  "${CMAKE_CURRENT_BINARY_DIR}/manpage/nvtop"
  IMMEDIATE @ONLY)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/desktop/nvtop.metainfo.xml.in"
  "${CMAKE_CURRENT_BINARY_DIR}/desktop/io.github.syllo.nvtop.metainfo.xml"
  IMMEDIATE @ONLY)
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/manpage/nvtop"
  DESTINATION share/man/man1/
  PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
  RENAME nvtop.1)
install(FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/desktop/nvtop.svg"
  DESTINATION share/icons/hicolor/scalable/apps
  PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
install(FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/desktop/nvtop.desktop"
  DESTINATION share/applications
  PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/desktop/io.github.syllo.nvtop.metainfo.xml"
  DESTINATION share/metainfo
  PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)
add_custom_target(uninstall
  COMMAND ${CMAKE_COMMAND} -P
  ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

#///////////////////////////////////////////////////////////////////#
#                             TESTING                               #
#///////////////////////////////////////////////////////////////////#


if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
  option(BUILD_TESTING "Build tests" OFF)
endif()
include(CTest)
add_subdirectory(tests)
