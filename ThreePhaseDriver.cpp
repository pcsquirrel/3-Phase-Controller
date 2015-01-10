/* 
 * File:   ThreePhaseDriver.cpp
 * Author: Cameron
 * 
 * Created on December 17, 2014, 3:20 PM
 */

#include <avr/pgmspace.h>

#include "ThreePhaseDriver.h"
#include "Board.h"
#include "Debug.h"

/**
 * We need an extra zero register that REALLY stays as zero.
 * 
 * The __zero_reg__ from avr-gcc can be clobbered by multiplication
 */
register u1 ZERO asm("r2");

// Reserved AVR registers for timer operation so we don't need to waste time
// with context switching and such.
register u1 cacheA asm("r3");
register u1 cacheB asm("r4");
register u1 cacheC asm("r5");
register u1 cacheM asm("r6");

//u1 ThreePhaseDriver::cacheA = 0;
//u1 ThreePhaseDriver::cacheB = 0;
//u1 ThreePhaseDriver::cacheC = 0;

/**
 * The overflow vector. We need to:
 *  - clear the interrupt flag of the next interrupt fast
 *  - update our OCRnx values
 *  - enable the next interrupt
 *  - return
 * 
 * None of these instructions modify any register or SREG (besides interrupt)
 */
extern "C" void TIMER1_OVF_vect() __attribute__ ((naked,__INTR_ATTRS));

void TIMER1_OVF_vect() {
 // clear timer match interrupts
 TIFR1 = cacheM; // asm ("out	0x16, r5" : : );

 OCR1AL = cacheA; // asm ("sts	0x0088, r2" : : );
 OCR1BL = cacheB; // asm ("sts	0x008A, r3" : : );
 OCR1CL = cacheC; // asm ("sts	0x008C, r4" : : );
 
 // Enable next interrupt
 TIMSK1 = cacheM; // asm ("sts	0x006F, r5" : : );
 
 // Manual interrupt return
 asm ("reti" : : );
}

// Save some grief later
#ifndef TIMER1_OVF_vect
#error Include <avr/io.h>!
#endif

/**
 * Interrupt that turns off B (low) and turns on C (low)
 * 
 * Happens at the end of Phase::B, beginning of Phase::C
 */
extern "C" void TIMER1_COMPA_vect() __attribute__ ((naked,__INTR_ATTRS));

void TIMER1_COMPA_vect() {
 // B low off
// Board::DRV::BL.off();
 PORTF &= ~(1 << 0);
 
 // C low on
// Board::DRV::CL.on();
 PORTB |=  (1 << 0);
 
 // Disable timer interrupts
 TIMSK1 = ZERO;
 
 // Manual interrupt return
 asm ("reti" : : );
}

/**
 * Interrupt that turns off C (low) and turns on A (low)
 * 
 * Happens at the end of Phase::C, beginning of Phase::A
 */
extern "C" void TIMER1_COMPB_vect() __attribute__ ((naked,__INTR_ATTRS));

void TIMER1_COMPB_vect() {
 // C low off
// Board::DRV::CL.off();
 PORTB &= ~(1 << 0);
 
 // A low on
// Board::DRV::AL.on();
 PORTB |=  (1 << 4);
 
 // Disable timer interrupts
 TIMSK1 = ZERO;
 
 // Manual interrupt return
 asm ("reti" : : );
}

/**
 * Interrupt that turns off A (low) and turns on B (low)
 * 
 * Happens at the end of Phase::A, beginning of Phase::B
 */
extern "C" void TIMER1_COMPC_vect() __attribute__ ((naked,__INTR_ATTRS));

void TIMER1_COMPC_vect() {
 // A low off
// Board::DRV::AL.off();
 PORTB &= ~(1 << 4);
 
 // B low on
// Board::DRV::BL.on();
 PORTF |=  (1 << 0);
 
 // Disable timer interrupts
 TIMSK1 = ZERO;
 
 // Manual interrupt return
 asm ("reti" : : );
}

