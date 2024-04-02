#include <stdio.h>


#define MINIMUM_BRIGHTNESS 0

//default of NUM_OF_LIGHTS is 4
//there are "[NUM_OF_LIGHTS]" <- this number of lights, therefore also that number of car-light-function (BLINKER will be NUM_OF_LIGHTS times - each for one of {NUM_OF_LIGHTS} lights)
//everything is represented by array and each index of these arrays carray info for different light (index 0 = 1st light... index 1 = 2nd light)

#ifndef TEST_H__
#define TEST_H__

#define NULL_COMPLEXANIMATION 0
#define	MAXIMUM_LENGTH_OF_BUFFER 16


typedef struct{
	uint8_t animation_type,
			brightness;

	//uint64_t clock;

	uint16_t duration,
			 step,
			 period_counter;

	_Bool clear_at_beggining;
} basic_anim_vars_t;

typedef struct{
	uint16_t length;
	uint16_t position;
} function_dimensons_t;


typedef struct{
//	uint8_t whatnumberoflight;
//	uint8_t whatpart_as_number;
	uint8_t animation_type;
	uint16_t duration_in_msecs;
	uint8_t brightness;
	_Bool should_it_clear_the_field_at_start;
	uint16_t how_many_periods_should_be_done;
} arguments_for_set_animation_t;


typedef struct{
	arguments_for_set_animation_t buffer_of_arguments[MAXIMUM_LENGTH_OF_BUFFER];
	uint16_t length_of_animation;

} complex_anim_t;



typedef struct{
	uint16_t current_position;
	uint16_t active_animation;
	_Bool on_of;
	_Bool loop_or_not;

} complexAnim_config_t;


typedef struct{
	basic_anim_vars_t basic_anim_vars;
	function_dimensons_t dimensions;
	complexAnim_config_t complex_anim_vars;
} car_light_function_t;



_Bool complex_animations_stepping(complexAnim_config_t* config, complex_anim_t* complexanim, basic_anim_vars_t* basicanim);
void ADD_to_complex_animation_buffer_XXdev(complex_anim_t *buffer_of_arguments, uint16_t index_in_buffer, uint8_t how_many_times_should_it_be_done, uint8_t whichanimation, uint16_t duration_in_msecs, uint8_t brightness, _Bool should_it_clear_the_field_at_start);

void set_complex_animation_XXdev(complexAnim_config_t* config,uint8_t the_superAnimation,_Bool loop_or_not);
void null_complex_animation_XXdev(complexAnim_config_t* config);





#endif
#ifndef _BASIC_ANIMATIONS_
#define _BASIC_ANIMATIONS_

//
void set_anim_XXdev(basic_anim_vars_t* basic_anim_variables, uint8_t whichanimation, uint16_t duration_in_msecs, uint8_t brightness, _Bool should_it_clear_the_field_at_start);
void null_animation_XXdev(basic_anim_vars_t* basic_anim_variables);
void Next_step_of_animation(car_light_function_t *Function, uint8_t* Light_car_buffer);
void Animate_lightfunction_timerversion(car_light_function_t *Function, uint8_t* Light_car_buffer);
//void Animate_lightfunction_freertosversion(car_light_function_t *Function, uint8_t* Light_car_buffer);



#endif




