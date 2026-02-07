if(NOT COMPILER_DEFINE_SET)
    set(COMPILER_DEFINE_SET true)

    #if(WARNINGS_AS_ERRORS)
    #    add_definitions(/WX)
    #endif()
    add_definitions(/MP) # enable multi-processor compiling for all projects by default.
    add_definitions(/bigobj) # enable jumbo sized object files
    add_definitions(/D"WIN32"
                    /D"_X86_"
                    /D"_CRT_SECURE_NO_WARNINGS"
                    /D"_SCL_SECURE_NO_WARNINGS"
                    /D"_CRT_SECURE_NO_DEPRECATE"
                    #/D"NOMINMAX"
                    /D"USE_VS_PRAGMAS"
                    /D"VC_EXTRALEAN"
                    /D"HAVE_STRUCT_TIMESPEC"
                    /D"HAVE_SNPRINTF")
    add_definitions(/wd4221 /wd4005 /wd4996 /wd4351 /wd4748 /wd4267 /wd4800 /wd4403 /vmg)
    #add_definitions(/wd4530)

    #add_definitions(/Zc:ternary) # enable standards conforming behaviour of the ternary (x?y:z) operator.
    # change default from "/MD" or "/MDd" to "/MT" or "/MTd" to force static runtime linking
    foreach(CONFIG "" "_DEBUG" "_RELEASE" "_MINSIZEREL" "_RELWITHDEBINFO")
        foreach(LANG "C" "CXX")
            set(FULL_CONFIG "CMAKE_${LANG}_FLAGS${CONFIG}")
            string(REPLACE "/MD" "/MT" ${FULL_CONFIG} "${${FULL_CONFIG}}")
        endforeach()
    endforeach()

    # extra optimizations for release builds
    foreach(LANG "C" "CXX")
        IF(CMAKE_BUILD_TYPE STREQUAL "Release")
            add_definitions(/Ox /GT /GF Qpar)
            IF(WIN_ENABLE_VECTOR_EXTENSIONS)
                IF(MSVC_64BIT)
                    add_definitions(/arch:AVX2)
                ELSE()
                    add_definitions(/arch:SSE2)
                ENDIF()
            ENDIF()
        ENDIF()
    endforeach()

    string(REPLACE "/O2" "/Od" CMAKE_C_FLAGS_RELWITHDEBINFO   "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
    string(REPLACE "/O2" "/Od" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")


endif()
