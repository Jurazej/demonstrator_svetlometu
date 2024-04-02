#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "esp_timer.h"
#include "animations.h"
#include "math.h"


//27_01
uint8_t innerarray1[32];// helps for shifting array





//=============================================

/*
functions below are not specific for this aplication
i made them to be so universal




*/

//=============================================
//REAL SIMPLE
void Set_level(uint8_t* Light_car_buffer,function_dimensons_t partoflight,uint8_t level){
	for(uint8_t b=0;b<partoflight.length;b++){
		Light_car_buffer[b+partoflight.position]=level;
	}
}

//===============================================================================

//length and offset tells the field (the indexes) the shift is applied to
//1: for contiuum going right and appearing at left like pacman
void R_array_shift1(uint8_t* Light_car_buffer,function_dimensons_t partoflight){ //overflow goes to left
	uint8_t b; //for FOR-loops

	for(b=0;b<partoflight.length;b++){
		if(b==0){
			innerarray1[0]=Light_car_buffer[partoflight.length-1+partoflight.position];
		}

		else{
			innerarray1[b]=Light_car_buffer[b-1+partoflight.position];
		}
	}

	for(b=0;b<partoflight.length;b++){
		Light_car_buffer[b+partoflight.position]=innerarray1[b];
	}
}

//===============================================================================

void L_array_shift1(uint8_t* Light_car_buffer,function_dimensons_t partoflight){ //overflow goes to left
	int8_t b; //for FOR-loops

	for(b=partoflight.length-1;b>-1;b--){
		if(b==partoflight.length-1){
			innerarray1[partoflight.length-1] = Light_car_buffer[0+partoflight.position];
		}

		else{
			innerarray1[b] = Light_car_buffer[b+1+partoflight.position];
		}
	}

	for(b=0-1;b<partoflight.length;b++){
		Light_car_buffer[b+partoflight.position]=innerarray1[b];
	}
}

//===============================================================================

//discontinued
//2: is cutted around the end of the shifted field (field is just part of the car-light i want to do stuf with. It is some function like tail, brakes, etc.) NOT USED
//just takes array, is interested only in part,which is set by length of field and ofset of field
void R_array_shift2(uint8_t* Light_car_buffer,uint8_t length_of_field,uint8_t offset_of_field){ //overflow goes to nothingness
	uint8_t b; //for FOR-loops
	for(b=length_of_field-1;b>0;b--){
		Light_car_buffer[b+offset_of_field] = Light_car_buffer[b+offset_of_field-1];
	}
	Light_car_buffer[offset_of_field] = MINIMUM_BRIGHTNESS;
}

//===============================================================================

void put_point(uint8_t* Light_car_buffer,function_dimensons_t partoflight,uint16_t stepcounter,uint8_t brightness,_Bool clear_at_beggining_of_any_step){
	if(clear_at_beggining_of_any_step){
		Set_level(Light_car_buffer,partoflight,MINIMUM_BRIGHTNESS);
	}
	if(stepcounter<partoflight.length){ //if its in region of influence of the function that was put into this function (so it wont go beyond the borders of TAIL for example) (it can't go below 0 for its unsigned)
		Light_car_buffer[partoflight.position+stepcounter] = brightness;
	}
}

//===============================================================================

void S_put_array(uint8_t* Light_car_buffer,function_dimensons_t partoflight, uint8_t* slided_array, uint8_t length_of_slided_array, uint16_t stepcounter,_Bool clear_every_time){

	for(uint8_t b=0;b<partoflight.length;b++){
		int8_t bigarrayindex		= b+partoflight.position;
		int8_t insertedarrayindex	= b+length_of_slided_array-stepcounter;
		if(insertedarrayindex<0 || insertedarrayindex>length_of_slided_array-1){
			if(clear_every_time){
				Light_car_buffer[bigarrayindex] = MINIMUM_BRIGHTNESS;
			}
			else{
				continue;
			}

		}
		else{
			Light_car_buffer[bigarrayindex]=slided_array[insertedarrayindex];
		}
	}
}



//========================================================================

//========================================================================
//MORE COMPLEX (REQUIERS SOME STEP COUNTING)
//it is one step that has to be repeated until "LSB" of my array i want to display is displayed
//so in other words it is good to be used in a FOR which iterates as many times as my slided_array is long
//but no for loop will be used for non-blocking programming


//===============================================================================

