file(GLOB_RECURSE SOURCE_FILES *.c*)
vision_add_plugin(surfel CATEGORY radiancecache SOURCES ${SOURCE_FILES})