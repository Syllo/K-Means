add_executable(kmeans k-means.c main.c k-means_png.c)
target_include_directories(kmeans PRIVATE ${PROJECT_SOURCE_DIR}/include)
set_property(TARGET kmeans
  PROPERTY C_STANDARD 11)
target_link_libraries(kmeans PRIVATE m)

#find_package(PNG) # Embed the version for better portability
if (NOT PNG_FOUND)
  message(STATUS "Fetching libPNG ...")
  include(FetchContent)
  FetchContent_Declare(
    libPNG
    GIT_REPOSITORY https://github.com/glennrp/libpng.git
    GIT_TAG origin/libpng16
    GIT_SHALLOW ON
    SOURCE_DIR ${CMAKE_BINARY_DIR}/libpng
    )
  FetchContent_GetProperties(libPNG)
  if (NOT libpng_POPULATED)
    FetchContent_Populate(libPNG)
    set(PNG_SHARED OFF CACHE BOOL "No shared libpng")
    set(PNG_TESTS OFF CACHE BOOL "No libpng tests")
    set(PNG_STATIC ON CACHE BOOL "Build static libpng")
    add_subdirectory(${libpng_SOURCE_DIR} ${libpng_BINARY_DIR})
    unset(PNG_SHARED)
    unset(PNG_STATIC)
    unset(PNG_TESTS)
  endif()
  if (NOT TARGET png_static)
    message(FATAL_ERROR "K-Means require libPNG but the build failed")
  else()
    target_link_libraries(kmeans PRIVATE png_static)
  endif()
else()
  add_library(png INTERFACE IMPORTED)
  set_property(TARGET png PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${PNG_INCLUDE_DIRS})
  set_property(TARGET png PROPERTY INTERFACE_COMPILE_DEFINITIONS ${PNG_DEFINITIONS})
  set_property(TARGET png PROPERTY INTERFACE_LINK_LIBRARIES ${PNG_LIBRARIES})
  target_link_libraries(kmeans PRIVATE png)
endif()

# Compile Options
include(compile-flags-helpers)
include(${PROJECT_SOURCE_DIR}/optimization_flags.cmake)

if (DEFINED ADDITIONAL_BENCHMARK_COMPILE_OPTIONS)
  add_compiler_option_to_target_type(kmeans Benchmark PRIVATE ${ADDITIONAL_BENCHMARK_COMPILE_OPTIONS})
endif()

foreach(compile_type IN ITEMS Release RelWithDebInfo)
  add_compiler_option_to_target_type(kmeans ${compile_type} PRIVATE ${ADDITIONAL_RELEASE_COMPILE_OPTIONS})
  add_linker_option_to_target_type(kmeans ${compile_type} PRIVATE ${ADDITIONAL_RELEASE_LINK_OPTIONS})
endforeach()

add_compiler_option_to_target_type(kmeans Debug PRIVATE ${ADDITIONAL_DEBUG_COMPILE_OPTIONS})

# Linker Options

if (DEFINED ADDITIONAL_BENCHMARK_LINK_OPTIONS)
  add_linker_option_to_target_type(kmeans Benchmark PRIVATE ${ADDITIONAL_BENCHMARK_LINK_OPTIONS})
endif()

add_sanitizers_to_target(kmeans Debug PRIVATE address undefined)

include(CheckIPOSupported)
check_ipo_supported(RESULT result)
if((result) AND USE_IPO)
  set_property(TARGET kmeans PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

install(TARGETS kmeans RUNTIME DESTINATION bin)
