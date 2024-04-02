#include "ECU_highlevel.h"

uint64_t last_time_we_sent_something;


//from ecu_config
//complex_anim_t ComplexAnimations[HOW_MANY_SUPERANIMTIONS_DEFAULT];
//========
complex_anim_t ComplexAnimations[HOW_MANY_SUPERANIMTIONS_DEFAULT];
SemaphoreHandle_t lookintocomplexanimations;
TaskHandle_t animationtask_handlers[NUM_OF_LIGHTS][NUM_OF_CAR_LIGHT_FUNCTIONS];
TaskHandle_t whole_light_aniamtion;

//====================================================

//================================



void ECU_init(Hsli_bus_params_t *hsli_bus){

	lookintocomplexanimations = xSemaphoreCreateBinary();
	set_complexanimations(hsli_bus);
	prepare_light_test_task(hsli_bus);
	TLD7002_init();
	nvs_flash_init();
	load_ECU_configuration(hsli_bus, NVS_PROFILE);

}







//Basic animations
//===============================================================================

void ensure_the_aniamtiontaskisrunning(Hsli_bus_params_t *bus,uint8_t whatnumberoflight, uint8_t whatpart_as_number){
	if(animationtask_handlers[whatnumberoflight][whatpart_as_number] == NULL){ //the task doesnt exist
		set_animation_task(bus,whatnumberoflight,whatpart_as_number);
	}

	eTaskState stateofanimationtask = eTaskGetState(animationtask_handlers[whatnumberoflight][whatpart_as_number]);
	if(stateofanimationtask==eSuspended){ //task is stopped
		vTaskResume(animationtask_handlers[whatnumberoflight][whatpart_as_number]);
	}
	bus->test.on_of = 0;
}

//===============================================================================

_Bool set_animation(Hsli_bus_params_t *bus,uint8_t whatnumberoflight, uint8_t whatpart_as_number, uint8_t whichanimation, uint16_t duration_in_msecs, uint8_t brightness, _Bool should_it_clear_the_field_at_start){ //everything is in numbers
	if(whatnumberoflight<NUM_OF_LIGHTS-1 && whatpart_as_number < NUM_OF_CAR_LIGHT_FUNCTIONS-1){
		null_complex_animation_XXdev(&bus->Function[whatnumberoflight][whatpart_as_number].complex_anim_vars);
		set_anim_XXdev(&bus->Function[whatnumberoflight][whatpart_as_number].basic_anim_vars, whichanimation, duration_in_msecs, brightness,should_it_clear_the_field_at_start);
		ensure_the_aniamtiontaskisrunning(bus,whatnumberoflight,whatpart_as_number);

		return true;
	}
	return false;

}

//===============================================================================

_Bool null_animation(Hsli_bus_params_t *bus,uint8_t whatnumberoflight, uint8_t whatpart_as_number){
	if(whatnumberoflight<NUM_OF_LIGHTS-1 && whatpart_as_number < NUM_OF_CAR_LIGHT_FUNCTIONS-1){
		null_animation_XXdev(&bus->Function[whatnumberoflight][whatpart_as_number].basic_anim_vars);
		//vTaskDelete(animationtask_handlers[whatnumberoflight][whatpart_as_number]);
		return true;
	}
		return false;
}

//===============================================================================

void Animate_lightfunction_freertosversion(void *pvParameters){

	Basic_animations_tasking_t* tasking_info = (Basic_animations_tasking_t*)pvParameters;
	car_light_function_t * Function = tasking_info->function;
	uint8_t *Light_car_buffer =  tasking_info->Car_light_buffer;



	while(1){

		if(Function->dimensions.length!=0 && Function->basic_anim_vars.animation_type!=0){//the Light function isnt set => animations cant be done

				uint32_t duration_of_one_step;
				//find out what is the current time between steps
				switch(Function->basic_anim_vars.animation_type){
				case 9:
				case 6:
					//this one is special, because its steps doesnt depend on the length of the
					duration_of_one_step = Function->basic_anim_vars.duration/Function->basic_anim_vars.brightness;

					break;
				default:
					duration_of_one_step = Function->basic_anim_vars.duration/Function->dimensions.length;

				}

				vTaskDelay(duration_of_one_step); // tickrate HAS TO BE 1kHz //actually not so important for animations but still better...

				Next_step_of_animation(Function, Light_car_buffer); //bundle of stepped animations
				if(!Function->basic_anim_vars.step){
					xSemaphoreGive(lookintocomplexanimations);
				}


		}
		else{
			 vTaskSuspend(NULL);

		}
	}
}

