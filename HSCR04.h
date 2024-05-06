/*
 * HSCR04.h
 *
 * Created: 3/27/2024 4:16:54 PM
 *  Author: Maximus
 */ 

#ifndef HSCR04_H_
#define HSCR04_H_
#include <stdint.h>

typedef struct{
	void(*WritePin)(uint8_t value);
	void (*ReadPin)(void);
}_sHSCR04Handle;

// =================================
void HSCR04_Init(_sHSCR04Handle aHandle);

uint16_t HSCR04_Read();		//!< Description: Funcion que al llamar tengo que dar la última medicion

void HSCR04_Time1us();		//!< Description: Se llama cada 1us cuando está iniciada la medicion

void HSCR04_Task();			//!< Description: 

void HSCR04_Start();		//!< Description: Inicio el proceso de medir

void HSCR04_Stop();			//!< Description: Detiene la espera del echo



#endif /* HSCR04_H_ */