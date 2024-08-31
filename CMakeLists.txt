cmake_minimum_required(VERSION 3.28)

# Project setup
project(OrgEngine VERSION 0.1)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options for different renderers
option(USE_DIRECTX11 "Use DirectX 11 Renderer" FALSE)
option(USE_VULKAN "Use Vulkan Renderer" TRUE)

# Definitions for Vulkan build and Windows platform
add_definitions(-DVULKAN_BUILD -DUNICODE -D_UNICODE -DNOMINMAX)
add_compile_definitions(VK_USE_PLATFORM_WIN32_KHR)

# MSVC-specific configurations
if(MSVC)
  # Release Flags
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /DNDEBUG /Ot /GT /LTCG /arch:AVX2")

  # Debug flags
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /DEBUG")
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<CONFIG:Debug>,EditAndContinue,ProgramDatabase>")
endif()

# Source files globbing
file(GLOB_RECURSE SOURCE_FILES "src/*.cpp")

# Add executable
add_executable(OrgEngine ${SOURCE_FILES})

# Find Vulkan package
find_package(Vulkan REQUIRED)

option ( TRACY_ENABLE "" ON )
option ( TRACY_ON_DEMAND "" ON )

# Include third-party libraries
add_subdirectory(libs/3rdParty/tracy)
add_subdirectory(libs/3rdParty/fmt)
add_subdirectory(libs/3rdParty/glm)
add_subdirectory(libs/3rdParty/vk-bootstrap)
add_subdirectory(libs/3rdParty/vma)
add_subdirectory(libs/3rdParty/fastgltf)
add_subdirectory(libs/3rdParty/fmod)

# ImGui integration
set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/3rdParty/imgui")
file(GLOB IMGUI_SOURCES
        "${IMGUI_DIR}/imgui.cpp"
        "${IMGUI_DIR}/imgui_draw.cpp"
        "${IMGUI_DIR}/imgui_demo.cpp"
        "${IMGUI_DIR}/imgui_tables.cpp"
        "${IMGUI_DIR}/imgui_widgets.cpp"
        "${IMGUI_DIR}/backends/imgui_impl_win32.cpp"
        "${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp"
)

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
        xinput
        Tracy::TracyClient
        Vulkan::Vulkan
        fmt::fmt
        vk-bootstrap::vk-bootstrap
        GPUOpen::VulkanMemoryAllocator
        glm::glm-header-only
        fastgltf
        fmod
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
    get_filename_component(SHADER_NAME ${SHADER_FILE} NAME)
    set(SHADER_OUTPUT "${CMAKE_BINARY_DIR}/App/shaders/${SHADER_NAME}.spv")

    add_custom_command(
            OUTPUT ${SHADER_OUTPUT}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/App/shaders"
            COMMAND glslc ${SHADER_FILE} -o ${SHADER_OUTPUT}
            DEPENDS ${SHADER_FILE}
            COMMENT "Compiling ${SHADER_NAME} to ${SHADER_OUTPUT}"
    )
    list(APPEND SHADER_OUTPUTS ${SHADER_OUTPUT})
  endforeach()

  add_custom_target(${TARGET}_Shaders ALL DEPENDS ${SHADER_OUTPUTS})
  add_dependencies(${TARGET} ${TARGET}_Shaders)
endfunction()

# Add shader compilation for shaders in 'src/Shaders' directory
add_shaders(OrgEngine "${CMAKE_CURRENT_SOURCE_DIR}/src/Shaders")

# Function to copy necessary runtime libraries and create an empty Audio folder
function(copy_dlls_and_create_audio_folder TARGET)
  if(WIN32)
    set(DLL_LIST
            "${CMAKE_CURRENT_SOURCE_DIR}/libs/3rdParty/fmod/win/core/lib/x64/fmod.dll"
    )
    foreach(DLL_PATH IN LISTS DLL_LIST)
      get_filename_component(DLL_NAME ${DLL_PATH} NAME)
      add_custom_command(
              TARGET ${TARGET} POST_BUILD
              COMMAND ${CMAKE_COMMAND} -E copy_if_different
              ${DLL_PATH}
              $<TARGET_FILE_DIR:${TARGET}>
              COMMENT "Copying ${DLL_NAME} to application directory"
      )
    endforeach()

    # Create an empty Audio folder in the App directory
    add_custom_command(
            TARGET ${TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${TARGET}>/Audio"
            COMMENT "Creating Audio folder in application directory"
    )
  endif()
endfunction()

# Use the function for the OrgEngine executable
copy_dlls_and_create_audio_folder(OrgEngine)

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
#
## Packaging for Release configuration
#if(CMAKE_BUILD_TYPE STREQUAL "Release")
#  include(InstallRequiredSystemLibraries)
#  set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
#  set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
#  set(CPACK_GENERATOR "TGZ")
#  set(CPACK_SOURCE_GENERATOR "TGZ")
#  include(CPack)
#endif()