//===============================================================================

void set_animation_task(Hsli_bus_params_t *bus,uint8_t light, uint8_t function){
	Basic_animations_tasking_t temp_task_params;
	temp_task_params.Car_light_buffer = bus->MAIN_BUFFER[light];
	temp_task_params.function = &(bus->Function[light][function]);
	xTaskCreatePinnedToCore(Animate_lightfunction_freertosversion, "basic_animation", 2048, &temp_task_params, 14+light*NUM_OF_CAR_LIGHT_FUNCTIONS+function, &animationtask_handlers[light][function],0);
}

//unused
//==================================================
//Main function for working on animations
//version: no interrupt
//if no animation is selected, cpu just goes through and doesn't do anything
void work_on_animations(Hsli_bus_params_t *bus){ // pretty shit
	for(uint8_t light = 0; light < bus->config.amount_of_active_lights;light++){
		for(uint8_t function = 0; function < bus->active_functions[light]; function++){
			Animate_lightfunction_timerversion(&bus->Function[light][function], bus->MAIN_BUFFER[light]);
		}
	}
}

//===============================================================================

void prepare_light_test_task(Hsli_bus_params_t * bus){
	xTaskCreatePinnedToCore(Whole_light_test, "light_test", 2048, bus, 13, &whole_light_aniamtion, 0);
}

//===============================================================================

void Whole_light_test(void *pvParameters){

	Hsli_bus_params_t* HSLI_bus_params = (Hsli_bus_params_t *)pvParameters;



	set_anim_XXdev(&HSLI_bus_params->test.whole_light.basic_anim_vars, 5, 0, 128, 1);
	while(1){
		//printf("im doing something\n");
		if(HSLI_bus_params->test.on_of){//the Light function isnt set => animations cant be done

				vTaskDelay(500); // tickrate HAS TO BE 1kHz //actually not so important for animations but still better...
				Next_step_of_animation(&HSLI_bus_params->test.whole_light, HSLI_bus_params->MAIN_BUFFER[HSLI_bus_params->test.which_light]);

		}
		else{
			 vTaskSuspend(NULL);

		}
	}
}

//===============================================================================

void set_Light_test_animation(Hsli_bus_params_t * bus, uint8_t light){
	bus->test.which_light = light;
	bus->test.on_of = 1;
	bus->test.whole_light.dimensions.length = get_amt_of_segs(bus,bus->test.which_light);
	eTaskState stateoftest = eTaskGetState(whole_light_aniamtion);
	if(stateoftest==eSuspended){ //task is stopped
		vTaskResume(whole_light_aniamtion);
	}
}

//===============================================================================

void null_Light_test_animation(Hsli_bus_params_t * bus){
	bus->test.which_light = 0;
	bus->test.on_of = 0;
}






















//bus communication
//===============================================================================

//===============================================================================



//===============================================================================
//unused, because freertos is slow
void TLD7002_update_task(void *pvParams){

	Hsli_bus_params_t* HSLI_bus_params = (Hsli_bus_params_t *)pvParams;

	while(true){ //problematic. because vtaskdelay can be sending frames 1000Hz at most, while the lights need AT LEAST 1500Hz of frames when all addresses are active
		if(HSLI_bus_params->config.delay_between_steps_in_ms>0 && HSLI_bus_params->config.delay_between_steps_in_ms<10){
			vTaskDelay(HSLI_bus_params->config.delay_between_steps_in_ms);
		}
		else{
			vTaskDelay(1);
		}
		//printf("%ld\n",HSLI_bus_params.config.delay_between_steps_in_us/1000);
		next_step_of_sending(&HSLI_bus_params->config, &HSLI_bus_params->sending_params,HSLI_bus_params->MAIN_BUFFER[HSLI_bus_params->sending_params.current_light]);
	}
}

//===============================================================================


//timer version
//not really good, so unused
/*
void update_routine(Hsli_bus_params_t *HSLI_bus_params){

	if(esp_timer_get_time() - last_time_we_sent_something >= HSLI_bus_params->config.delay_between_steps_in_us){
		last_time_we_sent_something = esp_timer_get_time();
		next_step_of_sending(&HSLI_bus_params->config, &HSLI_bus_params->sending_params,HSLI_bus_params->MAIN_BUFFER[HSLI_bus_params->sending_params.current_light]);
	}
}
*/
//===============================================================================

