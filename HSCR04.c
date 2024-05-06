/*
 * HSCR04.c
 *
 * Created: 3/27/2024 4:18:11 PM
 *  Author: Maximus
 */ 


#include "HSCR04.h"
#include <stdlib.h>

static _sHSCR04Handle myHandle = {.WritePin = NULL, .ReadPin = NULL};

static uint16_t time1us = 0;
static uint8_t flags	= 0;
	
void HSCR04_Init(_sHSCR04Handle aHandle){
	myHandle.WritePin = aHandle.WritePin;
	myHandle.ReadPin = aHandle.ReadPin;
	
	time1us = 0;
}

void HSCR04_Time1us(){
	time1us++;
}

void HSCR04_Start(){
	time1us = 0;
	
	if (myHandle.WritePin!=NULL){
		flags |= 0x01;
	}
}

void HSCR04_Stop(){
	flags ^= ~0x01;
}

void HSCR04_Task(){
	
}