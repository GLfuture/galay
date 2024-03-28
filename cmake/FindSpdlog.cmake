find_path(SPDLOG_INCLUDE_DIR spdlog/spdlog.h)
find_library(SPDLOG_LIBRARY spdlog)

if(SPDLOG_INCLUDE_DIR AND SPDLOG_LIBRARY)
    set(SPDLOG_FOUND TRUE)
endif()

if(SPDLOG_FOUND)
    if(NOT TARGET spdlog::spdlog)
        add_library(spdlog::spdlog UNKNOWN IMPORTED)
        set_target_properties(spdlog::spdlog PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${SPDLOG_INCLUDE_DIR}"
            IMPORTED_LOCATION "${SPDLOG_LIBRARY}"
        )
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(spdlog DEFAULT_MSG SPDLOG_INCLUDE_DIR SPDLOG_LIBRARY)

if(NOT TARGET spdlog::spdlog_header_only)
    add_library(spdlog::spdlog_header_only INTERFACE IMPORTED)
endif()