
#pragma once

class RFBaseGateway {
public:
  virtual ~RFBaseGateway() = default;

  // Pure virtual methods
  virtual void enable() = 0;
  virtual void disable() = 0;
  virtual int getReceiverID() const = 0;
};
