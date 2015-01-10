/* 
 * File:   main.cpp
 * Author: btacklind
 *
 * Created on Nov 28, 2014, 2:02 PM
 */

#include <util/delay.h>
#include "Board.h"

#include "ThreePhaseDriver.h"
#include "MLX90363.h"
#include "Debug.h"

/**
 * All the init functions should go in here.
 * gcc will automatically call it for us because of the constructor attribute.
 */
void init() __attribute__((constructor));

void init() {
 // Make sure the ^SS pin is a driven output BEFORE initializing the SPI hardware.
 // The AVR's ^SS pin is really Board::DRV::AL, controlled by ThreePhaseDriver.
 ThreePhaseDriver::init();
 MLX90363::init();
 
 Debug::init();
}

/*
 * 
 */
void main() {
 
 _delay_ms(100);
 
 sei();
 
 Board::SEN::BS.on();
 Board::SEN::BS.output();
 Board::SEN::BS.off();
 
 ThreePhaseDriver::setAmplitude(20);

 while (1) {
  _delay_us(100);
  ThreePhaseDriver::advance();
 }
 
 u1 num = 0;
 
 do {
  MLX90363::sendGET1Message(MLX90363::Marker::AlphaBeta);
  num = MLX90363::num;
  
  Debug::LED.on();
  _delay_us(num);
  Debug::LED.off();
  _delay_ms(1);
  
  while (MLX90363::isTransmitting());
 } while (MLX90363::handleResponse());
 
// Debug::LED.on();

 while(1);
}

