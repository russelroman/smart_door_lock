set(srcs "main.c")

idf_component_register(SRCS "wifi.c" "${srcs}"
                       INCLUDE_DIRS ".")
