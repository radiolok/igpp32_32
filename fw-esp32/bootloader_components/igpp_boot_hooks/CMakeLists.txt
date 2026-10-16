idf_component_register(SRCS "hooks.c")
# Keep the hooks from being discarded by the linker.
target_link_libraries(${COMPONENT_LIB} INTERFACE "-u bootloader_hooks_include")
