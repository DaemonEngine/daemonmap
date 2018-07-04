radiant_plugin(image
        bmp.cpp bmp.h
        dds.cpp dds.h
        image.cpp
        jpeg.cpp jpeg.h
        ktx.cpp ktx.h
        pcx.cpp pcx.h
        tga.cpp tga.h
        crn.cpp crn.h
        )

find_package(JPEG REQUIRED)
target_include_directories(image PRIVATE ${JPEG_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/libs/crunch)
target_link_libraries(image PRIVATE ddslib etclib crnlib ${JPEG_LIBRARIES})
target_compile_options(image PRIVATE -fexceptions)
