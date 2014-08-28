/*
 * Copyright (c) 2009 Xilinx, Inc.  All rights reserved.
 *
 * Xilinx, Inc.
 * XILINX IS PROVIDING THIS DESIGN, CODE, OR INFORMATION "AS IS" AS A
 * COURTESY TO YOU.  BY PROVIDING THIS DESIGN, CODE, OR INFORMATION AS
 * ONE POSSIBLE   IMPLEMENTATION OF THIS FEATURE, APPLICATION OR
 * STANDARD, XILINX IS MAKING NO REPRESENTATION THAT THIS IMPLEMENTATION
 * IS FREE FROM ANY CLAIMS OF INFRINGEMENT, AND YOU ARE RESPONSIBLE
 * FOR OBTAINING ANY RIGHTS YOU MAY REQUIRE FOR YOUR IMPLEMENTATION.
 * XILINX EXPRESSLY DISCLAIMS ANY WARRANTY WHATSOEVER WITH RESPECT TO
 * THE ADEQUACY OF THE IMPLEMENTATION, INCLUDING BUT NOT LIMITED TO
 * ANY WARRANTIES OR REPRESENTATIONS THAT THIS IMPLEMENTATION IS FREE
 * FROM CLAIMS OF INFRINGEMENT, IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

/*
 * Please refer to Matlab code for code comments and description.
 */

/* Include files */
#include "es_controller.h"
//#include "xiomodule.h"
#include "drp.h"
#include "es_simple_eye_acq.h"
#include "xaxi_eyescan.h"

#define DEBUG FALSE