void set_updating(Hsli_bus_params_t *hsli_bus){
	xTaskCreatePinnedToCore(TLD7002_update_task, "driver_update", 2048, hsli_bus, 10, NULL,1);
}






















//Complex Animations
//===============================================================================

//===============================================================================

void ADD_to_complex_animation_buffer(uint16_t Number_of_superanimation, uint16_t index_in_buffer, uint8_t how_many_times_should_it_be_done, uint8_t whichanimation, uint16_t duration_in_msecs, uint8_t brightness, _Bool should_it_clear_the_field_at_start){
	ADD_to_complex_animation_buffer_XXdev(&ComplexAnimations[Number_of_superanimation],index_in_buffer,how_many_times_should_it_be_done,whichanimation, duration_in_msecs, brightness,should_it_clear_the_field_at_start);

}

//===============================================================================

_Bool set_complex_animation(Hsli_bus_params_t *bus,uint8_t ForWhatLight,uint8_t ForWhatPart,uint8_t the_superAnimation,_Bool loop_or_not){
	if(ForWhatLight<NUM_OF_LIGHTS-1 && ForWhatPart < NUM_OF_CAR_LIGHT_FUNCTIONS-1){

		set_complex_animation_XXdev(&bus->Function[ForWhatLight][ForWhatPart].complex_anim_vars,		the_superAnimation,	loop_or_not);

		xSemaphoreGive(lookintocomplexanimations);
		return true;
	}
	return false;
}

//===============================================================================

_Bool null_complex_animation(Hsli_bus_params_t *bus,uint8_t ForWhatLight,uint8_t ForWhatPart){
	if(ForWhatLight<NUM_OF_LIGHTS-1 && ForWhatPart < NUM_OF_CAR_LIGHT_FUNCTIONS-1){
		null_complex_animation_XXdev(&bus->Function[ForWhatLight][ForWhatPart].complex_anim_vars);
		return true;
	}
	return false;

}

//===============================================================================

void complex_animations_check(Hsli_bus_params_t* bus){
	for(uint8_t light = 0; light < bus->config.amount_of_active_lights;light++){
		for(uint8_t function = 0; function < bus->active_functions[light]; function++){
			if(complex_animations_stepping(&bus->Function[light][function].complex_anim_vars,	 &ComplexAnimations[bus->Function[light][function].complex_anim_vars.active_animation], 	&bus->Function[light][function].basic_anim_vars)){
				ensure_the_aniamtiontaskisrunning(bus,light,function);
			}
		}
	}
}

//===============================================================================

void complex_animations_check_task(void *pvParameters) {
	Hsli_bus_params_t *bus = (Hsli_bus_params_t*)pvParameters;
    while (1) {
        if (xSemaphoreTake(lookintocomplexanimations, portMAX_DELAY) == pdTRUE) {
        	complex_animations_check(bus);
        }
    }
}

//===============================================================================

void set_complexanimations(Hsli_bus_params_t *hsli_bus){
	TaskHandle_t complexanim_handle;
	xTaskCreatePinnedToCore(complex_animations_check_task, "ecu_Cmplxanim", 2048, hsli_bus, 9, &complexanim_handle, 0);
	xSemaphoreGive(lookintocomplexanimations);
}

















//parametrization
//===============================================================================

//===============================================================================

_Bool set_number_of_parts(Hsli_bus_params_t *config,uint8_t which_light, uint8_t the_amount){
	config->active_functions[which_light] = the_amount;

	return true;
}

//===============================================================================

_Bool set_light_part(Hsli_bus_params_t *config, uint8_t on_what_light_is_it,	uint8_t what_function_is_it_asnumber,	uint8_t position__or_in_other_words_offset_from_left, 	uint8_t how_many_segments_is_it_long){
	config->Function[on_what_light_is_it][what_function_is_it_asnumber].dimensions.position = position__or_in_other_words_offset_from_left;
	config->Function[on_what_light_is_it][what_function_is_it_asnumber].dimensions.length = how_many_segments_is_it_long;

	return true;
}

//===============================================================================

_Bool set_amount_of_drivers(Hsli_bus_params_t* bus,uint8_t what_light__index,uint8_t the_amount){ //drivers=addresses
	if(what_light__index > NUM_OF_LIGHTS-1){
		return false;
	}
	bus->config.Light[what_light__index].drivers_amount = the_amount;

	return true;
}

//===============================================================================

