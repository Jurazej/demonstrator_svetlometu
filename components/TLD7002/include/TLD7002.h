


#define TX_pin GPIO_NUM_17
#define RX_pin GPIO_NUM_18


#define SYNC			0x55
#define	crc3_seed		0x5
#define BROADCAST		0x0
#define CRC8_SEED 		0xFF
#define CRC8_XOROUT 	0xFF
#define DLC_POSITION	3
#define MRC_POSITION	6
#define RES_BYTE 		0b00000000
#define HOW_MANY_CHANNELS_ON_ONE_DRIVER 16
#define NUM_OF_LIGHTS 4 // this can be changed but i assume more than 4 lights wont be happening
#define HOW_MANY_CHANNELS_ON_ONE_DRIVER 16
#define MAXIMUM_DRIVERS 16



void TLD7002_update_DC8(uint8_t adress,uint8_t* input_array);
void TLD7002_update_DC14(uint8_t adress,uint16_t* input_array);
void HWCR_init(uint8_t adress_hwcr);
void TLD7002_DC_sync(void);
void PM_change_init(uint8_t adress_PM);
void REQ_diag(uint8_t adress_diag);
void TLD7002_init(void);
_Bool read_report_of_diagnostics(uint8_t* where_to_save_ack, uint8_t* where_to_save_diagnostics);
_Bool read_response(uint8_t* where_to_save, uint8_t length_of_previosly_sent);


//upadting light
//============================================================================================
#ifndef _SENDING_FROM_BUFFER_TO_LIGHT_
#define _SENDING_FROM_BUFFER_TO_LIGHT_
#define LENGTH_OF_ACK 2

typedef struct{
	_Bool Can_we_move_on_another_light;
	_Bool send_dc_sync;
	uint16_t current_light;
	uint16_t previous_light;
	uint16_t current_driver;
	uint16_t previous_driver;
	uint8_t lenght_of_previous_message;
	uint8_t current_offset;
	uint16_t full_bus_roller_counter;
	uint8_t current_test_address;
	_Bool scan_done;
} TLD7002_sending_parameters_t;

typedef struct{
	uint16_t address;
	uint16_t wiring_mask;
	uint8_t invertion;
} TLD7002_driver_t;

//===============================================================================

typedef struct{
	TLD7002_driver_t drivers[MAXIMUM_DRIVERS];
	uint16_t drivers_amount;
	uint8_t invertion;
} light_t;

//===============================================================================

typedef struct{
	_Bool avaiable_addresses[32];
	uint8_t ack_bytes[32][2];
	uint8_t diagnostics_bytes[32][16];
} info_of_TLD7002_drivers_t;
//===============================================================================

typedef struct{ //*
	uint32_t delay_between_steps_in_ms;
	uint16_t amount_of_active_lights;
	uint16_t message_type;
	light_t Light[NUM_OF_LIGHTS];
	info_of_TLD7002_drivers_t driver_info;
} TLD7002_upd_cfg_t;





void set_refreshrate(uint16_t refreshrate, TLD7002_upd_cfg_t *config);
void TLD7002_update_task(void *Parameter);
_Bool part_of_buffer_sending(TLD7002_sending_parameters_t * parameters, TLD7002_upd_cfg_t *config, uint8_t *buffer);
_Bool part_of_dignoscics_request_sending(TLD7002_sending_parameters_t * parameters, TLD7002_upd_cfg_t *config);


void next_step_of_sending(TLD7002_upd_cfg_t *config, TLD7002_sending_parameters_t *parameters, uint8_t *buffer);
uint8_t prepare_body_of_message(uint8_t* array_of_segments,uint8_t offset_in_array, uint8_t inverse, uint16_t mask_for_driver_asinteger);

//void show_Light(uint8_t refresh_rate_in_Hz);
_Bool update_light(uint8_t whichlight,uint8_t whichDRIVER);
void normal_DC_update_routine(TLD7002_upd_cfg_t *config, TLD7002_sending_parameters_t *parameters, uint8_t *buffer);







#endif