void R_slide_array(uint8_t* Light_car_buffer,car_light_function_t *Function, uint8_t *slided_array, uint8_t length_of_slided_array){
	S_put_array(Light_car_buffer,Function->dimensions,slided_array,length_of_slided_array,Function->basic_anim_vars.step,true);

	if(Function->basic_anim_vars.step > Function->dimensions.length+length_of_slided_array){
		Function->basic_anim_vars.step = 0;
		Function->basic_anim_vars.period_counter += 1;

	}
	else{
		Function->basic_anim_vars.step += 1;
	}
}

//===============================================================================

void L_slide_array(uint8_t* Light_car_buffer,car_light_function_t *Function, uint8_t *slided_array, uint8_t length_of_slided_array){
	uint16_t index = Function->dimensions.length+length_of_slided_array-1-Function->basic_anim_vars.step;
	S_put_array(Light_car_buffer,Function->dimensions,slided_array,length_of_slided_array,index,true);


	Function->basic_anim_vars.step += 1;
	if(Function->basic_anim_vars.step > Function->dimensions.length-1){
		Function->basic_anim_vars.step = 0;
		Function->basic_anim_vars.period_counter += 1;
	}

}

//===============================================================================

//One point bouncing from sides
void bouncing_ball(uint8_t* Light_car_buffer, car_light_function_t *Function){
	uint8_t index = abs(Function->dimensions.length -1-Function->basic_anim_vars.step);

	Set_level(Light_car_buffer,Function->dimensions,MINIMUM_BRIGHTNESS);

	Light_car_buffer[index+Function->dimensions.position] = Function->basic_anim_vars.brightness;

	if(Function->basic_anim_vars.step<2*Function->dimensions.length-3){ //just part of logic for going right and left by only iterating
		Function->basic_anim_vars.step += 1;
	}
	else{
		Function->basic_anim_vars.step = 0;
		Function->basic_anim_vars.period_counter += 1;
	}
}

//===============================================================================

void breathe(uint8_t* Light_car_buffer,car_light_function_t *Function){
	uint8_t brightness = Function->basic_anim_vars.brightness- abs(Function->basic_anim_vars.brightness-Function->basic_anim_vars.step);
	Set_level(Light_car_buffer,Function->dimensions,brightness);
	if(Function->basic_anim_vars.step>2*Function->basic_anim_vars.brightness-2){
		Function->basic_anim_vars.step = 0;
		Function->basic_anim_vars.period_counter += 1;
	}
	else{
		Function->basic_anim_vars.step += 1;
	}
}

//===============================================================================

void R_point_slide(uint8_t* Light_car_buffer,car_light_function_t *Function){
	put_point(Light_car_buffer, Function->dimensions, Function->basic_anim_vars.step, Function->basic_anim_vars.brightness, true);

	//incremention of step
	Function->basic_anim_vars.step += 1;
	if(Function->basic_anim_vars.step > Function->dimensions.length-1){
		Function->basic_anim_vars.step = 0;
		Function->basic_anim_vars.period_counter += 1;
	}
}

//===============================================================================

void L_point_slide(uint8_t* Light_car_buffer,car_light_function_t *Function){
	uint16_t index = Function->dimensions.length-1-Function->basic_anim_vars.step;
	put_point(Light_car_buffer, Function->dimensions, index, Function->basic_anim_vars.brightness, true);

	//decremention of step
	Function->basic_anim_vars.step += 1;
	if(Function->basic_anim_vars.step > Function->dimensions.length-1){
		Function->basic_anim_vars.step = 0;
		Function->basic_anim_vars.period_counter += 1;
	}
}

//===============================================================================

void R_fill(uint8_t* Light_car_buffer,car_light_function_t *Function){
	put_point(Light_car_buffer, Function->dimensions, Function->basic_anim_vars.step, Function->basic_anim_vars.brightness, false);

	//incremention of step
	Function->basic_anim_vars.step += 1;
	if(Function->basic_anim_vars.step > Function->dimensions.length-1){
		Function->basic_anim_vars.step = 0;
		Function->basic_anim_vars.period_counter += 1;
	}
}

//===============================================================================

