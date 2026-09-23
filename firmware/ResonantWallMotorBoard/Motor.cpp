// Motor.cpp — see Motor.h.
#include "Motor.h"

#include <Arduino.h>
#include "config.h"

void Motor::begin(uint8_t index, int8_t pinRpwm, int8_t pinLpwm, int8_t pinLimit,
                  bool inverted, uint32_t travelMs) {
  index_ = index;
  pinFwd_ = inverted ? pinLpwm : pinRpwm;
  pinRev_ = inverted ? pinRpwm : pinLpwm;
  pinLimit_ = pinLimit;
  travelMs_ = travelMs ? travelMs : DEFAULT_TRAVEL_MS;

  ledcAttach(pinFwd_, PWM_FREQ_HZ, PWM_RES_BITS);
  ledcAttach(pinRev_, PWM_FREQ_HZ, PWM_RES_BITS);
  ledcWrite(pinFwd_, 0);
  ledcWrite(pinRev_, 0);

  if (pinLimit_ >= 0) {
    pinMode(pinLimit_, INPUT_PULLUP);
    limitRaw_ = limitPressed_ = (digitalRead(pinLimit_) == LOW);
  }
  lastUpdate_ = millis();
  limitChangedAt_ = lastUpdate_;
  state_ = ST_UNINIT;
}

uint8_t Motor::posPct() const {
  if (!posKnown_) return POS_UNKNOWN;
  return (uint8_t)(pos_ * 100.0f + 0.5f);
}

bool Motor::takeNewCalibration() {
  bool n = newCalibration_;
  newCalibration_ = false;
  return n;
}

uint32_t Motor::runawayMs() const {
  return (uint32_t)(travelMs_ * RUNAWAY_FACTOR);
}

void Motor::drive(int8_t dir, uint8_t pwm, uint32_t now) {
  // Starting on a pressed switch: it marks the end we are leaving, so ignore it
  // until it releases.
  limitArmed_ = !limitPressed_;
  dir_ = dir;
  pwm_ = pwm;
  phaseStart_ = now;
  ledcWrite(dir > 0 ? pinRev_ : pinFwd_, 0);
  ledcWrite(dir > 0 ? pinFwd_ : pinRev_, pwm);
}

void Motor::halt() {
  dir_ = 0;
  pwm_ = 0;
  gotoTarget_ = -1;
  ledcWrite(pinFwd_, 0);
  ledcWrite(pinRev_, 0);
}

void Motor::setFault(FaultCode f) {
  halt();
  posKnown_ = false;
  fault_ = f;
  state_ = ST_FAULT;
}

void Motor::startHoming(AfterHome after, uint32_t now) {
  fault_ = FAULT_NONE;
  afterHome_ = after;
  gotoTarget_ = -1;
  posKnown_ = false;
  if (limitPressed_) {
    // Already on the switch, but it could be either end. Try extending off it
    // first; if it stays pressed, we are at the extended end and retract instead.
    state_ = ST_HOMING_CLEAR;
    clearDir_ = +1;
    drive(+1, HOME_PWM, now);
  } else {
    enterSeek(now);
  }
}

void Motor::enterSeek(uint32_t now) {
  state_ = ST_HOMING_SEEK;
  drive(-1, HOME_PWM, now);
}

void Motor::readLimit(uint32_t now) {
  if (pinLimit_ < 0) return;
  bool raw = (digitalRead(pinLimit_) == LOW);
  if (raw != limitRaw_) {
    limitRaw_ = raw;
    limitChangedAt_ = now;
  } else if (raw != limitPressed_ && now - limitChangedAt_ >= LIMIT_DEBOUNCE_MS) {
    limitPressed_ = raw;
    if (!limitPressed_) limitArmed_ = true;
  }
}

