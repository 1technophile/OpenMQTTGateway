#ifndef RFRECEIVER_H
#define RFRECEIVER_H
#pragma once

class RFReceiver {
public:
  virtual ~RFReceiver() = default;

  // Pure virtual methods
  virtual void enable() = 0;
  virtual void disable() = 0;
  virtual int getReceiverID() const = 0;
};

#endif // RFRECEIVER_H