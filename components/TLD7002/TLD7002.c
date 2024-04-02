
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "driver/uart.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <inttypes.h>
#include "TLD7002.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

//what will be used from ECU_config
//uint8_t ARRAY_OF_LIGHTARRAYS[NUM_OF_LIGHTS][128]; //>512 BYTES,, array of 4 lights
//uint8_t ADDRESSES_OF_LIGHTS[NUM_OF_LIGHTS][14];//array containing arrays of addresses of at max 4 lights and lets say maximum 14 addresses if 2 lights are set (i assume no super duper mega light of >14 drivers on it will be connected to this)

//uint8_t MASKS_FOR_DRIVERS[NUM_OF_LIGHTS][MAXIMUM_DRIVERS][HOW_MANY_CHANNELS_ON_ONE_DRIVER]; //tells us active channels on the drivers
//HEADER==
uint8_t MRC = 0x0;	//init mrc
uint8_t DLC;
uint8_t FUN;
//========
uint8_t HSLI_message[36];
uint8_t array_to_send[16];
//uint8_t current_offset;






//set uart parameters

uart_config_t uart_config = {
	.baud_rate = 380400,
	.data_bits = UART_DATA_8_BITS,
	.parity    = UART_PARITY_DISABLE,
	.stop_bits = UART_STOP_BITS_1,
	.flow_ctrl = UART_HW_FLOWCTRL_DISABLE
};

