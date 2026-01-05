set(SHADERS
    raytracing.slang
)

foreach(source IN LISTS SHADERS)
  get_filename_component(FILENAME ${source} NAME)
  add_custom_command(
    COMMAND
      "C:/VulkanSDK/1.4.335.0/Bin/glslc.exe"
      -o ${CMAKE_CURRENT_BINARY_DIR}/${FILENAME}.spv
      ${source}
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${FILENAME}.spv
    DEPENDS ${source}
    COMMENT "Compiling ${FILENAME}"
  )
  list(APPEND SPV_SHADERS ${CMAKE_CURRENT_BINARY_DIR}/${FILENAME}.spv)
endforeach()
