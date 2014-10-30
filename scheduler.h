#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <Arduino.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
 
/* Task properties */
typedef struct _task {
	/*Tasks should have members that include: state, period,
		a measurement of elapsed time, and a function pointer.*/
	signed char state; //Task's current state
	unsigned long int period; //Task period
	unsigned long int elapsedTime; //Time elapsed since last task tick
	int (*TickFct)(int); //Task tick function
} task;

unsigned long TICK_PERIOD = 1;

unsigned char NUM_TASKS = 0;

bool schedLock = false;
 
/*
 * The task array.
 * This dictates the tasks to be run from the scheduler
 * The order of these tasks sets their priority, should more than one task run in one tick
 * */
task* tasks; 
 
void sleepNow()
{
 
    set_sleep_mode(SLEEP_MODE_IDLE);                                    /* sleep mode is set here */
 
    sleep_enable();                                                     /* enables the sleep bit in the mcucr register */
                                                                        /* so sleep is possible. just a safety pin */
 
    power_adc_disable();                                                /* Power down some things to save power */
    power_spi_disable();
    power_timer0_disable();
    power_twi_disable();
 
    schedLock = true;                                                   /* Prevent schedule from running on accidental wake-up */
    sleep_mode();                                                       /* here the device is actually put to sleep!! */
                                                                        /* THE PROGRAM IS WOKEN BY TIMER1 ISR */
}
 
/*
 * Start the timer interrupts
 */
void tick_Start()
{
    /* initialize Timer1 */
    noInterrupts();          /* disable global interrupts */
    TCCR1A = 0;     /* set entire TCCR1A register to 0 */
    TCCR1B = 0;     /* same for TCCR1B */
 
    /* set compare match register to desired timer count: */
    OCR1A = (16 * TICK_PERIOD); /* TICK_PERIOD is in microseconds */
    /* turn on CTC mode: */
    TCCR1B |= (1 << WGM12);
    /* enable timer compare interrupt: */
    TIMSK1 |= (1 << OCIE1A);
    TCCR1B |= (1 << CS10);
    interrupts(); /* enable global interrupts (start the timer)*/
}
 
 // Set TimerISR() to tick every m ms
void TimerSet(unsigned long m) {
	TICK_PERIOD = m;
}
 
/*
 * The ISR runs periodically every TICK_PERIOD
 */
ISR(TIMER1_COMPA_vect)
{
 
    sleep_disable();        /* disable sleep */
    power_all_enable();     /* restore all power */
    schedLock = false;      /* allow scheduler to run */
 
   static unsigned char i;
   if(schedLock == false){
      for (i = 0; i < NUM_TASKS; i++) { 
        if ( tasks[i].elapsedTime >= tasks[i].period ) { // Ready
            tasks[i].state = tasks[i].TickFct(tasks[i].state);
            tasks[i].elapsedTime = 0;
        }
        tasks[i].elapsedTime += TICK_PERIOD;
      }
   }
}
 
 
#endif //SCHEDULER_H