bool Motor::command(uint8_t cmd, uint8_t value, uint32_t now) {
  switch (cmd) {
    case CMD_STOP:
      halt();
      afterHome_ = AFTER_IDLE;
      fault_ = FAULT_NONE;
      state_ = posKnown_ ? ST_IDLE : ST_UNINIT;
      return true;

    case CMD_FWD:
    case CMD_REV: {
      int8_t dir = (cmd == CMD_FWD) ? +1 : -1;
      fault_ = FAULT_NONE;
      gotoTarget_ = -1;
      if (value == 0) return command(CMD_STOP, 0, now);
      // Already parked on the switch at the end we are asked to drive into.
      if (posKnown_ && limitPressed_ && ((dir > 0 && pos_ >= 1.0f) || (dir < 0 && pos_ <= 0.0f))) {
        halt();
        state_ = ST_IDLE;
        return true;
      }
      drive(dir, value, now);
      state_ = dir > 0 ? ST_MOVING_FWD : ST_MOVING_REV;
      return true;
    }

    case CMD_HOME:
      if (state_ == ST_HOMING_CLEAR || state_ == ST_HOMING_SEEK) return true;
      startHoming(AFTER_IDLE, now);
      return true;

    case CMD_CAL:
      if (state_ == ST_CALIBRATING) return true;
      startHoming(AFTER_CAL, now);
      return true;

    case CMD_GOTO: {
      if (value > 100) return false;
      // Position unknown: refuse the GOTO and home instead. The master keeps
      // streaming GOTOs, so a later one lands once homing is done.
      if (!posKnown_) {
        if (state_ != ST_HOMING_CLEAR && state_ != ST_HOMING_SEEK) startHoming(AFTER_IDLE, now);
        return true;
      }
      fault_ = FAULT_NONE;
      int cur = posPct();
      if (abs((int)value - cur) < GOTO_DEADBAND_PCT && value != 0 && value != 100) {
        if (gotoTarget_ >= 0) {  // retargeted onto where we are: stop here
          halt();
          state_ = ST_IDLE;
        }
        return true;
      }
      int8_t dir = ((int)value > cur) ? +1 : -1;
      if ((int)value == cur) {
        // 0 / 100 requested at the estimated end: drive into the switch anyway
        // to re-sync, unless we are already sitting on it.
        if (limitPressed_) return true;
        dir = (value == 100) ? +1 : -1;
      }
      if (dir_ != dir || pwm_ != HOME_PWM) drive(dir, HOME_PWM, now);
      // 0 and 100 run until the limit switch trips, which re-syncs the estimate.
      gotoTarget_ = (value == 0 || value == 100) ? -1 : value;
      state_ = dir > 0 ? ST_MOVING_FWD : ST_MOVING_REV;
      return true;
    }
  }
  return false;
}

void Motor::update(uint32_t now) {
  uint32_t dt = now - lastUpdate_;
  lastUpdate_ = now;
  readLimit(now);

  if (dir_ != 0 && posKnown_) {
    pos_ += dir_ * (float)dt / (float)travelMs_;
    if (pos_ < 0.0f) pos_ = 0.0f;
    if (pos_ > 1.0f) pos_ = 1.0f;
  }

  bool tripped = limitPressed_ && limitArmed_;
  uint32_t elapsed = now - phaseStart_;

  switch (state_) {
    case ST_MOVING_FWD:
    case ST_MOVING_REV:
      if (tripped) {
        pos_ = (dir_ > 0) ? 1.0f : 0.0f;
        posKnown_ = true;
        halt();
        state_ = ST_IDLE;
      } else if (gotoTarget_ >= 0 &&
                 ((dir_ > 0 && pos_ * 100.0f >= gotoTarget_) ||
                  (dir_ < 0 && pos_ * 100.0f <= gotoTarget_))) {
        halt();
        state_ = ST_IDLE;
      } else if (elapsed > runawayMs()) {
        setFault(FAULT_RUNAWAY_TIMEOUT);
      }
      break;

    case ST_HOMING_CLEAR:
      if (!limitPressed_) {
        // Off the switch. If we cleared by retracting we were at the extended
        // end; either way, homing always finishes by retracting onto the switch.
        enterSeek(now);
      } else if (elapsed > HOMING_CLEAR_MS) {
        if (clearDir_ > 0) {
          clearDir_ = -1;
          drive(-1, HOME_PWM, now);
        } else {
          setFault(FAULT_HOMING_TIMEOUT);
        }
      }
      break;

    case ST_HOMING_SEEK:
      if (tripped) {
        halt();
        pos_ = 0.0f;
        posKnown_ = true;
        if (afterHome_ == AFTER_CAL) {
          afterHome_ = AFTER_IDLE;
          state_ = ST_CALIBRATING;
          drive(+1, HOME_PWM, now);  // switch is pressed, so it stays unarmed until released
        } else {
          state_ = ST_IDLE;
        }
      } else if (elapsed > (afterHome_ == AFTER_CAL ? (uint32_t)CAL_TIMEOUT_MS : runawayMs())) {
        setFault(FAULT_HOMING_TIMEOUT);
      }
      break;

    case ST_CALIBRATING:
      if (tripped) {
        travelMs_ = elapsed;
        newCalibration_ = true;
        halt();
        pos_ = 1.0f;
        posKnown_ = true;
        state_ = ST_IDLE;
      } else if (elapsed > CAL_TIMEOUT_MS) {
        setFault(FAULT_RUNAWAY_TIMEOUT);
      }
      break;

    default:
      break;
  }
}