_Bool set_address(Hsli_bus_params_t* bus,uint8_t what_light_to_set_to__insert_index_please,   uint8_t in_what_row_is_this_address__insert_index_please, uint8_t put_the_address_here_1_to_31_btw){
	//the drivers can be assigned in any order, but if i want to use it, it should be set how its physically next to each other
	if(what_light_to_set_to__insert_index_please  > NUM_OF_LIGHTS-1){
		return false;
	}
	if(in_what_row_is_this_address__insert_index_please  > MAXIMUM_DRIVERS-1){
		return false;
	}
	if(put_the_address_here_1_to_31_btw  > 0b11111 /*31*/){
		return false;
	}
	bus->config.Light[what_light_to_set_to__insert_index_please].drivers[in_what_row_is_this_address__insert_index_please].address = put_the_address_here_1_to_31_btw;

	return true;
}

//===============================================================================

_Bool set_number_of_active_lights(Hsli_bus_params_t* bus,uint8_t how_many_lights_are_active_here){
	if(how_many_lights_are_active_here > NUM_OF_LIGHTS){
		return false;
	}
	bus->config.amount_of_active_lights = how_many_lights_are_active_here;

	return true;
}

//===============================================================================

_Bool set_if_the_driver_is_inverted(Hsli_bus_params_t* bus,uint8_t which_light_would_it_be__indexagain,uint8_t which_driver_would_it_be__indexagain, _Bool the_state__true_or_false){
	if(which_light_would_it_be__indexagain  > NUM_OF_LIGHTS-1){
		return false;
	}
	bus->config.Light[which_light_would_it_be__indexagain].drivers[which_driver_would_it_be__indexagain].invertion = the_state__true_or_false;


	return true;
}

//===============================================================================


_Bool make_wiring_mask(Hsli_bus_params_t* bus,uint8_t what_light__index, uint8_t driver, uint16_t maskasinteger){ //example of mask: 0b111011101110111 this means that: 1 = the channel is conected; 0 = not connected
	bus->config.Light[what_light__index].drivers[driver].wiring_mask = maskasinteger;
	return true;
}











//saving on flash
//===============================================================================

//===============================================================================

