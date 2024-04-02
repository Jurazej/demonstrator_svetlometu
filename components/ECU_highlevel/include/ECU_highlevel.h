#include <stdio.h>
#include "esp_timer.h"

#include "TLD7002.h"
#include "animations.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "nvs_flash.h"
#include "nvs.h"




#ifndef _ECU_
#define _ECU_


#define NVS_PROFILE	"ECU_PROFILE"
#define HOW_MANY_SUPERANIMTIONS_DEFAULT 8
#define NUM_OF_CAR_LIGHT_FUNCTIONS 8
#define LENGTH_OF_CAR_LIGHT_BUFFER 200


typedef struct{
	uint8_t on_of;
	uint8_t which_light;
	car_light_function_t whole_light;
} whole_light_test_t;




typedef struct{
	TLD7002_sending_parameters_t sending_params;
	TLD7002_upd_cfg_t config;
	car_light_function_t Function[NUM_OF_LIGHTS][NUM_OF_CAR_LIGHT_FUNCTIONS];
	uint8_t active_functions[NUM_OF_LIGHTS];
	uint8_t MAIN_BUFFER[NUM_OF_LIGHTS][LENGTH_OF_CAR_LIGHT_BUFFER];
	whole_light_test_t test;
}Hsli_bus_params_t;


_Bool load_ECU_configuration(Hsli_bus_params_t *bus, char * profile_name);
_Bool save_ECU_configuration(Hsli_bus_params_t *bus, char * profile_name);


//hsli bus
_Bool set_number_of_parts(Hsli_bus_params_t *config,uint8_t which_light, uint8_t the_amount);
_Bool set_address(Hsli_bus_params_t* bus,uint8_t what_light_to_set_to__insert_index_please,   uint8_t in_what_row_is_this_address__insert_index_please, uint8_t put_the_address_here_1_to_31_btw);
_Bool set_amount_of_drivers(Hsli_bus_params_t* bus,uint8_t what_light__index,uint8_t the_amount);
_Bool set_number_of_active_lights(Hsli_bus_params_t* bus,uint8_t how_many_lights_are_active_here);
_Bool set_if_the_driver_is_inverted(Hsli_bus_params_t* bus,uint8_t which_light_would_it_be__indexagain,uint8_t which_driver_would_it_be__indexagain, _Bool the_state__true_or_false);
_Bool set_amount_of_drivers(Hsli_bus_params_t* bus,uint8_t what_light__index,uint8_t the_amount);
_Bool make_wiring_mask(Hsli_bus_params_t* bus,uint8_t what_light__index, uint8_t driver, uint16_t maskasinteger);
_Bool set_light_part(Hsli_bus_params_t *config, uint8_t on_what_light_is_it,	uint8_t what_function_is_it_asnumber,	uint8_t position__or_in_other_words_offset_from_left, 	uint8_t how_many_segments_is_it_long);


#endif
//============================================================================================


#ifndef _TLD7002_HIGHLEVEL_
#define _TLD7002_HIGHLEVEL_




void update_routine(Hsli_bus_params_t *HSLI_bus_params);
void set_updating(Hsli_bus_params_t *config); //not used
_Bool set_sending_type(Hsli_bus_params_t *bus, uint8_t the_type);
void scan_bus_fully(Hsli_bus_params_t *bus, uint8_t the_type);
void scan_bus_for_drivers(Hsli_bus_params_t *bus, uint8_t the_type);
uint8_t yoink_avaiable_addresses(Hsli_bus_params_t *bus, uint8_t* array_for_addreses);
uint16_t get_amt_of_segs(Hsli_bus_params_t *bus,uint8_t light);


#endif


//complex animations
//============================================================================================
#ifndef _ANIMATIONS_HIGH_
#define _ANIMATIONS_HIGH_



typedef struct{
	car_light_function_t *function;
	uint8_t* Car_light_buffer;
} Basic_animations_tasking_t;



void set_animation_task(Hsli_bus_params_t *bus,uint8_t light, uint8_t function);
void Animate_lightfunction_freertosversion(void *pvParameters);
void work_on_animations(Hsli_bus_params_t *bus);
_Bool set_animation(Hsli_bus_params_t *bus,uint8_t whatnumberoflight, uint8_t whatpart_as_number, uint8_t whichanimation, uint16_t duration_in_msecs, uint8_t brightness, _Bool should_it_clear_the_field_at_start);
_Bool null_animation(Hsli_bus_params_t *bus,uint8_t whatnumberoflight, uint8_t whatpart_as_number);
void set_segment(Hsli_bus_params_t *bus, uint8_t light, uint16_t index, uint8_t level);



_Bool set_complex_animation(Hsli_bus_params_t *bus,uint8_t ForWhatLight,uint8_t ForWhatPart,uint8_t the_superAnimation,_Bool loop_or_not);
_Bool null_complex_animation(Hsli_bus_params_t *bus,uint8_t ForWhatLight,uint8_t ForWhatPart);
void complex_animations_check(Hsli_bus_params_t* bus);
void ADD_to_complex_animation_buffer(uint16_t Number_of_superanimation, uint16_t index_in_buffer, uint8_t how_many_times_should_it_be_done, uint8_t whichanimation, uint16_t duration_in_msecs, uint8_t brightness, _Bool should_it_clear_the_field_at_start);
void complex_animations_check_task(void *pvParameters);
void set_complexanimations(Hsli_bus_params_t *hsli_bus);



void Whole_light_test(void *pvParameters);
void set_Light_test_animation(Hsli_bus_params_t * bus, uint8_t light);
void null_Light_test_animation(Hsli_bus_params_t * bus);
void ECU_init(Hsli_bus_params_t *hsli_bus);
void prepare_light_test_task(Hsli_bus_params_t * bus);


#endif

//============================================================================================








