# CMakeList.txt : CMake project for OrgEngine, include source and define
# project specific logic here.
#
cmake_minimum_required(VERSION 3.28)

project("OrgEngine")

# Enable Hot Reload for MSVC compilers if supported.
if(POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
endif()

# Set the MSVC debug information format for supporting Edit and Continue
if(MSVC)
  # Release Flags
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O1")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Ot /GT /arch:AVX2") # WILL REGRET THIS LATER

  # Debug flags
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /ZI /DEBUG")
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<CONFIG:Debug,RelWithDebInfo>,EditAndContinue,ProgramDatabase>")
  # Ensure we are using wide characters on Windows
  add_definitions(-DUNICODE -D_UNICODE)
endif()

# Set C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Globbing source files
file(GLOB_RECURSE SOURCE_FILES "src/*.cpp" )

# Add source to this project's executable
add_executable(OrgEngine WIN32 ${SOURCE_FILES}
        src/Core/Logger.h)

# Find Vulkan package
find_package(Vulkan REQUIRED)

add_subdirectory(libs/fmt)

# Include Vulkan headers
target_include_directories(OrgEngine PRIVATE ${Vulkan_INCLUDE_DIRS})

# Link
target_link_libraries(OrgEngine PRIVATE ${Vulkan_LIBRARIES} fmt::fmt)


target_compile_definitions(OrgEngine PRIVATE
        $<$<CONFIG:Debug>:DEBUG>
)

# TODO: Add tests and install targets if needed.
