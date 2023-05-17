set(srcs
        "ble_mesh.c")

idf_component_register(SRCS "main.cpp" "${srcs}"
                    INCLUDE_DIRS  ".")