void ThreePhaseDriver::init() {
 // Gotta do it somewhere...
 ZERO = 0;
 
 // Turn off interrupts just in case
 TIMSK1 = 0;
 // And clear them all (just the ones we know about) just in case
 TIFR1 = 0b00101111;
 
 // Clear compare match registers for now
 OCR1A = 0;
 OCR1B = 0;
 OCR1C = 0;
 
 /* 
  * Set WGM for PWM Phase Correct 8-bit
  * WGM = 0b0001
  * 
  * Set all compare output modules to:
  * Clear OC1[ABC] on compare match when up-counting.
  * Set OC1[ABC] on compare match when down-counting.
  * COM1[ABC] = 0b10
  * 
  * This makes the OCR1[ABC] registers reflect the output's "high time" per cycle
  * 
  * Select the fastest clock divider ( clkIO/1 )
  * 
  * CS1 = 0b001;
  */
 
 /*
  * WGM10:  1 -  
  * WGM11:  0 - 
  * COM1C0: 0 - 
  * COM1C1: 1 - 
  * COM1B0: 0 - 
  * COM1B1: 1 - 
  * COM1A0: 0 - 
  * COM1A1: 1 - 
  * 
  * CS10:  1 - 
  * CS11:  0 - 
  * CS12:  0 - 
  * WGM12: 0 - 
  * WGM13: 0 - 
  * ~~~~~: 0 - 
  * ICES1: 0 - 
  * ICNC1: 0 - 
  */
 
 TCCR1A = 0b10101001;
 TCCR1B = 0b00000001;
 
 // Turn everything off
 Board::DRV::AL.off();
 Board::DRV::BL.off();
 Board::DRV::CL.off();
 Board::DRV::AH.off();
 Board::DRV::BH.off();
 Board::DRV::CH.off();
 
 // Enable outputs
 Board::DRV::AL.output();
 Board::DRV::BL.output();
 Board::DRV::CL.output();
 Board::DRV::AH.output();
 Board::DRV::BH.output();
 Board::DRV::CH.output();
}

static const u1 limitedSinTable[ThreePhaseDriver::StepsPerPhase] PROGMEM = {
   0,  2,  4,  6,  8, 10, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31,
  33, 35, 37, 39, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64,
  66, 68, 70, 72, 74, 76, 78, 80, 82, 84, 86, 88, 90, 92, 94, 96,
  98,100,101,103,105,107,109,111,113,115,117,118,120,122,124,126,
 128,129,131,133,135,136,138,140,142,143,145,147,149,150,152,154,
 155,157,159,160,162,163,165,167,168,170,171,173,174,176,177,179,
 180,182,183,185,186,188,189,190,192,193,194,196,197,198,200,201,
 202,204,205,206,207,208,210,211,212,213,214,215,217,218,219,220,
 221,222,223,224,225,226,227,228,229,230,231,231,232,233,234,235,
 236,236,237,238,239,239,240,241,241,242,243,243,244,245,245,246,
 246,247,247,248,248,249,249,250,250,250,251,251,252,252,252,253,
 253,253,253,254,254,254,254,254,254,255,255,255,255,255,255,255,
 255,255,255,255,255,255,255,255,254,254,254,254,254,254,253,253,
 253,253,252,252,252,251,251,250,250,250,249,249,248,248,247,247,
 246,246,245,245,244,243,243,242,241,241,240,239,239,238,237,236,
 236,235,234,233,232,231,231,230,229,228,227,226,225,224,223,222,
};

u1 ThreePhaseDriver::amplitude = 0;

u1 ThreePhaseDriver::getPhasePWM(const u1 step) {
// u1 const sin = MAX * SIN(2 * PI * step / StepsPerCycle);
 u1 const sin = pgm_read_byte(&limitedSinTable[step]);
 
 // TODO: This product (and subsequent truncation) does not fully cover the
 // range of the return u1. Ideally, instead of dividing by 256 (>> 8) we should
 // be dividing by 255. We can get closer, on average, to that ideal division
 // if we add 188 for instance.
 u2 const prod = sin * amplitude + sin + amplitude;
 
 return prod >> 8;
}

ThreePhaseDriver::Phase ThreePhaseDriver::currentPhase = Phase::C;

void ThreePhaseDriver::advanceTo(const Phase phase, const u1 step) {
 u1 const ONE = getPhasePWM(    step);
 u1 const TWO = getPhasePWM(255-step);
 
 if (phase == Phase::A) {
  cacheA = 0;
  cacheB = TWO;
  cacheC = ONE;
  if (currentPhase == Phase::C)
   cacheM = 1 << OCF1B;
 } else if (phase == Phase::B) {
  cacheA = ONE;
  cacheB = 0;
  cacheC = TWO;
  if (currentPhase == Phase::A)
   cacheM = 1 << OCF1C;
 } else if (phase == Phase::C) {
  cacheA = TWO;
  cacheB = ONE;
  cacheC = 0;
  if (currentPhase == Phase::B)
   cacheM = 1 << OCF1A;
 } else {
  cacheA = 0;
  cacheB = 0;
  cacheC = 0;
  cacheM = 0;
  return;
  
  // TODO: Not fully reset. Any low could be high right now
 }
 
 // Enable the overflow interrupt which will do the rest of this stuff for us
 TIMSK1 = 1;

 currentPhase = phase;
 
 // BIG TODO: IF WE'RE CHANGING PHASES, THE OLD VALUES, TWO must be less than ONE.
 // Otherwise, the interrupt would happen at the wrong time, and we could blow things up.
 // This would only happen if we're spinning fast.
}

void ThreePhaseDriver::advance() {
 static u2 step = 0;
 advanceTo(step);
 Debug::reportByte(step >> 2);
 if (++step == 0x300) step = 0;
}
