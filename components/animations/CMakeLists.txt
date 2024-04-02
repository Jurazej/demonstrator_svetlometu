idf_component_register(
	SRCS
		"animations.c"
	
	INCLUDE_DIRS
		"include"

	REQUIRES
		driver
		esp_timer
	PRIV_REQUIRES
)