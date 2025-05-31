idf_component_register(
    SRCS "sensors.c" "aht20.c" "bmp280.c"
    INCLUDE_DIRS "." "include"
    REQUIRES driver
)