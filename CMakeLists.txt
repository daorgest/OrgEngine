cmake_minimum_required(VERSION 3.28)

# Project setup
project(OrgEngine VERSION 0.1)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Renderer selection options (only one should be TRUE)
option(USE_DIRECTX11 "Use DirectX 11 Renderer" OFF)
option(USE_VULKAN "Use Vulkan Renderer" ON)
option(USE_OPENGL "Use OpenGL Renderer" OFF)
option(ASAN_ENABLED "Build this target with AddressSanitizer" ON)

# Ensure only one renderer is enabled
if((USE_VULKAN AND USE_DIRECTX11) OR (USE_VULKAN AND USE_OPENGL) OR (USE_DIRECTX11 AND USE_OPENGL))
  message(FATAL_ERROR "Only one renderer can be enabled at a time. Please disable other renderers.")
elseif(NOT (USE_VULKAN OR USE_DIRECTX11 OR USE_OPENGL))
  message(FATAL_ERROR "At least one renderer must be enabled.")
endif()

# Define executable
add_executable(OrgEngine
        src/Renderer/Camera.cpp
)

# Platform and renderer-specific definitions
target_compile_definitions(OrgEngine PRIVATE UNICODE _UNICODE NOMINMAX)

if(USE_VULKAN)
  target_compile_definitions(OrgEngine PRIVATE VULKAN_BUILD VK_USE_PLATFORM_WIN32_KHR)
elseif(USE_DIRECTX11)
  target_compile_definitions(OrgEngine PRIVATE DIRECTX11_BUILD)
elseif(USE_OPENGL)
  target_compile_definitions(OrgEngine PRIVATE OPENGL_BUILD)
endif()

if(MSVC)
  # Release flags
  target_compile_options(OrgEngine PRIVATE
          $<$<CONFIG:Release>:/O2 /DNDEBUG /Ot>
  )

  # Debug flags
  target_compile_options(OrgEngine PRIVATE
          $<$<CONFIG:Debug>:/Od /DEBUG /Zi /MP>
  )

  # Enable multi-processor compilation for all builds
  target_compile_options(OrgEngine PRIVATE /MP)
endif()

# Source files
file(GLOB_RECURSE GENERAL_SOURCE_FILES
        "${CMAKE_SOURCE_DIR}/src/Platform/*.*"
        "${CMAKE_SOURCE_DIR}/src/Game/*.*"
        "${CMAKE_SOURCE_DIR}/src/Core/*.*"
        "${CMAKE_SOURCE_DIR}/src/Renderer/ResourceLoader.*"
)
target_sources(OrgEngine PRIVATE ${GENERAL_SOURCE_FILES})

# Renderer-specific source files and definitions
if(USE_VULKAN)
  file(GLOB_RECURSE RENDERER_SOURCES "${CMAKE_SOURCE_DIR}/src/Renderer/Vulkan/*.*")
  target_compile_definitions(OrgEngine PRIVATE VULKAN_BUILD VK_USE_PLATFORM_WIN32_KHR)
elseif(USE_OPENGL)
  file(GLOB_RECURSE RENDERER_SOURCES "${CMAKE_SOURCE_DIR}/src/Renderer/OpenGL/*.*")
  target_compile_definitions(OrgEngine PRIVATE OPENGL_BUILD)
elseif(USE_DIRECTX11)
  file(GLOB_RECURSE RENDERER_SOURCES "${CMAKE_SOURCE_DIR}/src/Renderer/DirectX11/*.*")
  target_compile_definitions(OrgEngine PRIVATE DIRECTX11_BUILD)
endif()
target_sources(OrgEngine PRIVATE ${RENDERER_SOURCES})

# Organize source files into filters/groups for IDEs
function(assign_source_groups)
  foreach(SOURCE IN LISTS ARGN)
    get_filename_component(SOURCE_PATH "${SOURCE}" PATH)
    file(RELATIVE_PATH SOURCE_PATH_REL "${CMAKE_SOURCE_DIR}" "${SOURCE_PATH}")
    string(REPLACE "/" "\\" GROUP_PATH "${SOURCE_PATH_REL}")
    source_group("${GROUP_PATH}" FILES "${SOURCE}")
  endforeach()
endfunction()

assign_source_groups(${GENERAL_SOURCE_FILES})
assign_source_groups(${RENDERER_SOURCES})

# Find and link Vulkan if enabled
if(USE_VULKAN)
  find_package(Vulkan REQUIRED)
  target_include_directories(OrgEngine PRIVATE ${Vulkan_INCLUDE_DIRS})
  target_link_libraries(OrgEngine PRIVATE Vulkan::Vulkan)
endif()

# Tracy options
option(TRACY_ENABLE "Enable Tracy Profiler" ON)
option(TRACY_ON_DEMAND "Enable Tracy On-Demand Mode" ON)

if(CMAKE_BUILD_TYPE MATCHES Debug)
  add_definitions(-DTRACY_ENABLE)
endif()

# Include third-party libraries
add_subdirectory(libs/3rdParty/tracy)
add_subdirectory(libs/3rdParty/fmt)
add_subdirectory(libs/3rdParty/glm)
add_subdirectory(libs/3rdParty/fastgltf)
add_subdirectory(libs/3rdParty/fmod)
if(USE_VULKAN)
  add_subdirectory(libs/3rdParty/vma)
  add_subdirectory(libs/3rdParty/vk-bootstrap)
  target_link_libraries(OrgEngine PRIVATE GPUOpen::VulkanMemoryAllocator vk-bootstrap)
