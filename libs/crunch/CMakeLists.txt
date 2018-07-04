add_library(crnlib
        crn_decomp.h
        crnlib.h
        crn_rgba.h
        crn_rgba.cpp
)
set_target_properties(crnlib PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(crnlib PRIVATE ddslib)
target_compile_options(crnlib PRIVATE -fexceptions)