void es_simple_eye_acq(eye_scan *eye_struct)
{
	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% Does nothing but return.
	//%%%%%%%%%%%%%%%%%%  WAIT State (es_simple_eye_acq)  %%%%%%%%%%%%%%%%%%% Waits for RESET state
	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% to be written externally.
	if(eye_struct->state == WAIT_STATE || eye_struct->state == DONE_STATE) { //(WAIT x0000 = 0, DONE = 1)
		if( DEBUG ) printf( "did we get here?\n" );
		return;
	}

	if( DEBUG ) printf( "start es_simple_eye_acq\n");

	u16 vert_step_size = eye_struct->vert_step_size;
	u16 max_vert_offset = 0;
	u16 max_horz_offset = eye_struct->max_horz_offset;
	u16 data_width = eye_struct->data_width;
	//lpm_mode: Zero for LPM & 1 for DFE
	u8  max_ut_sign = 1 - eye_struct->lpm_mode;

	//Gear-shifting-related
	u16 min_error_count = 3;
	u8  max_prescale = eye_struct->max_prescale;
	u8  step_prescale = 3;
	s16 next_prescale = 0;

	u16 es_status = 0;
	u16 error_count = 0;
	u16 sample_count = 0;
	u16 es_state = 0;
	u16 es_done = 0;
	u16 prescale = 0;
	u32 center_error = 0;

	u16 horz_value = 0;
	u16 vert_value = 0;
	u16 phase_unification = 0;

	//Find largest multiple of vert_step_size that is less than 127
	max_vert_offset = 0;
	while(max_vert_offset < 127){
		max_vert_offset += vert_step_size;
	}
	max_vert_offset -= vert_step_size;

	//Read DONE bit & state of Eye Scan State Machine
	if( DEBUG ) printf( "es_simple_eye_acq read es_control_status lane %d\n" , eye_struct->lane_number );
	es_status = drp_read(ES_CONTROL_STATUS, eye_struct->lane_number);
	if( DEBUG ) printf( "lane %d es_status bit 0x%x\n" , eye_struct->lane_number , es_status );
	es_state = es_status >> 1;
	es_done = es_status & 1;
	if( DEBUG ) printf( "es_state 0x%x es_done 0x%x\n" , es_state , es_done );

	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% Initializes parameters for
	//%%%%%%%%%%%%%%%%%%  RESET State (es_simple_eye_acq)  %%%%%%%%%%%%%%%%%% statistical eye scan
	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% and sets ES_PRESCALE
	// Called with RESET state when eye acquisition is to be started.
	if(eye_struct->state == RESET_STATE) {  //(RESET x0010 = 16)
		//Eye Scan SM must be in WAIT state with RUN & ARM deasserted
		if(es_state != 0) {
			return;
		}
		if(eye_struct->lpm_mode == 0) {
			eye_struct->ut_sign = 1;
		} else {
			eye_struct->ut_sign = 0;
		}

		//Starts in center of eye
		eye_struct->horz_offset = 0;
		eye_struct->vert_offset = -max_vert_offset - vert_step_size; //Incremented to -max_vert_offset in SETUP state

		//Gear Shifting start
		drp_write(eye_struct->prescale, ES_PRESCALE, eye_struct->lane_number);
		//Gear Shifting end
		eye_struct->state = SETUP_STATE; //SETUP
	}

	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% Increments vert_offset,
	//%%%%%%%%%%%%%%%%%%  SETUP State (es_simple_eye_acq)  %%%%%%%%%%%%%%%%%% horz_offset and ut_sign
	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	if(eye_struct->state == SETUP_STATE) {  //(SETUP x0020 = 32)
		if( eye_struct->pixel_count >= NUM_PIXELS_TOTAL ) {
			eye_struct->state = DONE_STATE;
			return;
		}
		//Advance vert_offset (-max to max)
		eye_struct->vert_offset = eye_struct->vert_step_size + eye_struct->vert_offset;

		//If done scanning pixel column
		if(eye_struct->vert_offset > max_vert_offset) {
			//Reset to -max_vert_offset
			eye_struct->vert_offset = -max_vert_offset;

			//Advance(decrement) ut_sign
			eye_struct->ut_sign = eye_struct->ut_sign - 1;

			//If completed 1 & 0 for DFE (or just 0 for LPM), reset to max
			if(eye_struct->ut_sign < 0){
				eye_struct->ut_sign = max_ut_sign;

				//Advance horz_offset
				if(eye_struct->horz_offset < 0){
					eye_struct->horz_offset = -eye_struct->horz_offset;
				} else {
					//Increments thru sequence: 0, -step, step, -2*step, 2*step, -3*step, 3*step, ...
					eye_struct->horz_offset = -(eye_struct->horz_step_size + eye_struct->horz_offset);
				}

				//If incremented thru all pixel columns, reset to 0 and set to DONE state, which acts like WAIT state in this function
				if(eye_struct->horz_offset < -max_horz_offset){
					eye_struct->horz_offset = 0;
					eye_struct->state = DONE_STATE;
					return;
				}
			}
		}
		//Convert to 'phase_unification' + two's complement
		if(eye_struct->horz_offset < 0) {
			horz_value = eye_struct->horz_offset + 2048; //Generate 11-bit 2's complement number
			phase_unification = 2048; //12th bit i.e. bit[11]
		} else {
			horz_value = eye_struct->horz_offset;
			phase_unification = 0;
		}
		horz_value  = phase_unification + horz_value;
		//Write horizontal offset
		drp_write(horz_value, ES_HORZ_OFFSET,eye_struct->lane_number);

		//ES_VERT_OFFSET[8] is UT_sign
		vert_value = abs(eye_struct->vert_offset) + eye_struct->ut_sign * 256; //256 = 2^8 ut_sign is bit[8]
		if(eye_struct->vert_offset < 0) {
			vert_value = vert_value + 128;//128=2^7 sign is bit [7]
		}

		//Write vertical offset
		drp_write(vert_value, ES_VERT_OFFSET, eye_struct->lane_number);

		//Assert RUN bit to start error and sample counters
		drp_write(1, ES_CONTROL, eye_struct->lane_number);

		//Transition to 'COUNT' state, allow other operations while errors & samples accumulate
		eye_struct->state = COUNT_STATE;
		return;
	}

	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% When acquisition finished, reads
	//%%%%%%%%%%%%%%%%%%  COUNT State (es_simple_eye_acq)  %%%%%%%%%%%%%%%%%% error & sample counts and prescale.
	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% Adjusts prescale as needed.

	if(eye_struct->state == COUNT_STATE) { // (COUNT x0030 = 48)
		//If acquisition is not finished, return
		if(es_done == 0){
			return;
		}
		//If DONE, deassert RUN bit to transition to Eye Scan WAIT state
		drp_write(0, ES_CONTROL, eye_struct->lane_number);

		//Read error, sample counts & prescale used for acquisition
		error_count = drp_read(ES_ERROR_COUNT, eye_struct->lane_number);
		sample_count = drp_read(ES_SAMPLE_COUNT, eye_struct->lane_number);
		prescale = drp_read(ES_PRESCALE, eye_struct->lane_number);
		center_error = xaxi_eyescan_read_channel_reg(eye_struct->lane_number,XAXI_EYESCAN_MONITOR) & 0x80FF;
		if( DEBUG ) printf( "current prescale read from drp %d\n" , prescale );
		if(error_count < (10*min_error_count) || error_count > (1000*min_error_count)){
			if (prescale < max_prescale && error_count < 10*min_error_count){
				next_prescale = prescale+step_prescale;
				if(next_prescale > max_prescale) {
					next_prescale = max_prescale;
				}

				//Restart measurement if too few errors counted
				if (error_count < min_error_count){
					drp_write(next_prescale, ES_PRESCALE, eye_struct->lane_number);
					//Assert RUN bit to restart sample & error counters. Remain in COUNT state.
					drp_write(1, ES_CONTROL, eye_struct->lane_number);
					return;
				}
			}
			else if(prescale > 0 && error_count > 10*min_error_count){
				if (error_count > 1000*min_error_count){
					next_prescale = prescale - 2*step_prescale;
				} else {
					next_prescale = prescale - step_prescale;
				}

				if(next_prescale < 0){
					next_prescale = 0;
				}
			}else{
				next_prescale   = prescale;
			}
			drp_write(next_prescale,ES_PRESCALE, eye_struct->lane_number);
		}
		if( DEBUG ) printf( "lane %d pixel_count %d error_count %d sample_count %d prescale %d\n" , eye_struct->lane_number , eye_struct->pixel_count , error_count , sample_count , prescale );
		eye_struct->prescale = prescale;

		// Store information about current pixel
		eye_scan_pixel * current_pixel = ( eye_struct->pixels + eye_struct->pixel_count );
		current_pixel->error_count = error_count;
		current_pixel->sample_count = sample_count;
		current_pixel->h_offset = eye_struct->horz_offset;
		current_pixel->v_offset = eye_struct->vert_offset;
		current_pixel->center_error = center_error;
		current_pixel->ut_sign = eye_struct->ut_sign;
		current_pixel->prescale = eye_struct->prescale;
		eye_struct->pixel_count++;
		if( eye_struct->pixel_count % 10 == 0 ) {
			if( DEBUG ) printf( "lane %d at pixel %d\n" , eye_struct->lane_number , eye_struct->pixel_count );
		}

		//Transition to SETUP state
		eye_struct->state = SETUP_STATE;
		return;
	}
}
