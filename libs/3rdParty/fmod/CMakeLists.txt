project(fmod)

add_library(fmod SHARED IMPORTED GLOBAL)

if(WIN32)
    set(FMOD_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/win/core/lib/x64")
    set(FMOD_INC_PATH "${CMAKE_CURRENT_SOURCE_DIR}/win/core/inc")
    set(FMOD_IMPLIB "${FMOD_LIB_PATH}/fmod_vc.lib")
    set(FMOD_DLL "${FMOD_LIB_PATH}/fmod.dll")
elseif(APPLE)
    set(FMOD_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/mac/core/lib")
    set(FMOD_INC_PATH "${CMAKE_CURRENT_SOURCE_DIR}/mac/core/inc")
    set(FMOD_DLL "${FMOD_LIB_PATH}/libfmod.dylib")
endif()

set_property(TARGET fmod PROPERTY
        IMPORTED_LOCATION "${FMOD_DLL}")
if(WIN32)
    set_property(TARGET fmod PROPERTY
            IMPORTED_IMPLIB "${FMOD_IMPLIB}")
endif()
set_property(TARGET fmod PROPERTY
        INTERFACE_INCLUDE_DIRECTORIES "${FMOD_INC_PATH}")
