#include "pid.h"
#include "defines.h"
#include "logger.h"

namespace hakkou {

void PID::reset() {
  integrator_ = 0.0;
  prev_err_ = 0.0;
  differentiator_ = 0.0;
  prev_measurement = 0.0;

  // wrap around every 50 seconds or so
  last_time = timer_u32();
}

float PID::update(float setpoint, float measurement) {
  setpoint_ = setpoint;

  // calculate the actual sample time in s
  u32 new_time = timer_u32();
  float Ts = timer_delta_s(new_time - last_time);

  // error signal
  float error = setpoint_ - measurement;

  // proportial term
  float proportional = cfg_.Kp * error;

  // integral term
  integrator_ = integrator_ + 0.5f * cfg_.Ki * Ts * (error + prev_err_);
  // anti wind up
  if (integrator_ > cfg_.i_limit_max) {
    integrator_ = cfg_.i_limit_max;
  } else if (integrator_ < cfg_.i_limit_min) {
    integrator_ = cfg_.i_limit_min;
  }

  // differential term [band-limited] (low-pass)
  // this is calculated not on the error signal, but the measurement
  // the goal is to avoid the `kick, when a large step in setpoint occurs
  /* Note: derivative on measurement, therefore
                                  minus sign in front of equation! */
  differentiator_ = -(2.0f * cfg_.Kd * (measurement - prev_measurement) +
                      (2.0f * cfg_.tau - Ts) * differentiator_) /
                    (2.0f * cfg_.tau + Ts);

  // calculate output and ensure bounds
  out_ = proportional + integrator_ + differentiator_;
  if (out_ > cfg_.out_max) {
    out_ = cfg_.out_max;
  } else if (out_ < cfg_.out_min) {
    out_ = cfg_.out_min;
  }

  // store signals for next iteration
  prev_err_ = error;
  prev_measurement = measurement;

  HDEBUG("P=%f, I=%f, D=%f", proportional, integrator_, differentiator_);
  last_time = new_time;
  return out_;
}

}  // namespace hakkou