/*
  laser.cpp - Laser control library for Arduino using 16 bit timers- Version 1
  Copyright (c) 2013 Timothy Schmidt.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "laser.h"
#include "Configuration.h"
#include "ConfigurationStore.h"
#include "pins.h"
#include <avr/interrupt.h>
#include <Arduino.h>
#include "Marlin.h"

laser_t laser;

void laser_init()
{
  pinMode(LASER_FIRING_PIN, OUTPUT);

  #if LASER_CONTROL == 2
    pinMode(LASER_INTENSITY_PIN, OUTPUT);
    analogWrite(LASER_INTENSITY_PIN, 1);  // let Arduino setup do it's thing to the PWM pin

    TCCR4B = 0x00;  // stop Timer4 clock for register updates
    TCCR4A = 0x82; // Clear OC4A on match, fast PWM mode, lower WGM4x=14
    ICR4 = labs(F_CPU / LASER_PWM); // clock cycles per PWM pulse
    OCR4A = labs(F_CPU / LASER_PWM) - 1; // ICR4 - 1 force immediate compare on next tick
    TCCR4B = 0x18 | 0x01; // upper WGM4x = 14, clock sel = prescaler, start running

    noInterrupts();
    TCCR4B &= 0xf8; // stop timer, OC4A may be active now
    TCNT4 = labs(F_CPU / LASER_PWM); // force immediate compare on next tick
    ICR4 = labs(F_CPU / LASER_PWM); // set new PWM period
    TCCR4B |= 0x01; // start the timer with proper prescaler value
    interrupts();
    
  #endif

  #if LASER_CONTROL == 3
    pinMode(LASER_POWER_PIN, OUTPUT);
    analogWrite(LASER_FIRING_PIN, 1);  // let Arduino setup do it's thing to the PWM pin

    TCCR4B = 0x00;  // stop Timer4 clock for register updates
    TCCR4A = 0x82; // Clear OC4A on match, fast PWM mode, lower WGM4x=14
    ICR4 = labs(F_CPU / LASER_PWM); // clock cycles per PWM pulse
    OCR4A = labs(F_CPU / LASER_PWM) - 1; // ICR4 - 1 force immediate compare on next tick
    TCCR4B = 0x18 | 0x01; // upper WGM4x = 14, clock sel = prescaler, start running

    noInterrupts();
    TCCR4B &= 0xf8; // stop timer, OC4A may be active now
    TCNT4 = labs(F_CPU / LASER_PWM); // force immediate compare on next tick
    ICR4 = labs(F_CPU / LASER_PWM); // set new PWM period
    TCCR4B |= 0x01; // start the timer with proper prescaler value
    interrupts();
  
  #endif

  #ifdef LASER_PERIPHERALS
  digitalWrite(LASER_PERIPHERALS_PIN, HIGH);  // Laser peripherals are active LOW, so preset the pin
  pinMode(LASER_PERIPHERALS_PIN, OUTPUT);

  digitalWrite(LASER_PERIPHERALS_STATUS_PIN, HIGH);  // Set the peripherals status pin to pull-up.
  pinMode(LASER_PERIPHERALS_STATUS_PIN, INPUT_PULLUP);
  #endif // LASER_PERIPHERALS

  // initialize state to some sane defaults
  laser.intensity = 100.0;
  laser.ppm = 0;
  laser.duration = 0;
  laser.status = LASER_OFF;
  laser.firing = LASER_OFF;
  laser.mode = CONTINUOUS;
  laser.last_firing = 0;
  laser.time = 0;
  #ifdef LASER_RASTER
    laser.raster_aspect_ratio = LASER_RASTER_ASPECT_RATIO;
    laser.raster_mm_per_pulse = LASER_RASTER_MM_PER_PULSE;
    laser.raster_direction = 1;
  #endif // LASER_RASTER
  #ifdef MUVE_Z_PEEL
    laser.peel_distance = 2.0;
    laser.peel_speed = 2.0;
    laser.peel_pause = 0.0;
  #endif // MUVE_Z_PEEL
}

#if LASER_CONTROL == 1
unsigned long calc_laser_intensity(float intensity) {
  if (intensity > 100.0) intensity = 100.0; // restrict intensity between 0 and 255
  if (intensity < 0.0) intensity = 0.0;
  return labs(intensity*2.55);
}
#else
unsigned long calc_laser_intensity(float intensity) {
  if (intensity > 100.0) intensity = 100.0; // restrict intensity between 0 and 255
  if (intensity < 0.0) intensity = 0.0;
  return labs((intensity / 100.0)*(F_CPU / LASER_PWM));
}
#endif

#ifdef LASER_PERIPHERALS
bool laser_peripherals_ok() {
	return !digitalRead(LASER_PERIPHERALS_STATUS_PIN);
}

void laser_peripherals_on() {
	digitalWrite(LASER_PERIPHERALS_PIN, LOW);
  #if LASER_DIAGNOSTICS
	  SERIAL_ECHO_START;
	  SERIAL_ECHOLNPGM("Laser Peripherals Enabled");
  #endif
}

void laser_peripherals_off() {
	if (!digitalRead(LASER_PERIPHERALS_STATUS_PIN)) {
	  digitalWrite(LASER_PERIPHERALS_PIN, HIGH);
    #if LASER_DIAGNOSTICS
	    SERIAL_ECHO_START;
	    SERIAL_ECHOLNPGM("Laser Peripherals Disabled");
    #endif
  }
}

void laser_wait_for_peripherals() {
	unsigned long timeout = millis() + LASER_PERIPHERALS_TIMEOUT;
  #if LASER_DIAGNOSTICS
	  SERIAL_ECHO_START;
	  SERIAL_ECHOLNPGM("Waiting for peripheral control board signal...");
	#endif
	while(!laser_peripherals_ok()) {
		if (millis() > timeout) {
      #if LASER_DIAGNOSTICS
			  SERIAL_ERROR_START;
			  SERIAL_ERRORLNPGM("Peripheral control board failed to respond");
			#endif
			Stop();
			break;
		}
	}
}
#endif // LASER_PERIPHERALS