void L_fill(uint8_t* Light_car_buffer,car_light_function_t *Function){
	uint16_t index = Function->dimensions.length-1-Function->basic_anim_vars.step;

	put_point(Light_car_buffer, Function->dimensions, index, Function->basic_anim_vars.brightness, false);

	//decremention of step
	Function->basic_anim_vars.step += 1;
	if(Function->basic_anim_vars.step > Function->dimensions.length-1){
		Function->basic_anim_vars.step = 0;
		Function->basic_anim_vars.period_counter += 1;
	}


}
//=====================================================================
//=====================================================================
//=====================================================================

/*
this function takes:
 * 	the car-light buffer
 * 	the coordinates where to do "stuff"
 * 	what animation is being worked on (pointer to it, so it can edit it by itself)
 * 	what is the period of the animation
 * 	variable to save steps (pointer to it, so it can edit it by itself)
 * 	variable to save the time it performed step last time (pointer to it, so it can edit it by itself)
 * 	brightness of the animation
 * 	info whether or not to clear the field defined by the coordinates (as i stated as the second thing) at the start of period (that means at the first step of animation)
*/

//it basically just edits the car-light buffer some fancy way






void Next_step_of_animation(car_light_function_t *Function, uint8_t* Light_car_buffer){
				switch(Function->basic_anim_vars.animation_type){
					case 5:
						if(Function->basic_anim_vars.clear_at_beggining ){
							Function->basic_anim_vars.clear_at_beggining = 0;
							Set_level(Light_car_buffer, Function->dimensions, MINIMUM_BRIGHTNESS);
						}
						break;
					default:
						if(Function->basic_anim_vars.clear_at_beggining && (Function->basic_anim_vars.step == 0)){
							Set_level(Light_car_buffer, Function->dimensions, MINIMUM_BRIGHTNESS);
						}
				}


				switch(Function->basic_anim_vars.animation_type){
				case 0: //====================================
					//just for sure
					//code should never get here
					break;
				case 1: //====================================
					R_point_slide(Light_car_buffer, Function);
					break;

				case 2: //====================================
					L_point_slide(Light_car_buffer, Function);
					break;

				case 3: //====================================
					uint8_t slided_array1[7]={Function->basic_anim_vars.brightness*10/100,Function->basic_anim_vars.brightness*25/100,Function->basic_anim_vars.brightness*40/100, Function->basic_anim_vars.brightness*55/100,Function->basic_anim_vars.brightness*70/100,Function->basic_anim_vars.brightness*85/100,Function->basic_anim_vars.brightness};
					R_slide_array(Light_car_buffer,Function,slided_array1,7);
					break;

				case 4: //====================================
					uint8_t slided_array2[7]={Function->basic_anim_vars.brightness, Function->basic_anim_vars.brightness*85/100, Function->basic_anim_vars.brightness*70/100, Function->basic_anim_vars.brightness*55/100, Function->basic_anim_vars.brightness*40/100,Function->basic_anim_vars.brightness*25/100,Function->basic_anim_vars.brightness*10/100};
					L_slide_array(Light_car_buffer,Function,slided_array2,7);
					break;

				case 5: //====================================
					bouncing_ball(Light_car_buffer,Function); //works nicely
					break;

				case 6: //====================================
					breathe(Light_car_buffer,Function); //works nicely
					break;

				case 7: //====================================
					R_fill(Light_car_buffer,Function); //works nicely
					break;

				case 8: //====================================
					L_fill(Light_car_buffer, Function); //works nicely
					break;

				case 0xfe: //====================================
					Set_level(Light_car_buffer, Function->dimensions,Function->basic_anim_vars.brightness);
					Function->basic_anim_vars.animation_type = 0;
					break;

				case 0xff: //====================================
					Set_level(Light_car_buffer,Function->dimensions,MINIMUM_BRIGHTNESS);
					Function->basic_anim_vars.animation_type = 0;
					break;
				default: //====================================
					//user assigned some BS
					Function->basic_anim_vars.animation_type = 0;
					break;
				}
			}

/*
void Animate_lightfunction_timerversion(car_light_function_t *Function, uint8_t* Light_car_buffer){
	if(Function->dimensions.length!=0 && Function->basic_anim_vars.animation_type!=0){//the Light function isnt set => animations cant be done

			uint32_t duration_of_one_step;
			//find out what is the current time between steps
			switch(Function->basic_anim_vars.animation_type){
			case 6:
				//this one is special, because its
				duration_of_one_step = Function->basic_anim_vars.duration*1000/Function->basic_anim_vars.brightness;//make it micros and divide that by what is maximum set brightness for brightness=amountofsteps (by logic)

				break;
			default:
				duration_of_one_step = Function->basic_anim_vars.duration*1000/Function->dimensions.length; //make it microsecs and divide by how many steps have to be done to complete cycle of animation

			}



			//check if it is time to to another step on animation
			if(esp_timer_get_time() - Function->basic_anim_vars.clock >= duration_of_one_step){ //check if its time to work on animation
				Function->basic_anim_vars.clock = esp_timer_get_time();
				Next_step_of_animation(Function, Light_car_buffer);
			}
	}
}*/




