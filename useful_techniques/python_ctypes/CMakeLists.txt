add_library( test SHARED test.cpp )
set_target_properties( test PROPERTIES 
    LIBRARY_OUTPUT_NAME "test" PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} )