_Bool save_ECU_configuration(Hsli_bus_params_t *bus, char * profile_name){
	nvs_handle_t ecu_nvs_handle;
	nvs_open(profile_name, NVS_READWRITE, &ecu_nvs_handle);


	//saving amount of lights
	nvs_set_u16(ecu_nvs_handle, "amt_adrs", bus->config.amount_of_active_lights);
	nvs_commit(ecu_nvs_handle);


	//saving amount of drivers
	char driversamt_key[13] = "amt_drivers";
	printf(driversamt_key);
	printf("\n");
	for(uint8_t helpvar_lights = 0; helpvar_lights < bus->config.amount_of_active_lights; helpvar_lights++){
		driversamt_key[11] = helpvar_lights+'A';
		printf(driversamt_key);
		printf("\n");
		nvs_set_u16(ecu_nvs_handle, driversamt_key, bus->config.Light[helpvar_lights].drivers_amount);
		nvs_commit(ecu_nvs_handle);

	}


	//saving addresses
	char driversadrs_key[14] = "adr_drivers  ";
	for(uint8_t helpvar_lights = 0; helpvar_lights < bus->config.amount_of_active_lights; helpvar_lights++){
		for(uint8_t helpvar_driver = 0; helpvar_driver < bus->config.Light[helpvar_lights].drivers_amount; helpvar_driver++){
			driversadrs_key[11] = helpvar_lights+'A';
			driversadrs_key[12] = helpvar_driver+'A';

			nvs_set_u16(ecu_nvs_handle, driversadrs_key, bus->config.Light[helpvar_lights].drivers[helpvar_driver].address);
			nvs_commit(ecu_nvs_handle);

		}
	}


	//saving wiring mask
	char drivers_wiring_key[14] = "wiring_drvs  ";
	for(uint8_t helpvar_lights = 0; helpvar_lights < bus->config.amount_of_active_lights; helpvar_lights++){
		for(uint8_t helpvar_driver = 0; helpvar_driver < bus->config.Light[helpvar_lights].drivers_amount; helpvar_driver++){
			drivers_wiring_key[11] = helpvar_lights+'A';
			drivers_wiring_key[12] = helpvar_driver+'A';

			nvs_set_u16(ecu_nvs_handle, drivers_wiring_key, bus->config.Light[helpvar_lights].drivers[helpvar_driver].wiring_mask);
			nvs_commit(ecu_nvs_handle);

		}
	}

	//saving if the light is inverted
	char driver_invert_key[14] = "driverinvrt  ";
	for(uint8_t helpvar_lights = 0; helpvar_lights < bus->config.amount_of_active_lights; helpvar_lights++){
		for(uint8_t helpvar_driver = 0; helpvar_driver < bus->config.Light[helpvar_lights].drivers_amount; helpvar_driver++){
			driver_invert_key[11] = helpvar_lights+'A';
			driver_invert_key[12] = helpvar_driver+'A';

			nvs_set_u8(ecu_nvs_handle, driver_invert_key, bus->config.Light[helpvar_lights].drivers[helpvar_driver].invertion);
			nvs_commit(ecu_nvs_handle);

		}
	}
	//SAVING PARAMETERS OF PARTS

	//saving number of parts on all lights light

	char part_amount_key[13] = "amountparts ";
	for(uint8_t helpvar_lights = 0; helpvar_lights < bus->config.amount_of_active_lights; helpvar_lights++){
		part_amount_key[11] = helpvar_lights+'A';
		nvs_set_u8(ecu_nvs_handle, part_amount_key, bus->active_functions[helpvar_lights]);
		nvs_commit(ecu_nvs_handle);

	}


	//saving position of all parts
	char parts_position[14] = "parts_posit  ";
	for(uint8_t helpvar_lights = 0; helpvar_lights < bus->config.amount_of_active_lights; helpvar_lights++){
		for(uint8_t helpvar_parts = 0; helpvar_parts < bus->active_functions[helpvar_lights]; helpvar_parts++){
			parts_position[11] = helpvar_lights+'A';
			parts_position[12] = helpvar_parts+'A';

			nvs_set_u16(ecu_nvs_handle, parts_position, bus->Function[helpvar_lights][helpvar_parts].dimensions.position);
			nvs_commit(ecu_nvs_handle);

		}
	}


	//saving length of all parts
	char parts_legth[14] = "partslength  ";
	for(uint8_t helpvar_lights = 0; helpvar_lights < bus->config.amount_of_active_lights; helpvar_lights++){
		for(uint8_t helpvar_parts = 0; helpvar_parts < bus->active_functions[helpvar_lights]; helpvar_parts++){
			parts_legth[11] = helpvar_lights+'A';
			parts_legth[12] = helpvar_parts+'A';

			nvs_set_u16(ecu_nvs_handle, parts_legth, bus->Function[helpvar_lights][helpvar_parts].dimensions.length);
			nvs_commit(ecu_nvs_handle);

		}
	}
	nvs_close(ecu_nvs_handle);
	return true;
}


//loading from flash
//==================================================================================

