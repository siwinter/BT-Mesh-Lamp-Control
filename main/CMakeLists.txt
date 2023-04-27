set(srcs
        "board.c")

idf_component_register(SRCS "ble_mesh.c" "main.cpp" "${srcs}"
                    INCLUDE_DIRS  ".")
