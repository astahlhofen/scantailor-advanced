set(ui_files
    ApplyDialog.ui
    OptionsWidget.ui)

set(sources
    ImageView.cpp ImageView.h
    Filter.cpp Filter.h
    OptionsWidget.cpp OptionsWidget.h
    ApplyDialog.cpp ApplyDialog.h
    Settings.cpp Settings.h
    Task.cpp Task.h
    CacheDrivenTask.cpp CacheDrivenTask.h)

qt5_wrap_ui(sources ${ui_files})

add_library(fix_orientation STATIC ${sources} ${ui_files})
target_link_libraries(fix_orientation PUBLIC core)
target_include_directories(fix_orientation PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

translation_sources(scantailor ${sources} ${ui_files})