endif()

# ImGui integration
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/libs/3rdParty/imgui")
set(IMGUI_SOURCES
        "${IMGUI_DIR}/imgui.cpp"
        "${IMGUI_DIR}/imgui_draw.cpp"
        "${IMGUI_DIR}/imgui_demo.cpp"
        "${IMGUI_DIR}/imgui_tables.cpp"
        "${IMGUI_DIR}/imgui_widgets.cpp"
        "${IMGUI_DIR}/backends/imgui_impl_win32.cpp"
)

# Add ImGui backend based on the renderer
if(USE_VULKAN)
  list(APPEND IMGUI_SOURCES "${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp")
elseif(USE_OPENGL)
  list(APPEND IMGUI_SOURCES "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp")
endif()

# Include ImGui directories and sources
target_include_directories(OrgEngine PRIVATE "${IMGUI_DIR}" "libs/3rdParty/stb/")
target_sources(OrgEngine PRIVATE ${IMGUI_SOURCES})
assign_source_groups(${IMGUI_SOURCES})

# OpenGL configuration
if(USE_OPENGL)
  # Add SDL3
  add_subdirectory(libs/3rdParty/sdl3 EXCLUDE_FROM_ALL)
  if(WIN32)
    add_custom_command(
            TARGET OrgEngine POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy $<TARGET_FILE:SDL3::SDL3-shared> $<TARGET_FILE_DIR:OrgEngine>
            VERBATIM
    )
  endif()

  # Find and add GLAD sources
  file(GLOB GLAD_SOURCES "${CMAKE_SOURCE_DIR}/libs/3rdParty/glad/src/glad.c")
  target_sources(OrgEngine PRIVATE ${GLAD_SOURCES})
  assign_source_groups(${GLAD_SOURCES})
  # Include directories for GLAD
  target_include_directories(OrgEngine PRIVATE
          "${CMAKE_SOURCE_DIR}/libs/3rdParty/glad/include"
  )
  # Link OpenGL and SDL libraries
  target_link_libraries(OrgEngine PRIVATE SDL3::SDL3)
endif()


# Link additional libraries
target_link_libraries(OrgEngine PRIVATE
        Tracy::TracyClient
        fmt::fmt
        glm::glm-header-only
        fastgltf
        fmod
)

# Compile definitions for Debug configuration
target_compile_definitions(OrgEngine PRIVATE
        $<$<CONFIG:Debug>:DEBUG>
)

# Function to add shaders
function(add_shaders TARGET SHADER_DIR)
  file(GLOB_RECURSE SHADER_FILES
          "${SHADER_DIR}/*.comp"
          "${SHADER_DIR}/*.frag"
          "${SHADER_DIR}/*.vert"
  )
  set(SHADER_OUTPUTS)
  foreach(SHADER_FILE IN LISTS SHADER_FILES)
    get_filename_component(SHADER_NAME ${SHADER_FILE} NAME)
    set(SHADER_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/App/Shaders/${SHADER_NAME}.spv")
    list(APPEND SHADER_OUTPUTS ${SHADER_OUTPUT})
    add_custom_command(
            OUTPUT ${SHADER_OUTPUT}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/App/Shaders"
            COMMAND glslc ${SHADER_FILE} -o ${SHADER_OUTPUT}
            DEPENDS ${SHADER_FILE}
            COMMENT "Compiling ${SHADER_NAME} to ${SHADER_OUTPUT}"
    )
  endforeach()
  add_custom_target(${TARGET}_Shaders ALL DEPENDS ${SHADER_OUTPUTS})
  add_dependencies(${TARGET} ${TARGET}_Shaders)
  # Assign shaders to source groups
  assign_source_groups(${SHADER_FILES})
endfunction()
add_shaders(OrgEngine "${CMAKE_SOURCE_DIR}/src/Shaders")

# Function to copy DLLs and create Audio folder
function(copy_dlls_and_create_audio_folder TARGET)
  if(WIN32)
    set(DLL_LIST
            "${CMAKE_SOURCE_DIR}/libs/3rdParty/fmod/win/core/lib/x64/fmod.dll"
    )
    foreach(DLL_PATH IN LISTS DLL_LIST)
      get_filename_component(DLL_NAME ${DLL_PATH} NAME)
      add_custom_command(
              TARGET ${TARGET} POST_BUILD
              COMMAND ${CMAKE_COMMAND} -E copy_if_different
              "${DLL_PATH}"
              "$<TARGET_FILE_DIR:${TARGET}>"
              COMMENT "Copying ${DLL_NAME} to application directory"
      )
    endforeach()
    # Create Audio folder
    add_custom_command(
            TARGET ${TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${TARGET}>/Audio"
            COMMENT "Creating Audio folder in application directory"
    )
  endif()
endfunction()
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
  # Assign assets to source groups (optional)
  assign_source_groups(${ASSET_FILES})
endfunction()
# Copy Models to App/Models
copy_assets(OrgEngine "${CMAKE_SOURCE_DIR}/src/Assets/Models" "${CMAKE_CURRENT_BINARY_DIR}/App/Models")

# Create an empty Textures folder in the App directory
add_custom_command(
        TARGET OrgEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/App/Textures"
        COMMENT "Creating empty Textures folder in application directory"
)

# Set output directories for executable
set_target_properties(OrgEngine PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/App"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_BINARY_DIR}/App"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_BINARY_DIR}/App"
)

# Set Visual Studio working directory
if(MSVC)
  set_target_properties(OrgEngine PROPERTIES
          VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/App"
  )
endif()
