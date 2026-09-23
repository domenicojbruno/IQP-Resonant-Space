// Motor.h — one BTS7960-driven linear actuator with a single shared limit switch.
//
// The actuators have no encoders, so position is time-based: elapsed drive time
// divided by the motor's full-travel time (measured by CAL, persisted to flash).
// Hitting the limit switch re-syncs the estimate to 0 % or 100 %.
#pragma once

#include <stdint.h>
#include "Protocol.h"

class Motor {
 public:
  void begin(uint8_t index, int8_t pinRpwm, int8_t pinLpwm, int8_t pinLimit,
             bool inverted, uint32_t travelMs);

  // Call every loop iteration.
  void update(uint32_t now);

  // Apply a protocol command. Returns false if the value is out of range.
  bool command(uint8_t cmd, uint8_t value, uint32_t now);

  MotorState state() const { return state_; }
  FaultCode fault() const { return fault_; }
  uint8_t posPct() const;
  bool driving() const { return dir_ != 0; }
  bool hasLimitSwitch() const { return pinLimit_ >= 0; }
  uint32_t travelMs() const { return travelMs_; }

  // True once after CAL finishes; the caller persists travelMs().
  bool takeNewCalibration();

 private:
  enum AfterHome : uint8_t { AFTER_IDLE, AFTER_CAL };

  void drive(int8_t dir, uint8_t pwm, uint32_t now);
  void halt();
  void startHoming(AfterHome after, uint32_t now);
  void enterSeek(uint32_t now);
  void setFault(FaultCode f);
  void readLimit(uint32_t now);
  uint32_t runawayMs() const;

  uint8_t index_ = 0;
  int8_t pinFwd_ = -1;    // extend
  int8_t pinRev_ = -1;    // retract
  int8_t pinLimit_ = -1;

  MotorState state_ = ST_UNINIT;
  FaultCode fault_ = FAULT_NONE;

  int8_t dir_ = 0;        // +1 extend, -1 retract, 0 stopped
  uint8_t pwm_ = 0;
  uint32_t phaseStart_ = 0;
  uint32_t lastUpdate_ = 0;

  bool posKnown_ = false;
  float pos_ = 0.0f;      // 0 = retracted, 1 = extended
  int16_t gotoTarget_ = -1;  // 0-100 while a GOTO is running, else -1

  uint32_t travelMs_ = 0;
  bool newCalibration_ = false;

  AfterHome afterHome_ = AFTER_IDLE;
  int8_t clearDir_ = 0;

  // Limit switch (debounced). A move that starts on a pressed switch ignores it
  // until it releases ("armed"), since the same switch marks both ends.
  bool limitRaw_ = false;
  bool limitPressed_ = false;
  uint32_t limitChangedAt_ = 0;
  bool limitArmed_ = true;
};
