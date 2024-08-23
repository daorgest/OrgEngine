cmake_minimum_required(VERSION 3.28)

# Project setup
project(OrgEngine VERSION 0.1)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options for different renderers
option(USE_DIRECTX11 "Use DirectX 11 Renderer" FALSE)
option(USE_VULKAN "Use Vulkan Renderer" TRUE)
option(USE_SDL3 "Use SDL Window Manager" TRUE)

message(STATUS "USE_DIRECTX11 is set to: ${USE_DIRECTX11}")
message(STATUS "USE_VULKAN is set to: ${USE_VULKAN}")
message(STATUS "USE_SDL is set to: ${USE_SDL3}")

# Definitions for Vulkan build and Windows platform
add_definitions(-DVULKAN_BUILD -DUNICODE -D_UNICODE -DNOMINMAX)
add_compile_definitions(VK_USE_PLATFORM_WIN32_KHR)

# MSVC-specific stuff
if(MSVC)
  # Release Flags
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /DNDEBUG /Ot /GT /arch:AVX2")

  # Debug flags
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /DEBUG")
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<CONFIG:Debug>,EditAndContinue,ProgramDatabase>")

  # AddressSanitizer flags
  set(CMAKE_CXX_FLAGS_ASAN "${CMAKE_CXX_FLAGS_DEBUG} /fsanitize=address")
endif()

# Source files globbing
file(GLOB_RECURSE SOURCE_FILES "src/*.cpp")

# Conditional SDL integration
if(USE_SDL3)
  add_compile_definitions(USE_SDL3)
  add_subdirectory(libs/3rdParty/sdl3)
  target_link_libraries(OrgEngine PRIVATE SDL3::SDL3-static)

  # Remove PlatformWindows.cpp from SOURCE_FILES if SDL is enabled
  list(REMOVE_ITEM SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/PlatformWindows.cpp")

  # Add SDL-specific sources
  list(APPEND SOURCE_FILES
          "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/PlatformSDL.h"
          "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/PlatformSDL.cpp"
  )
else()
  # Add Win32-specific sources if SDL is not used
  list(APPEND SOURCE_FILES
          "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/PlatformWindows.h"
          "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/PlatformWindows.cpp"
  )
endif()

# Add executable
add_executable(OrgEngine ${SOURCE_FILES})

# Find Vulkan package
find_package(Vulkan REQUIRED)

# Include third-party libraries
add_subdirectory(libs/3rdParty/fmt)
add_subdirectory(libs/3rdParty/glm)
add_subdirectory(libs/3rdParty/vk-bootstrap)
add_subdirectory(libs/3rdParty/vma)
add_subdirectory(libs/3rdParty/fastgltf)

set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/3rdParty/imgui")
file(GLOB IMGUI_SOURCES
        "${IMGUI_DIR}/imgui.cpp"
        "${IMGUI_DIR}/imgui_draw.cpp"
        "${IMGUI_DIR}/imgui_demo.cpp"
        "${IMGUI_DIR}/imgui_tables.cpp"
        "${IMGUI_DIR}/imgui_widgets.cpp"
        "${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp"
)

# Choose the appropriate backend based on the window manager
if(USE_SDL3)
  list(APPEND IMGUI_SOURCES "${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp")
else()
  list(APPEND IMGUI_SOURCES "${IMGUI_DIR}/backends/imgui_impl_win32.cpp")
endif()

# Include directories
target_include_directories(OrgEngine PRIVATE
        ${Vulkan_INCLUDE_DIRS}
        "${CMAKE_SOURCE_DIR}/libs/3rdParty/vma/"
        "${IMGUI_DIR}"
)

# Add ImGui sources to the executable
target_sources(OrgEngine PRIVATE ${IMGUI_SOURCES})

# Link libraries
target_link_libraries(OrgEngine PRIVATE
        xinput.lib
        Vulkan::Vulkan
        fmt::fmt
        vk-bootstrap::vk-bootstrap
        GPUOpen::VulkanMemoryAllocator
        glm::glm-header-only
        fastgltf
)

# Compile definitions based on the configuration
target_compile_definitions(OrgEngine PRIVATE
        $<$<CONFIG:Debug>:DEBUG>
)

# Function to compile GLSL shaders
function(add_shaders TARGET SHADER_DIR)
  file(GLOB_RECURSE SHADER_FILES
          "${SHADER_DIR}/*.comp"
          "${SHADER_DIR}/*.frag"
          "${SHADER_DIR}/*.vert"
  )

  set(SHADER_OUTPUTS "")
  foreach(SHADER_FILE ${SHADER_FILES})
    get_filename_component(SHADER_NAME_WE ${SHADER_FILE} NAME_WE)
    set(SHADER_OUTPUT "${CMAKE_BINARY_DIR}/App/shaders/${SHADER_NAME_WE}.spv")

    # Avoid adding duplicate custom commands
    if(NOT SHADER_OUTPUT IN_LIST SHADER_OUTPUTS)
      add_custom_command(
              OUTPUT ${SHADER_OUTPUT}
              COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/App/shaders"
              COMMAND glslc ${SHADER_FILE} -o ${SHADER_OUTPUT}
              DEPENDS ${SHADER_FILE}
              COMMENT "Compiling ${SHADER_NAME_WE} to ${SHADER_OUTPUT}"
      )
      list(APPEND SHADER_OUTPUTS ${SHADER_OUTPUT})
    endif()
  endforeach()

  add_custom_target(${TARGET}_Shaders ALL DEPENDS ${SHADER_OUTPUTS})
  add_dependencies(${TARGET} ${TARGET}_Shaders)
endfunction()


# Add shader compilation for shaders in 'src/Shaders' directory
add_shaders(OrgEngine "${CMAKE_CURRENT_SOURCE_DIR}/src/Shaders")

# Function to copy assets
function(copy_assets TARGET ASSET_DIR OUTPUT_DIR)
  file(GLOB_RECURSE ASSET_FILES "${ASSET_DIR}/*")

  add_custom_command(
          TARGET ${TARGET}
          PRE_BUILD
          COMMAND ${CMAKE_COMMAND} -E make_directory "${OUTPUT_DIR}"
          COMMAND ${CMAKE_COMMAND} -E copy_directory "${ASSET_DIR}" "${OUTPUT_DIR}"
          COMMENT "Copying assets from ${ASSET_DIR} to ${OUTPUT_DIR}"
  )
endfunction()

# Copy assets to the output directory
copy_assets(OrgEngine "${CMAKE_CURRENT_SOURCE_DIR}/src/Assets" "${CMAKE_BINARY_DIR}/App/assets")

# Set the output directories for the executable
set_target_properties(OrgEngine PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/App"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/App"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/App"
)

# Packaging for Release configuration
if(${CMAKE_CXX_FLAGS_RELEASE})
  include(InstallRequiredSystemLibraries)
  set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/License.txt")
  set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
  set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
  set(CPACK_GENERATOR "TGZ")
  set(CPACK_SOURCE_GENERATOR "TGZ")
  include(CPack)
endif()