void TLD7002_init(void) //practically initializing just uart
{
	uart_param_config(UART_NUM_1, &uart_config);
	uart_set_pin(UART_NUM_1, TX_pin, RX_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
	uart_driver_install(UART_NUM_1, 256, 256, 0, NULL, 0);
}

//=======================================================================

//=======================================================================
//crc3

/* CRC3 lookup table for 5 bit value */
const uint8_t TLD7002_LOOKUP_CRC3_5BIT[32] = {
	0, 1, 5, 4, 7, 6, 2, 3, 6, 7, 3, 2, 1, 0, 4, 5,
	3, 2, 6, 7, 4, 5, 1, 0, 5, 4, 0, 1, 2, 3, 7, 6
};

/* CRC3 lookup table for 8 bit value */
const uint8_t TLD7002_LOOKUP_CRC3_8BIT[256] = {
	0, 3, 4, 7, 2, 1, 6, 5, 1, 2, 5, 6, 3, 0, 7, 4,
	5, 6, 1, 2, 7, 4, 3, 0, 4, 7, 0, 3, 6, 5, 2, 1,
	7, 4, 3, 0, 5, 6, 1, 2, 6, 5, 2, 1, 4, 7, 0, 3,
	2, 1, 6, 5, 0, 3, 4, 7, 3, 0, 7, 4, 1, 2, 5, 6,
	6, 5, 2, 1, 4, 7, 0, 3, 7, 4, 3, 0, 5, 6, 1, 2,
	3, 0, 7, 4, 1, 2, 5, 6, 2, 1, 6, 5, 0, 3, 4, 7,
	1, 2, 5, 6, 3, 0, 7, 4, 0, 3, 4, 7, 2, 1, 6, 5,
	4, 7, 0, 3, 6, 5, 2, 1, 5, 6, 1, 2, 7, 4, 3, 0,
	3, 0, 7, 4, 1, 2, 5, 6, 2, 1, 6, 5, 0, 3, 4, 7,
	6, 5, 2, 1, 4, 7, 0, 3, 7, 4, 3, 0, 5, 6, 1, 2,
	4, 7, 0, 3, 6, 5, 2, 1, 5, 6, 1, 2, 7, 4, 3, 0,
	1, 2, 5, 6, 3, 0, 7, 4, 0, 3, 4, 7, 2, 1, 6, 5,
	5, 6, 1, 2, 7, 4, 3, 0, 4, 7, 0, 3, 6, 5, 2, 1,
	0, 3, 4, 7, 2, 1, 6, 5, 1, 2, 5, 6, 3, 0, 7, 4,
	2, 1, 6, 5, 0, 3, 4, 7, 3, 0, 7, 4, 1, 2, 5, 6,
	7, 4, 3, 0, 5, 6, 1, 2, 6, 5, 2, 1, 4, 7, 0, 3
};

/* CRC3 lookup table used for reflection */
const uint8_t TLD7002_MIRROR_MID_CRC3[8] = {
	0, 4, 2, 6, 1, 5, 3, 7
};

uint8_t Crc3_calc(uint8_t adressbyte,uint8_t byte_mrc_dlc_fun) {
	uint8_t crc = crc3_seed;
	crc = TLD7002_LOOKUP_CRC3_5BIT[crc^adressbyte];				/*< CRC of lower 5 bits */
	crc = TLD7002_MIRROR_MID_CRC3[crc];										/*< get reflected CRC3 value */
	crc = TLD7002_LOOKUP_CRC3_8BIT[crc ^ byte_mrc_dlc_fun];					/*< get reflected CRC3 value */
	return crc;
}

//=====
//crc8




const uint8_t CRC8_SAE_J1850_LOOKUP[256] = {
    0x00, 0x1D, 0x3A, 0x27, 0x74, 0x69, 0x4E, 0x53, 0xE8, 0xF5, 0xD2, 0xCF, 0x9C, 0x81, 0xA6, 0xBB,
    0xCD, 0xD0, 0xF7, 0xEA, 0xB9, 0xA4, 0x83, 0x9E, 0x25, 0x38, 0x1F, 0x02, 0x51, 0x4C, 0x6B, 0x76,
    0x87, 0x9A, 0xBD, 0xA0, 0xF3, 0xEE, 0xC9, 0xD4, 0x6F, 0x72, 0x55, 0x48, 0x1B, 0x06, 0x21, 0x3C,
    0x4A, 0x57, 0x70, 0x6D, 0x3E, 0x23, 0x04, 0x19, 0xA2, 0xBF, 0x98, 0x85, 0xD6, 0xCB, 0xEC, 0xF1,
    0x13, 0x0E, 0x29, 0x34, 0x67, 0x7A, 0x5D, 0x40, 0xFB, 0xE6, 0xC1, 0xDC, 0x8F, 0x92, 0xB5, 0xA8,
    0xDE, 0xC3, 0xE4, 0xF9, 0xAA, 0xB7, 0x90, 0x8D, 0x36, 0x2B, 0x0C, 0x11, 0x42, 0x5F, 0x78, 0x65,
    0x94, 0x89, 0xAE, 0xB3, 0xE0, 0xFD, 0xDA, 0xC7, 0x7C, 0x61, 0x46, 0x5B, 0x08, 0x15, 0x32, 0x2F,
    0x59, 0x44, 0x63, 0x7E, 0x2D, 0x30, 0x17, 0x0A, 0xB1, 0xAC, 0x8B, 0x96, 0xC5, 0xD8, 0xFF, 0xE2,
    0x26, 0x3B, 0x1C, 0x01, 0x52, 0x4F, 0x68, 0x75, 0xCE, 0xD3, 0xF4, 0xE9, 0xBA, 0xA7, 0x80, 0x9D,
    0xEB, 0xF6, 0xD1, 0xCC, 0x9F, 0x82, 0xA5, 0xB8, 0x03, 0x1E, 0x39, 0x24, 0x77, 0x6A, 0x4D, 0x50,
    0xA1, 0xBC, 0x9B, 0x86, 0xD5, 0xC8, 0xEF, 0xF2, 0x49, 0x54, 0x73, 0x6E, 0x3D, 0x20, 0x07, 0x1A,
    0x6C, 0x71, 0x56, 0x4B, 0x18, 0x05, 0x22, 0x3F, 0x84, 0x99, 0xBE, 0xA3, 0xF0, 0xED, 0xCA, 0xD7,
    0x35, 0x28, 0x0F, 0x12, 0x41, 0x5C, 0x7B, 0x66, 0xDD, 0xC0, 0xE7, 0xFA, 0xA9, 0xB4, 0x93, 0x8E,
    0xF8, 0xE5, 0xC2, 0xDF, 0x8C, 0x91, 0xB6, 0xAB, 0x10, 0x0D, 0x2A, 0x37, 0x64, 0x79, 0x5E, 0x43,
    0xB2, 0xAF, 0x88, 0x95, 0xC6, 0xDB, 0xFC, 0xE1, 0x5A, 0x47, 0x60, 0x7D, 0x2E, 0x33, 0x14, 0x09,
    0x7F, 0x62, 0x45, 0x58, 0x0B, 0x16, 0x31, 0x2C, 0x97, 0x8A, 0xAD, 0xB0, 0xE3, 0xFE, 0xD9, 0xC4
};
uint8_t CalcCRC8(uint8_t * input,uint8_t len)
{
	uint8_t a;
	uint8_t _crc = CRC8_SEED;

    for (a = 0; a < len; a++)
    {
      _crc = CRC8_SAE_J1850_LOOKUP[_crc ^ input[a+3]];
    }

  return (_crc ^ CRC8_XOROUT);
}


//=======================================================================


void iterate_mrc(void)
{
  switch(MRC)
  {
    case 0:
    	MRC = 1;
        break;

    case 1:
    	MRC = 2;
        break;

    case 2:
    	MRC = 3;
        break;

    case 3:
    	MRC = 0;
        break;

  }
}
void set_header(uint8_t adress_set,uint8_t DLC_set,uint8_t FUN_set)
{
	uint8_t mrc_dlc_fun;
	uint8_t crc;

	mrc_dlc_fun  = (MRC << MRC_POSITION)	& 0b11000000;
	mrc_dlc_fun |= (DLC_set<<DLC_POSITION)	& 0b00111000;
	mrc_dlc_fun |= (FUN_set) 				& 0b00000111;
	crc = Crc3_calc(adress_set,mrc_dlc_fun);

	HSLI_message[0] = 0x55;
	HSLI_message[1] = (crc << 5) | adress_set;
	HSLI_message[2] = mrc_dlc_fun;

	iterate_mrc();
}

void send(uint8_t* what_to_send, uint8_t how_long_is_it){

	uart_write_bytes(UART_NUM_1, what_to_send, how_long_is_it);
	//printf("send\n");
}

//universal function for dcl that takes adress of driver and array of duty cycle info˘˘




void TLD7002_update_DC8(uint8_t adress,uint8_t* input_array){ //this just takes first 16 items of array and puts it to message array

	set_header(adress,4,1);
	HSLI_message[3]  = input_array[0];
	HSLI_message[4]  = input_array[1];
	HSLI_message[5]  = input_array[2];
	HSLI_message[6]  = input_array[3];

	HSLI_message[7]  = input_array[4];
	HSLI_message[8]  = input_array[5];
	HSLI_message[9]  = input_array[6];
	HSLI_message[10] = input_array[7];

	HSLI_message[11] = input_array[8];
	HSLI_message[12] = input_array[9];
	HSLI_message[13] = input_array[10];
	HSLI_message[14] = input_array[11];

	HSLI_message[15] = input_array[12];
	HSLI_message[16] = input_array[13];
	HSLI_message[17] = input_array[14];
	HSLI_message[18] = input_array[15];

	HSLI_message[19]=CalcCRC8(HSLI_message, 16);

	send(HSLI_message, 20);
}


void TLD7002_update_DC14(uint8_t adress,uint16_t* input_array){//this just takes first 16 items of array and puts it to message array

	set_header(adress,6,1);
	HSLI_message[3]  = (input_array[0]>>8) & 0b00111111;
	HSLI_message[4]  = input_array[0];
	HSLI_message[5]  = (input_array[1]>>8) & 0b00111111;
	HSLI_message[6]  = input_array[1];
	HSLI_message[7]  = (input_array[2]>>8) & 0b00111111;
	HSLI_message[8]  = input_array[2];
	HSLI_message[9]  = (input_array[3]>>8) & 0b00111111;
	HSLI_message[10] = input_array[3];
	HSLI_message[11] = (input_array[4]>>8) & 0b00111111;
	HSLI_message[12] = input_array[4];
	HSLI_message[13] = (input_array[5]>>8) & 0b00111111;
	HSLI_message[14] = input_array[5];
	HSLI_message[15] = (input_array[6]>>8) & 0b00111111;
	HSLI_message[16] = input_array[6];
	HSLI_message[17] = (input_array[7]>>8) & 0b00111111;
	HSLI_message[18] = input_array[7];
	HSLI_message[19] = (input_array[8]>>8) & 0b00111111;
	HSLI_message[20] = input_array[8];
	HSLI_message[21] = (input_array[9]>>8) & 0b00111111;
	HSLI_message[22] = input_array[9];
	HSLI_message[23] = (input_array[10]>>8) & 0b00111111;
	HSLI_message[24] = input_array[10];
	HSLI_message[25] = (input_array[11]>>8) & 0b00111111;
	HSLI_message[26] = input_array[11];
	HSLI_message[27] = (input_array[12]>>8) & 0b00111111;
	HSLI_message[28] = input_array[12];
	HSLI_message[29] = (input_array[13]>>8) & 0b00111111;
	HSLI_message[30] = input_array[13];
	HSLI_message[31] = (input_array[14]>>8) & 0b00111111;
	HSLI_message[32] = input_array[14];
	HSLI_message[33] = (input_array[15]>>8) & 0b00111111;
	HSLI_message[34] = input_array[15];

	HSLI_message[35]=CalcCRC8(HSLI_message, 32);

	send(HSLI_message, 36);
}
//UNUSED IN MY CODE and discontinued
/*void TLD7002_update_DC14(uint8_t adress,uint8_t* input_array,_Bool inverse){ //unused
	uint16_t level;
	uint8_t a;
	//stuff from datasheet
	set_header(adress,6,1);
	switch(inverse){
	case 0:
		for(a=0;a<16;a++){
			level = input_array[a]*163; // in the virtual light is the level in 0-100 (percents) so it is multiplied by 0x3FFF and divided by 100 (or just multiplied by 163) to make it 0-0X3FFF
			HSLI_message[2*a+3]=(level>>8)&0b00111111;
			HSLI_message[2*a+4]=level;
		}
		break;
	case 1:
		for(a=0;a<16;a++){
			level = input_array[a]*163;
			HSLI_message[2*a+3]=((~level)>>8);
			HSLI_message[2*a+4]=(~level)&0b00111111;
		}
		break;
	}
	HSLI_message[35]=CalcCRC8(HSLI_message, 32);
	send(adress, HSLI_message, 36);
	//uart_write_bytes(UART_NUM_1, HSLI_message, 36);


}*/
//==================================
void TLD7002_DC_sync(){
	//stuff from datasheet
	set_header(0,0,0);
	send(HSLI_message,3);
	//printf("DC_SYNC\n\n");

}

void REQ_diag(uint8_t adress_diag){
	//stuff from datasheet
	//printf("Posilam request na adresu: %d\n",adress_diag);
	set_header(adress_diag,4,2);
	send(HSLI_message,3);

}

//==============
//wont use probably
void PM_change_init(uint8_t adress_PM){
	//set header of message
	set_header(adress_PM,1,6);
	//powermode byte
	HSLI_message[3]=0x00;
	//filler zeros
	HSLI_message[4]=0x00;
	HSLI_message[5]=CalcCRC8(HSLI_message, 2);
	send(HSLI_message,6);
}
//===================================================================================
//wont used probably
void HWCR_init(uint8_t adress_hwcr){
	set_header(adress_hwcr,3,3);
	//everything below is from datasheet
	//reset overload registers to 0 (no overload detected)
	HSLI_message[3]=0;
	HSLI_message[4]=0;
	//reset openload registers to 0 (no openload detected)
	HSLI_message[5]=0;
	HSLI_message[6]=0;
	//reset sls to 0 (no single led short detected)
	HSLI_message[7]=0;
	HSLI_message[8]=0;
	//
	HSLI_message[9]=RES_BYTE;
	//reset status byte
	HSLI_message[10]=0;
	//

	HSLI_message[11]=CalcCRC8(HSLI_message, 8);
	send(HSLI_message,12);
}




uint8_t ack[50];


_Bool read_response(uint8_t* where_to_save, uint8_t length_of_previosly_sent){ //for everything that isn't broadcast (because then the drivers don't respond)

	 //ill read what i sent + two bytes of ack (better solution then somehow flushing the buffer right after tx is done)
	int length = uart_read_bytes(UART_NUM_1, ack, 50, 0);

	if(length==length_of_previosly_sent){
		where_to_save[0] = 0;
		where_to_save[1] = 0;
		//printf("no driver!\n");
		uart_flush_input(UART_NUM_1);
		return false;
	}
	else if(length==length_of_previosly_sent+2){
		//printf("byte1: %d\n", ack[length_of_previosly_sent]);
		//printf("byte2: %d\n", ack[length_of_previosly_sent+1]);
		where_to_save[0] = ack[length_of_previosly_sent];
		where_to_save[1] = ack[length_of_previosly_sent+1];
		uart_flush_input(UART_NUM_1);
		return true;
	}
	else if(length==2){
		//printf("byte1: %d\n", ack[length_of_previosly_sent]);
		//printf("byte2: %d\n", ack[length_of_previosly_sent+1]);
		where_to_save[0] = ack[0];
		where_to_save[1] = ack[1];
		uart_flush_input(UART_NUM_1);
		return true;
	}
	else if(length==0){
		//printf("Probably too fast bus\n");
		//printf("byte2: %d\n", ack[length_of_previosly_sent+1]);
		where_to_save[0] = ack[0];
		where_to_save[1] = ack[1];
		uart_flush_input(UART_NUM_1);
		return false;
	}
	return false;

}

_Bool read_report_of_diagnostics(uint8_t* where_to_save_ack, uint8_t* where_to_save_diagnostics){ //for diagnostics request

	int length = uart_read_bytes(UART_NUM_1, ack, 50, 20);
	//printf("LENGTH: %d\n", length);
	if(length==3){
		for(uint8_t b=0; b<16;b++){
			where_to_save_diagnostics[b] = 0;
		}
		where_to_save_ack[0] = 0;
		where_to_save_ack[1] = 0;
		//printf("no driver!\n");
		uart_flush_input(UART_NUM_1);
		return false;
	}
	else if(length==22){ //our sent message is in there too
		for(uint8_t b = 0; b<16; b++){
			where_to_save_diagnostics[b] = ack[3+b];
			printf("%d  ", ack[3+b]);
		}
		printf("\n");
		where_to_save_ack[0] = ack[20];
		where_to_save_ack[1] = ack[21];
	}
	else if(length==18){
		for(uint8_t b = 0; b<16; b++){
			where_to_save_diagnostics[b] = ack[b];
			printf("%d  ", ack[b]);
		}
		printf("\n");
		where_to_save_ack[0] = ack[16];
		where_to_save_ack[1] = ack[17];
	}
	uart_flush_input(UART_NUM_1);
	return true;
}









//=====================================================================================================================

//main functions to update lights

//warning: heavy logic and indexing ahead :)


//we have buffer of "virtual light" aka ARRAY_OF_LIGHTARRAYS, which contains info of DC for each SEGMENT, NOT CHANNEL.
//problem: Not all segments are connected as some would expect
uint8_t prepare_body_of_message(uint8_t* array_of_segments,uint8_t offset_in_array, uint8_t inverse, uint16_t mask_for_driver_asinteger){
	_Bool mask_as_bool_array[16]; //tells us how are chanells wired physically
	mask_as_bool_array[0] = mask_for_driver_asinteger>>15 & 1;
	mask_as_bool_array[1] = mask_for_driver_asinteger>>14 & 1;
	mask_as_bool_array[2] = mask_for_driver_asinteger>>13 & 1;
	mask_as_bool_array[3] = mask_for_driver_asinteger>>12 & 1;
	mask_as_bool_array[4] = mask_for_driver_asinteger>>11 & 1;
	mask_as_bool_array[5] = mask_for_driver_asinteger>>10 & 1;
	mask_as_bool_array[6] = mask_for_driver_asinteger>>9  & 1;
	mask_as_bool_array[7] = mask_for_driver_asinteger>>8  & 1;
	mask_as_bool_array[8] = mask_for_driver_asinteger>>7  & 1;
	mask_as_bool_array[9] = mask_for_driver_asinteger>>6  & 1;
	mask_as_bool_array[10] = mask_for_driver_asinteger>>5 & 1;
	mask_as_bool_array[11] = mask_for_driver_asinteger>>4 & 1;
	mask_as_bool_array[12] = mask_for_driver_asinteger>>3 & 1;
	mask_as_bool_array[13] = mask_for_driver_asinteger>>2 & 1;
	mask_as_bool_array[14] = mask_for_driver_asinteger>>1 & 1;
	mask_as_bool_array[15] = mask_for_driver_asinteger>>0 & 1;

	uint8_t a;
	uint8_t current_index=0;

	switch(inverse){

		case 0:
			for(a=0; a<16; a++){
				if (mask_as_bool_array[a]> 0){
					array_to_send[a] = array_of_segments[current_index+offset_in_array];
					current_index++; //iterates even when at the end
				}
				else{
					array_to_send[a]=0;
				}

			}
			break;

		case 1:
			for(a=0; a<16; a++){
				//is the channel with index of "a" active?
				if (mask_as_bool_array[a]> 0){
					array_to_send[a] = ~array_of_segments[current_index+offset_in_array];
					current_index++;
				}
				else{
					array_to_send[a]=0xff;
				}
			}
			break;
		}
	/*for(uint8_t b = 0; b<16;b++){
		printf("%d  ", array_to_send[b]);
	}
	printf("\n");*/
	return current_index;
}















void normal_DC_update_routine(TLD7002_upd_cfg_t *config, TLD7002_sending_parameters_t *parameters, uint8_t *buffer){

	switch(parameters->send_dc_sync){ //will we send duty cycle info to some driver or are we done with that and dc_sync is to be sent as broadcast?
					case true:

						parameters->lenght_of_previous_message += 3;
						parameters->send_dc_sync = false;
						parameters->full_bus_roller_counter++;
						TLD7002_DC_sync();

						break;



					case false: //we wont be sending dc_sync for now
						uint8_t amount_of_segs_written_on;

						//ACKS are read from previous sending because it is stbus->config.the buffer
						if(read_response(config->driver_info.ack_bytes[config->Light[parameters->previous_light].drivers[parameters->previous_driver].address],  parameters->lenght_of_previous_message)){
							//read previous ack and save it to certain index in array
							config->driver_info.avaiable_addresses[parameters->current_test_address] = true;
						}



						parameters->lenght_of_previous_message = 20;
						//parameters->Can_we_move_on_another_light = part_of_buffer_sending(parameters,  config, buffer); //yes/no


						amount_of_segs_written_on = prepare_body_of_message(buffer, parameters->current_offset, config->Light[parameters->current_light].drivers[parameters->current_driver].invertion, config->Light[parameters->current_light].drivers[parameters->current_driver].wiring_mask);
						parameters->current_offset += amount_of_segs_written_on;


						TLD7002_update_DC8(config->Light[parameters->current_light].drivers[parameters->current_driver].address,   array_to_send);
						parameters->previous_driver = parameters->current_driver;
						parameters->previous_light = parameters->current_light;


						if(parameters->current_driver >= config->Light[parameters->current_light].drivers_amount-1){//  we are done updating this lights addresses and we can move on

								parameters->current_offset = 0;
								if(parameters->current_light  >=  config->amount_of_active_lights-1){ //yep, this was the last light, lets send dc_sync in next and repeat the whole process
										parameters->send_dc_sync = true;
										parameters->current_driver = 0;
										parameters->current_light = 0;
								}
								else{ //nope, some more lights to update, lets just increment light
										parameters->current_driver = 0;
										parameters->current_light++;
								}
						}
						else{ //we are not done with this light and also need to update more addresses
								parameters->current_driver++;
						}


						break;


		}
}


void test_DC_update(TLD7002_upd_cfg_t *config, TLD7002_sending_parameters_t *parameters){
	switch(parameters->send_dc_sync){ //will we send duty cycle info to some driver or are we done with that and dc_sync is to be sent as broadcast?
				case false: //we wont be sending dc_sync for now

					uint8_t test_buffer[16] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
					TLD7002_update_DC8(parameters->current_test_address,   test_buffer);


					if(parameters->current_test_address >= 31){ //yep, this was the last, lets send dc_sync in next and repeat the whole process
						parameters->send_dc_sync = true;
						parameters->current_test_address = 0;
					}
					else{ //nope, some more lights to update, lets just increment this index
						parameters->current_test_address ++;
					}
					break;

				case true:
					parameters->send_dc_sync = false;
					parameters->full_bus_roller_counter++;
					printf("synced\n");
					TLD7002_DC_sync();

					break;
	}
}
void BUS_diagnnostics(TLD7002_upd_cfg_t *config, TLD7002_sending_parameters_t *parameters){
	config->driver_info.avaiable_addresses[parameters->current_test_address] = read_report_of_diagnostics(config->driver_info.ack_bytes[parameters->current_test_address], config->driver_info.diagnostics_bytes[parameters->current_test_address]);
	printf("adresa %d: %d\n\n",parameters->current_test_address,    config->driver_info.avaiable_addresses[parameters->current_test_address]);
	if(parameters->current_test_address < 31){
		parameters->current_test_address++;
		REQ_diag(parameters->current_test_address);
	}
	else{
		config->message_type = 0;
		parameters->full_bus_roller_counter = 0;

	}
	parameters->scan_done = 1;

}



void simple_BUS_diagnnostics_just_for_drivers(TLD7002_upd_cfg_t *config, TLD7002_sending_parameters_t *parameters){
	config->driver_info.avaiable_addresses[parameters->current_test_address] = read_response(config->driver_info.ack_bytes[config->Light[parameters->previous_light].drivers[parameters->previous_driver].address],  parameters->lenght_of_previous_message);

	if(parameters->current_test_address < 31){
		parameters->current_test_address++;
		uint8_t test_buffer[16] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
		TLD7002_update_DC8(parameters->current_test_address,   test_buffer);
	}
	else{
		config->message_type = 0;
		parameters->full_bus_roller_counter = 0;

	}
	parameters->scan_done = 1;

}
//===============================================================================

void next_step_of_sending(TLD7002_upd_cfg_t *config, TLD7002_sending_parameters_t *parameters, uint8_t *buffer){
	switch(config->message_type){
	case 0:
		break;

	case 1: //send the buffer  //-------------------------------------------------------
		normal_DC_update_routine(config,parameters,buffer);
		break;
	case 2: //sending diagnosctics request    WIP  //-------------------------------------------------------
	case 3: //-------------------------------------------------------
		//this should scan the bus and save info of every channel
		//i wanted to use this for automatic parametrization of
		if(parameters->full_bus_roller_counter < 2){
			test_DC_update(config, parameters);
		}


		else if(parameters->full_bus_roller_counter >= 2){
			BUS_diagnnostics(config, parameters);
		}

		//just for sure
		if(parameters->full_bus_roller_counter>5){
			config->message_type = 0;

			parameters->full_bus_roller_counter = 0;
		}
		break;
	case 4: //-------------------------------------------------------

		if(parameters->full_bus_roller_counter >= 1){
			simple_BUS_diagnnostics_just_for_drivers(config, parameters);
		}

		//just for sure
		if(parameters->full_bus_roller_counter>5){
			config->message_type = 0;

			parameters->full_bus_roller_counter = 0;
		}
		break;
	}
}

//===============================================================================

_Bool part_of_dignoscics_request_sending(TLD7002_sending_parameters_t * parameters, TLD7002_upd_cfg_t *config){
	REQ_diag(config->Light[parameters->current_light].drivers[parameters->current_driver].address);
	if(parameters->current_driver >= config->Light[parameters->current_light].drivers_amount-1){//  we are done updating this lights addresses and we can move on
		return true;
	}
	else{ //we are not done with this light and also need to update more addresses
		return false; //this light needs some more addresses to be updated so we say by this that we are not moving on another light
	}
}

//===============================================================================

_Bool part_of_buffer_sending(TLD7002_sending_parameters_t * parameters, TLD7002_upd_cfg_t *config, uint8_t *buffer){
	uint8_t amount_of_segs_written_on;

	amount_of_segs_written_on = prepare_body_of_message(buffer, parameters->current_offset, config->Light[parameters->current_light].drivers[parameters->current_driver].invertion, config->Light[parameters->current_light].drivers[parameters->current_driver].wiring_mask);
	parameters->current_offset += amount_of_segs_written_on;
	/*printf("current offset: %d\n",parameters->current_offset);
	printf("current driver: %d\n",parameters->current_driver);
	printf("driver: %d\n",config->Light[parameters->current_light].drivers[ parameters->current_driver].address);*/
	TLD7002_update_DC8(config->Light[parameters->current_light].drivers[parameters->current_driver].address,   array_to_send);
	if(parameters->current_driver >= config->Light[parameters->current_light].drivers_amount-1){//  we are done updating this lights addresses and we can move on

		parameters->current_offset = 0;
		return true;

	}
	else{ //we are not done with this light and also need to update more addresses
		return false; //this light needs some more addresses to be updated so we say by this that we are not moving on another light
	}
}

//===============================================================================

void set_refreshrate(uint16_t refreshrate, TLD7002_upd_cfg_t *config){
	uint16_t amount_of_steps = 1; //one step is always DC_SYNC
	for(uint8_t b=0; b < config->amount_of_active_lights;b++){
		printf("drivers: %d\n", config->Light[b].drivers_amount);
		amount_of_steps += config->Light[b].drivers_amount;

	}
	printf("steps: %d\n", amount_of_steps);
	config->delay_between_steps_in_ms = 1000/(refreshrate*amount_of_steps);
	if(config->delay_between_steps_in_ms < 1){
		config->delay_between_steps_in_ms = 1;
	}
}

