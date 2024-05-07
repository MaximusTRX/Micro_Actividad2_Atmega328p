/*
 * actividad_2.c
 *
 * Created: 3/20/2024 4:41:47 PM
 * Author : Maximus
 */ 

#include <avr/io.h>
// #include <iom328p.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/common.h>

#include "HSCR04.h"

#define	LEDBUILDIN	5
#define	ECHO		0
#define TRIGGER		1

// **** GPIOR0 como regsitros de banderas ****
#define F10MS		0	// GPIOR0<0>:
#define F100MS		1	// GPIOR0<1>:
#define F10US		2	// GPIOR0<2>:
#define BTNACTUAL	3	// GPIOR0<3>:
#define BTNDOWN		4	// GPIOR0<4>:
//#define	5	// GPIOR0<5>:
//#define 	6	// GPIOR0<6>:
//#define 	7	// GPIOR0<7>:

// **** GPIOR1 como regsitros de banderas ****
//#define 	0	// GPIOR1<0>:
//#define 	1	// GPIOR1<1>:
//#define 	2	// GPIOR1<2>:
//#define 	3	// GPIOR1<3>:
#define ECHOSTATE	4	// GPIOR1<4>:	0 cuando se espera el flanco rising - 1 cuando se espera el falling
#define ECHOFINISH	5	// GPIOR1<5>:	1 cuando la medición termina
#define INIMED		6	// GPIOR1<6>:
#define FTRIGGER	7	// GPIOR1<7>:



// EEPROM
// eeprom_write_byte() para escribir en la eeprom
// eeprom_read_byte() para leer desde la eeprom

EEMEM uint8_t variable1;

//.

// Constantes en FLASH

//PROGMEM const uint8_t varFlash = 10;

//.

//========================== Constantes Globales son guardadas en la SRAM

uint8_t		tTRIGGER;		//!< Description: Tiempo entre inicio de medicion y medicion
uint8_t		tdebounce;
uint8_t		t100ms;
uint8_t		MASKHEARBEAT;
uint16_t	tEchoUP;			//!< Description: Guarda el valor del contador del flanco ascendente del ECHO
uint16_t	tEchoDOWN;			//!< Description: Guarda el valor del contador del flanco descendente del ECHO
uint8_t		lastMedicion;		//!< Description: Guarda el valor de la resta entre los dos anteriores
//


//========================== Declaración Cabeceras de Funciones
void ini_ports();
void ini_Timer1();
void ini_Externals();
void do_10ms();
void do_Medicion();
void do_BTNCHANGE();
void do_Transmit();

//.

//========================== Declaración Funciones de Interrupciones
ISR (TIMER1_COMPA_vect){
	//TCNT1	= 0x00;
	OCR1A	+= 20000;			// Equivale a 10ms
	GPIOR0	|= (1<<F10MS);
}

ISR (TIMER1_COMPB_vect){
	//TCNT1	= 0x00;
	OCR1B	+= 20;				// Equivale a 10us
	GPIOR0	|= (1<<F10US);
}

ISR (USART_RX_vect){
	
}


ISR (PCINT0_vect){
	if (!(GPIOR1 & (1<<ECHOSTATE)))
	{
		tEchoUP =	TCNT1;
		EICRA	=	(1<<ISC01);		//!< Description: Configura el flanco a detectar, falling.
		GPIOR1	|=	(1<<ECHOSTATE);
	}else{
		tEchoDOWN = TCNT1;
		GPIOR1 |= (1<<ECHOFINISH);
	}
}


//========================== Initialization Functions
void ini_ports(){
	DDRB	= (1<<LEDBUILDIN) | (1<<TRIGGER);
	PORTB	= (1<<PINB4) | (1<<ECHO);
}

void ini_Timer1(){
	TCCR1A	= 0x00;
	TCCR1B	= 0b00000010;	// PRESSCALER /8 -> +1 cada 500ns
	OCR1A	= 20000;		// Equivale a 10ms
	OCR1B	= 20;			// Equivale a 10us
	TIMSK1	= 0b00000110;
	TIFR1	= TIFR1;
}

void ini_USART0(){
	UCSR0A = UCSR0A;
	UCSR0A = (1<<U2X0);
	UCSR0B = (1<<TXEN0); //(1<<RXCIE0) | (1<<RXEN0) | 
	UCSR0C = (1<< UCSZ01) | (1<< UCSZ00);
	UBRR0  = 0x10;
	//UBRR0H = (16>>8);
	//UBRR0L = 16; 
}

void ini_Externals(){
	EIMSK	= (1<<INT0);			//!< Description: Habilita Interrupción general de los pines INT0
	PCMSK0	= (1<<PCINT0);			//!< Description: Seleccióna el pin que se controla el cambio para generar la interrupt
	PCICR	|= (1<<PCIE0);			//!< Description: Hablilita la interrupción por cambio de flanco
}

