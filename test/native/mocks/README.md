# Mock Implementations for Unit Testing

This directory contains mock implementations of interfaces and classes used in OpenMQTTGateway for unit testing purposes.

## Available Mocks

### mock_IStorage.h
Mock implementation of the \IStorage\ interface using Google Mock framework.

**Usage:**
\\\cpp
#include "../mocks/mock_IStorage.h"

MockStorage mockStorage;
EXPECT_CALL(mockStorage, begin(_, false))
    .WillOnce(Return(true));
EXPECT_CALL(mockStorage, putString("key", ::testing::An<const std::string&>()))
    .WillOnce(Return(100));
\\\

**Features:**
- Fully mocked \IStorage\ interface
- Uses \std::string\ (not Arduino \String\)
- Compatible with Google Mock expectations

### mock_RFBaseGateway.h
Mock implementation of the \RFReceiver\ class.

### mock_arduino.h
Mock implementations of Arduino framework functions and classes for unit testing without hardware dependencies.

**Includes:**
- Digital I/O functions (\pinMode\, \digitalWrite\, \digitalRead\)
- Analog I/O functions (\nalogRead\, \nalogWrite\)
- Time functions (\millis\, \micros\, \delay\)
- Serial mock
- Arduino \String\ class mock (simplified)

## Notes

- Mock implementations are designed to be used in unit tests only
- They should not be included in production code
- All mocks are header-only for simplicity
