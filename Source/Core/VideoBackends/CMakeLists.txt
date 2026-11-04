add_subdirectory(OGL)
add_subdirectory(Null)
if(NOT DOLPHIN_BIG_ENDIAN)
  # The software renderer packs pixels as little-endian words.
  add_subdirectory(Software)
endif()

if(WIN32)
  add_subdirectory(D3DCommon)
  add_subdirectory(D3D)
  add_subdirectory(D3D12)
endif()

if(APPLE)
  add_subdirectory(Metal)
endif()

if(ENABLE_VULKAN)
  add_subdirectory(Vulkan)
endif()
