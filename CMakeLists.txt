# file(GLOB_RECURSE SRCS "src/*.c")

file(GLOB SRCS "src/*.c"
               "src/peripheral/*.c"
               "src/common/*.c")

if(CONFIG_SWITCH_DEVICE)
file(GLOB SWITCH_SRCS "src/switch/*.c")
list(APPEND SRCS ${SWITCH_SRCS})
endif()

if(CONFIG_GCOOL_DEVICE)
file(GLOB GCOOL_SRCS "src/gcool/*.c")
list(APPEND SRCS ${GCOOL_SRCS})
endif()

# Collect all include directories
file(GLOB_RECURSE ALL_HEADERS "inc/*.h")
# Extract directory paths from header list 
set(INCS "")
foreach(_header ${ALL_HEADERS})
    get_filename_component(_dir ${_header} DIRECTORY)
    list(APPEND INCS ${_dir})
endforeach()
list(REMOVE_DUPLICATES INCS)

idf_component_register(SRCS ${SRCS}
                       INCLUDE_DIRS ${INCS}
                       PRIV_REQUIRES freertos log esp_hw_support
                       REQUIRES driver esp_rom esp_event esp_timer)




