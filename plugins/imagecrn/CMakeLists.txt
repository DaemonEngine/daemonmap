radiant_plugin(imagecrn
        imagecrn.cpp
        crn.cpp crn.h
        )

target_include_directories(imagecrn PRIVATE ${CMAKE_SOURCE_DIR}/libs/crnrgba)
target_link_libraries(imagecrn PRIVATE crnrgba)
