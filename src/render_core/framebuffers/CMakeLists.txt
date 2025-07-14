add_library(vision-framebuffer INTERFACE)
vision_add_plugin(normal CATEGORY framebuffer SOURCES normal.cpp)
vision_add_plugin(lightfield CATEGORY framebuffer SOURCES lightfield.cpp)