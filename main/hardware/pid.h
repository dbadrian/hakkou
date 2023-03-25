#pragma once
// Derived from:
//
// https://github.com/pms67/PID
// Licensed under MIT:
//
// MIT License
//
// Copyright (c) 2020 Philip Salmony
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// Notes on changes:
// - Dedicated config struct
// - questionable turning into a class ;)

#include "defines.h"

#include "ext/timer_u32.h"

namespace hakkou {

struct PIDConfig {
  float Kp;
  float Ki;
  float Kd;
  float tau;
  float out_min;
  float out_max;
  float i_limit_min;
  float i_limit_max;
};

class PID {
 public:
  PID(PIDConfig cfg) : cfg_(cfg) { reset(); }

  void reset();
  [[nodiscard]] float update(float setpoint, float measurement);

 private:
  PIDConfig cfg_;

  // memory of the PID
  float setpoint_{0.0};
  float integrator_{0.0};
  float differentiator_{0.0};
  float prev_err_{0.0};
  float prev_measurement{0.0};
  u32 last_time{0};
  float out_{0.0};
};

}  // namespace hakkou