idf_component_register(
    SRCS
        main.c
        effects.c
        flags.c
        color.c
        wifi_ota.c
    INCLUDE_DIRS
        .
    PRIV_REQUIRES
        fatfs
        nvs_flash
        badge-bsp
        wpa_supplicant
        esp_https_ota
        app_update
        esp_bootloader_format
        custom-certificates
        wifi-manager
        esp_timer
        ndef
)

fatfs_create_spiflash_image(locfd ../fat FLASH_IN_PROJECT)
