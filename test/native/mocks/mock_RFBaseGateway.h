#include <gmock/gmock.h> // Brings in gMock.
#include <rf/RFBaseGateway.h> // Include the RFReceiver base class

class MockRFGateway : public RFBaseGateway {
public:
  MOCK_METHOD(void, enable, (), (override));
  MOCK_METHOD(void, disable, (), (override));
  MOCK_METHOD(int, getReceiverID, (), (const, override));
};