//puts the numbers into struct
void set_anim_XXdev(basic_anim_vars_t* basic_anim_variables, uint8_t whichanimation, uint16_t duration_in_msecs, uint8_t brightness, _Bool should_it_clear_the_field_at_start){
	if(whichanimation != basic_anim_variables->animation_type){
		basic_anim_variables->animation_type	=	whichanimation;
		basic_anim_variables->step				=	0;
	}
	basic_anim_variables->duration				=	duration_in_msecs;
	basic_anim_variables->brightness			=	brightness;
	basic_anim_variables->clear_at_beggining	=	should_it_clear_the_field_at_start;
	basic_anim_variables->period_counter		= 	0;
}

//to shorten the code
void null_animation_XXdev(basic_anim_vars_t* basic_anim_variables){
	basic_anim_variables->duration			=	0;
	basic_anim_variables->animation_type	=	0xff;
	basic_anim_variables->brightness		=	0;
	basic_anim_variables->step				=	0;
}









//===============================================================================

_Bool complex_animations_stepping(complexAnim_config_t* config, complex_anim_t* complexanim, basic_anim_vars_t* basicanim){
	_Bool returnvalue = false;
	if(config->on_of){

		if(config->current_position == 0 || (basicanim->period_counter    >=    complexanim->buffer_of_arguments->how_many_periods_should_be_done)){

			if(config->current_position    >    complexanim->length_of_animation){//if the animation is at an end

				if(config->loop_or_not){
					config->current_position = 0; //reset the animation
				}
				else{
					null_animation_XXdev(basicanim);
					config->on_of = NULL_COMPLEXANIMATION;
				}
			}
			//many arguments but nothing code breaking
			//*perhaps fix
			if(config->on_of){//double check

				set_anim_XXdev(

					basicanim,
					complexanim->buffer_of_arguments[config->current_position].animation_type,
					complexanim->buffer_of_arguments[config->current_position].duration_in_msecs,
					complexanim->buffer_of_arguments[config->current_position].brightness,
					complexanim->buffer_of_arguments[config->current_position].should_it_clear_the_field_at_start
				);
				returnvalue = true;
			}


			config->current_position++;
		}
	}
	return returnvalue;
}

//===============================================================================

void ADD_to_complex_animation_buffer_XXdev(complex_anim_t *buffer_of_arguments, uint16_t index_in_buffer, uint8_t how_many_times_should_it_be_done, uint8_t whichanimation, uint16_t duration_in_msecs, uint8_t brightness, _Bool should_it_clear_the_field_at_start){
	buffer_of_arguments->buffer_of_arguments[index_in_buffer].animation_type = whichanimation;
	buffer_of_arguments->buffer_of_arguments[index_in_buffer].duration_in_msecs = duration_in_msecs;
	buffer_of_arguments->buffer_of_arguments[index_in_buffer].brightness = brightness;
	buffer_of_arguments->buffer_of_arguments[index_in_buffer].should_it_clear_the_field_at_start = should_it_clear_the_field_at_start;
	buffer_of_arguments->buffer_of_arguments[index_in_buffer].how_many_periods_should_be_done = how_many_times_should_it_be_done;
	if(index_in_buffer > buffer_of_arguments->length_of_animation){
		buffer_of_arguments->length_of_animation = index_in_buffer;
		printf("%d\n",buffer_of_arguments->length_of_animation);
	}
}

//===============================================================================

void set_complex_animation_XXdev(complexAnim_config_t* config,uint8_t the_superAnimation,_Bool loop_or_not){
	config->active_animation = the_superAnimation;
	config->current_position = 0;
	config->loop_or_not = loop_or_not;
	config->on_of = 1;
}

//===============================================================================

void null_complex_animation_XXdev(complexAnim_config_t* config){
	config->active_animation = 0;
	config->current_position = 0;
	config->loop_or_not = 0;
	config->on_of = 0;
}