_Bool load_ECU_configuration(Hsli_bus_params_t *bus, char * profile_name){
	nvs_handle_t ecu_nvs_handle;
	nvs_open(profile_name, NVS_READWRITE, &ecu_nvs_handle);


	//saving amount of lights
	nvs_get_u16(ecu_nvs_handle, "amt_adrs", &bus->config.amount_of_active_lights);



	//saving amount of drivers
	char driversamt_key[13] = "amt_driversp";
	for(uint8_t helpvar_lights = 0; helpvar_lights < bus->config.amount_of_active_lights; helpvar_lights++){
		driversamt_key[11] = helpvar_lights+'A';
		printf(driversamt_key);
		printf("\n");
		nvs_get_u16(ecu_nvs_handle, driversamt_key, &bus->config.Light[helpvar_lights].drivers_amount);

	}


	//saving addresses
	char driversadrs_key[14] = "adr_drivers  ";
	for(uint8_t helpvar_lights = 0; helpvar_lights < bus->config.amount_of_active_lights; helpvar_lights++){
		for(uint8_t helpvar_driver = 0; helpvar_driver < bus->config.Light[helpvar_lights].drivers_amount; helpvar_driver++){
			driversadrs_key[11] = helpvar_lights+'A';
			driversadrs_key[12] = helpvar_driver+'A';

			nvs_get_u16(ecu_nvs_handle, driversadrs_key, &bus->config.Light[helpvar_lights].drivers[helpvar_driver].address);

		}
	}


	//saving wiring mask
	char drivers_wiring_key[14] = "wiring_drvs  ";
	for(uint8_t helpvar_lights = 0; helpvar_lights < bus->config.amount_of_active_lights; helpvar_lights++){
		for(uint8_t helpvar_driver = 0; helpvar_driver < bus->config.Light[helpvar_lights].drivers_amount; helpvar_driver++){
			drivers_wiring_key[11] = helpvar_lights+'A';
			drivers_wiring_key[12] = helpvar_driver+'A';

			nvs_get_u16(ecu_nvs_handle, drivers_wiring_key, &bus->config.Light[helpvar_lights].drivers[helpvar_driver].wiring_mask);

		}
	}
	char driver_invert_key[14] = "driverinvrt  ";
	for(uint8_t helpvar_lights = 0; helpvar_lights < bus->config.amount_of_active_lights; helpvar_lights++){
		for(uint8_t helpvar_driver = 0; helpvar_driver < bus->config.Light[helpvar_lights].drivers_amount; helpvar_driver++){
			driver_invert_key[11] = helpvar_lights+'A';
			driver_invert_key[12] = helpvar_driver+'A';

			nvs_get_u8(ecu_nvs_handle, driver_invert_key, &bus->config.Light[helpvar_lights].drivers[helpvar_driver].invertion);
			nvs_commit(ecu_nvs_handle);

		}
	}

	//SAVING PARAMETERS OF PARTS

	//saving number of parts on all lights light
	char part_amount_key[13] = "amountparts ";
	for(uint8_t helpvar_lights = 0; helpvar_lights < bus->config.amount_of_active_lights; helpvar_lights++){
		part_amount_key[11] = helpvar_lights+'A';
		nvs_get_u8(ecu_nvs_handle, part_amount_key, &bus->active_functions[helpvar_lights]);
		nvs_commit(ecu_nvs_handle);

	}
	//saving position of all parts
	char parts_position[14] = "parts_posit  ";
	for(uint8_t helpvar_lights = 0; helpvar_lights < bus->config.amount_of_active_lights; helpvar_lights++){
		for(uint8_t helpvar_parts = 0; helpvar_parts < bus->active_functions[helpvar_lights]; helpvar_parts++){
			parts_position[11] = helpvar_lights+'A';
			parts_position[12] = helpvar_parts+'A';

			nvs_get_u16(ecu_nvs_handle, parts_position, &bus->Function[helpvar_lights][helpvar_parts].dimensions.position);

		}
	}


	//saving length of all parts
	char parts_legth[14] = "partslength  ";
	for(uint8_t helpvar_lights = 0; helpvar_lights < bus->config.amount_of_active_lights; helpvar_lights++){
		for(uint8_t helpvar_parts = 0; helpvar_parts < bus->active_functions[helpvar_lights]; helpvar_parts++){
			parts_legth[11] = helpvar_lights+'A';
			parts_legth[12] = helpvar_parts+'A';

			nvs_get_u16(ecu_nvs_handle, parts_legth, &bus->Function[helpvar_lights][helpvar_parts].dimensions.length);

		}
	}

	nvs_close(ecu_nvs_handle);
	return true;
}










//some usable functions in main
//===============================================================================

//===============================================================================

_Bool set_sending_type(Hsli_bus_params_t *bus, uint8_t the_type){
	if(the_type<4){
		bus->config.message_type = the_type;
		bus->sending_params.scan_done = 0;
		return true;

	}
	return false;
}

//===============================================================================

uint8_t yoink_avaiable_addresses(Hsli_bus_params_t *bus, uint8_t* array_for_addreses){
	uint8_t amount_of_avaiable = 0;
	for(uint8_t b = 1; b < MAXIMUM_DRIVERS; b++){
		if(bus->config.driver_info.avaiable_addresses[b]){
			array_for_addreses[amount_of_avaiable] = b;
			amount_of_avaiable++;
		}

	}
	return amount_of_avaiable;
}

//===============================================================================

void scan_bus_fully(Hsli_bus_params_t *bus, uint8_t the_type){
	set_sending_type(bus, 3);
}
void scan_bus_for_drivers(Hsli_bus_params_t *bus, uint8_t the_type){
	set_sending_type(bus, 4);
}

//===============================================================================

uint16_t get_amt_of_segs(Hsli_bus_params_t *bus,uint8_t light){
	uint16_t segments = 0;
	for(uint8_t a=0; a<bus->config.Light[light].drivers_amount;a++){
		for(uint8_t b =0; b<16;b++){
			if(bus->config.Light[light].drivers[a].wiring_mask >> (15-b) & 1){
				segments++;
			}
		}
	}
	return segments;
}

//===============================================================================

void set_segment(Hsli_bus_params_t *bus, uint8_t light,uint16_t index, uint8_t level){
	bus->MAIN_BUFFER[light][index] = level;
}


