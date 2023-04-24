set(srcs
        "board.c")

idf_component_register(SRCS "main.c" "${srcs}"
                    INCLUDE_DIRS  ".")
