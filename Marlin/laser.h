/*
  laser.h - Laser cutter control library for Arduino using 16 bit timers- Version 1
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

#ifndef LASER_H
#define LASER_H

#define LASER_DIAGNOSTICS FALSE

#include <inttypes.h>
#include "Configuration.h"

typedef struct {
  int fired; // method used to ask the laser to fire - LASER_FIRE_G1, LASER_FIRE_SPINDLE, LASER_FIRE_E, etc
  float intensity; // Laser firing instensity 0.0 - 100.0
  float ppm; // pulses per millimeter, for pulsed firing mode
  unsigned long duration; // laser firing duration in microseconds, for pulsed firing mode
  bool status; // LASER_ON / LASER_OFF - buffered
  bool firing; // LASER_ON / LASER_OFF - instantaneous
  uint8_t mode; // CONTINUOUS, PULSED, RASTER
  unsigned long last_firing; // microseconds since last laser firing
  unsigned int time; // temporary counter to limit eeprom writes
  unsigned int lifetime; // laser lifetime firing counter in minutes
  unsigned long micron_counter; // number of microns since last fire
  unsigned long microns_per_pulse; // number of microns per one pulse
  unsigned int micron_inc_diagonal; // distance increment equivalent to one step in X and Y (sqrt(X^2+Y^2))
  unsigned int micron_inc_x; // distance increment equivalent to one step in X
  unsigned int micron_inc_y; // distance increment equivalent to one step in Y
  unsigned long pulse_ticks; // duration of one pulse in Timer1 ticks
  unsigned long time_counter; // counts the time the laser has been on in Timer1 ticks
  #ifdef LASER_RASTER
    char raster_data[LASER_MAX_RASTER_LINE];
    float raster_aspect_ratio;
    float raster_mm_per_pulse;
    int raster_raw_length;
    int raster_num_pixels;
    bool raster_direction;
  #endif // LASER_RASTER
  #ifdef MUVE_Z_PEEL
    float peel_distance;
    float peel_speed;
    float peel_pause;
  #endif // MUVE_Z_PEEL
} laser_t;

extern laser_t laser;

void laser_init();
void laser_pulse_init();
unsigned long calc_laser_intensity(float intensity);
void laser_fire(unsigned long intensity);
void laser_fire_raster(int intensity);
void laser_extinguish();
void laser_update_lifetime();
#ifdef LASER_PERIPHERALS
  bool laser_peripherals_ok();
  void laser_peripherals_on();
  void laser_peripherals_off();
  void laser_wait_for_peripherals();
#endif // LASER_PERIPHERALS

// Laser constants
#define LASER_OFF 0
#define LASER_ON 1

#define CONTINUOUS 0
#define PULSED 1
#define RASTER 2

FORCE_INLINE void laser_fire(unsigned long intensity){
  static unsigned long prev_intensity;

  if (laser.firing == LASER_OFF) {
    laser.firing = LASER_ON;
    #if LASER_CONTROL == 1
      analogWrite(LASER_FIRING_PIN, intensity);             
    #elif LASER_CONTROL == 2
      analogWrite(LASER_INTENSITY_PIN, intensity);
      WRITE(LASER_FIRING_PIN, HIGH);
    #elif LASER_CONTROL == 3
      WRITE(LASER_POWER_PIN, HIGH);
      analogWrite(LASER_FIRING_PIN, intensity);
    #endif

    #if LASER_DIAGNOSTICS
      SERIAL_ECHOLN("*F");
    #endif
  }
}

FORCE_INLINE void laser_fire_raster(int intensity = 100.0) {
  laser.firing = LASER_ON;
  laser.last_firing = micros(); // microseconds of last laser firing
  if (intensity > 100.0) intensity = 100.0; // restrict intensity between 0 and 100
  if (intensity < 0) intensity = 0;
  #if LASER_CONTROL == 1
    analogWrite(LASER_FIRING_PIN, (intensity*2.55));             
  #endif
  #if LASER_CONTROL == 2
    analogWrite(LASER_INTENSITY_PIN, labs((intensity / 100.0)*(F_CPU / LASER_PWM)));
    digitalWrite(LASER_FIRING_PIN, HIGH);
  #endif
  #if LASER_CONTROL == 3
    digitalWrite(LASER_POWER_PIN, (intensity*2.55));
    analogWrite(LASER_FIRING_PIN, labs((intensity / 100.0)*(F_CPU / LASER_PWM)));
  #endif

  #if LASER_DIAGNOSTICS
    SERIAL_ECHOLN("Laser fired");
  #endif
}

FORCE_INLINE void laser_extinguish() {
  if (laser.firing == LASER_ON) {
    laser.firing = LASER_OFF;
    digitalWrite(LASER_FIRING_PIN, LOW);
    #if LASER_CONTROL == 3
    digitalWrite(LASER_POWER_PIN, 0);
    #endif

    #if LASER_DIAGNOSTICS
      SERIAL_ECHOLN("*E");
    #endif
  }
}

#endif // LASER_H
