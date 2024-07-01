cmake_minimum_required(VERSION 3.28)

project("OrgEngine")

# Set the MSVC debug information format for supporting Edit and Continue
if(MSVC)
  # Release Flags
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /DNDEBUG")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Ot /GT /arch:AVX2") # WILL REGRET THIS LATER

  # Debug flags
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /ZI /DEBUG")
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<CONFIG:Debug,RelWithDebInfo>,EditAndContinue,ProgramDatabase>")
elseif(MINGW)
  # Set MinGW specific flags
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wa,-mbig-obj")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Og -g3")
endif()

# Ensure we are using wide characters on Windows universally
add_definitions(-DUNICODE -D_UNICODE)

# Set C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Globbing source files
file(GLOB_RECURSE SOURCE_FILES "src/*.cpp")

# Add source to this project's executable
add_executable(OrgEngine WIN32 ${SOURCE_FILES})

# Find Vulkan package
find_package(Vulkan REQUIRED)

add_subdirectory(libs/3rdParty/fmt)
add_subdirectory(libs/3rdParty/vk-bootstrap)
set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/imgui")
file(GLOB IMGUI_SOURCES
        "${IMGUI_DIR}/*.cpp"
        "${IMGUI_DIR}/*.h"
        "${IMGUI_DIR}/backends/imgui_impl_win32.cpp"
        "${IMGUI_DIR}/backends/imgui_impl_win32.h"
        "${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp"
        "${IMGUI_DIR}/backends/imgui_impl_vulkan.h")

# Include Vulkan headers
target_include_directories(OrgEngine PRIVATE
        ${Vulkan_INCLUDE_DIRS}
        "${CMAKE_SOURCE_DIR}/3rdParty/vma/"
        "${IMGUI_DIR}"
)

# Add ImGui sources to the executable
target_sources(OrgEngine PRIVATE ${IMGUI_SOURCES})

# Link
target_link_libraries(OrgEngine PRIVATE
        ${Vulkan_LIBRARIES}
        fmt::fmt
        vk-bootstrap::vk-bootstrap
)

target_compile_definitions(OrgEngine PRIVATE
        $<$<CONFIG:Debug>:DEBUG>
)

# Custom function to compile GLSL shaders
function(add_shaders TARGET SHADER_DIR)
  file(GLOB_RECURSE SHADER_FILES "${SHADER_DIR}/*.comp")

  foreach(SHADER_FILE ${SHADER_FILES})
    get_filename_component(SHADER_NAME ${SHADER_FILE} NAME)
    set(SHADER_OUTPUT "${CMAKE_BINARY_DIR}/${SHADER_NAME}.spv")

    add_custom_command(
            OUTPUT ${SHADER_OUTPUT}
            COMMAND glslc ${SHADER_FILE} -o ${SHADER_OUTPUT}
            DEPENDS ${SHADER_FILE}
            COMMENT "Compiling ${SHADER_NAME} to ${SHADER_OUTPUT}"
    )

    target_sources(${TARGET} PRIVATE ${SHADER_OUTPUT})
  endforeach()
endfunction()

# Add shader compilation for shaders in 'src/Shaders' directory
add_shaders(OrgEngine "${CMAKE_CURRENT_SOURCE_DIR}/src/Shaders")

# Create a target for compiling shaders
add_custom_target(Shaders DEPENDS ${SHADER_OUTPUT})

# Add Shaders target as a dependency of OrgEngine
add_dependencies(OrgEngine Shaders)

# TODO: Add tests and install targets if needed.
