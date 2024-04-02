
idf_component_register(
	SRCS
		"ECU_highlevel.c"
	INCLUDE_DIRS
		"include"
	REQUIRES
		driver
		esp_timer
		TLD7002
		nvs_flash
		animations
	PRIV_REQUIRES
		
)