//===================================== Functions
void do_10ms(){
	GPIOR0 &= ~(1<<F10MS);
	
	t100ms --;
	if(!t100ms)
	{
		t100ms = 50;
		GPIOR0 |= (1<<F100MS);
	}
	
	if (GPIOR0 & (1<<BTNACTUAL)){	// Si el btn está press
		if (tdebounce){				// Entra cuando es <> 0
			tdebounce --;
		}else{
			GPIOR0 |= (1<<BTNDOWN);
		}
	}else{
		tdebounce = 10;
	}
}

void do_Medicion(){
	GPIOR1 &= ~(1<<INIMED);
	GPIOR1	|= (1<<FTRIGGER);
	GPIOR1	&= ~(1<<ECHOSTATE);
	GPIOR1	&= ~(1<<ECHOFINISH);
	//PCICR	|= (1<<PCIE0);				//!< Description: Hablilita la interrupción por cambio de flanco
	EICRA	|= (1<<ISC01) | (1<<ISC00); //!< Description: Configura el flanco a detectar. rising = 1-1. falling = 1-0
}

void do_Transmit(){
	if (UCSR0A & (1<<UDRE0)){
		UDR0 = lastMedicion;					//!< Description: UDR0 es el registro que se carga para mandar los datos
		//UCSR0A &= ~(1<<UDRE0);
	}
}

void do_BTNCHANGE(){
	PORTB ^= (1<<LEDBUILDIN);
	//lastMedicion = 180;
}

//.



int main(void)
{
	// Ejemplo
	// GPIOR0 |= (1<<IS10MS) ES LO MISMO GPIOR0 |= _BV(IS10MS);
	// GPIOR0 |= (1<<5);  PONE SOLO El BIT 5 DEL REGISTRO EN 1
	// GPIOR0 &= ~(1<<5); PONE SOLO EL BIT 5 DEL REGISTRO EN 0
	
	// El START de assembler
	cli();
	ini_ports();
	ini_Timer1();
	ini_USART0();
	ini_Externals();
	sei();
	tTRIGGER = 147;
	t100ms = 30;
	tdebounce = 10;
	MASKHEARBEAT = 58;
	HSCR04_Start();
	
    while (1) 
    {
		if (GPIOR0 & (1<<F10US))
		{
			GPIOR0 &= ~(1<<F10US);
			tTRIGGER --;
			if(!tTRIGGER)
			{
				tTRIGGER = 147;
				GPIOR1 |= (1<<INIMED);
			}
			if (GPIOR1 & (1<<FTRIGGER)){
				if (PINB & (1<<TRIGGER)){		//!< Description: Si el trigger está en 1 pongo la flag en 0
					GPIOR1 &= ~(1<<FTRIGGER);	//!< Description: Porque no necesito hacer toggle de nuevo
				}
				PORTB ^= (1<<TRIGGER);			//!< Description: Toggle del pin de trigger
			}
		}
		
		if (GPIOR0 & (1<<F10MS)){
			do_10ms();
		}
		
		if(GPIOR0 & (1<<F100MS)){
			GPIOR0 &= ~(1<<F100MS);
			do_Transmit();
		}
		
		
		if(GPIOR1 & (1<<INIMED)){
			do_Medicion();
		}
		
		//============================CAPTURA DE ECHO
		/*
		if (PINB & (1<<ECHO))
		{
			if (!(GPIOR0 & (1<<ECHOFLAG))){			//!< Description: Entro solamente una vez para tomar el valor de tiempo
				tEchoUP = TCNT1;
			}
			
			GPIOR0 |= (1<<ECHOFLAG);
			
		}else if (GPIOR0 & (1<<ECHOFLAG))
		{		//!< Description: Detecto el flanco al saber que el pin ECHO = 0 y sí la bandera estaba en 1 significa que hubo un cambio de flanco
			tEchoDOWN = TCNT1;
			lastMedicion = (tEchoDOWN - tEchoUP)/58;
			GPIOR0 &= ~(1<<ECHOFLAG);
			
		}
		*/
		if (GPIOR1 & (1<<ECHOFINISH))
		{
			//PCICR	= 0x00;							//!< Description: Deshabilito la interrupción por cambio de flanco.
			lastMedicion = (tEchoDOWN - tEchoUP)/58;
		}
		
		//=============================CAPTURA DE BOTON
		if(!(PINB & (1<<PINB4))){			//!< Description: Actualiza la bandera de estado del btn
			GPIOR0 |= (1<<BTNACTUAL);		// Si el btn press == 1
		} else {
			GPIOR0 &= ~(1<<BTNACTUAL);		// Si el btn no press == 0
		}
		
		if (GPIOR0 & (1<<BTNDOWN)){				//!< Description: Una vez presionado el boton
			if (!(GPIOR0 & (1<<BTNACTUAL))){	//!< Description: Espera a que se suelte para ir a la funcion
				GPIOR0 &= ~(1<<BTNDOWN);
				do_BTNCHANGE();	
			}		
		}
    }
}

/*
en el .c
void (*ptrFun) (uint16_t value);

en el .h se la llama como
ptrFun(8);

el usuario define
void prtFunUser (uint16_t);
*/
