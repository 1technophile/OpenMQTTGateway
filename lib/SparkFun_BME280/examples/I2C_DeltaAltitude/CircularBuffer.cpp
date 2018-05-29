#include "CircularBuffer.h"
#include <stdint.h>

//****************************************************************************//
//
//  Circular buffer
//
//****************************************************************************//

//Construct a CircularBuffer type with arguments
//  uint16_t inputSize: number of elements
CircularBuffer::CircularBuffer(uint16_t inputSize)
{
	cBufferData = new float[inputSize];
	cBufferLastPtr = 0;
	cBufferElementsUsed = 0;  
	cBufferSize = inputSize;
	
}

CircularBuffer::~CircularBuffer()
{
	delete[] cBufferData;

}

//Get an element at some depth into the circular buffer
//zero is the push location.  Max is cBufferSize - 1
//
//Arguments:
//  uint16_t elementNum: number of element in
//
float CircularBuffer::getElement( uint16_t elementNum )
{
  //Translate elementNum into terms of cBufferLastPtr.
  int16_t virtualElementNum;
  virtualElementNum = cBufferLastPtr - elementNum;
  if( virtualElementNum < 0 )
  {
    virtualElementNum += cBufferSize;
  }
  
  //Output the value
  return cBufferData[virtualElementNum];
}

//Put a new element into the buffer.
//This also expands the size up to the max size
//Arguments:
//
//  int16_t elementVal: value of new element
//
void CircularBuffer::pushElement( float elementVal )
{
  //inc. the pointer
  cBufferLastPtr++;

  //deal with roll
  if( cBufferLastPtr >= cBufferSize )
  {
    cBufferLastPtr = 0;
  }

  //write data
  cBufferData[cBufferLastPtr] = elementVal;

  //increase length up to cBufferSize
  if( cBufferElementsUsed < cBufferSize )
  {
    cBufferElementsUsed++;
  }
}

//Averages the last n numbers and provides that.  Discards fractions
float CircularBuffer::averageLast( uint16_t numElements )
{
	if( numElements < recordLength() )
	{
		numElements = recordLength();
	}
  //Add up all the elements
  float accumulator = 0;
  int8_t i;
  for( i = 0; i < numElements; i++ )
  {
    accumulator += getElement( i );
  }
  //Divide by number of elements
  if( numElements != 0 )
  {
	accumulator /= numElements;
  }
  else
  {
	  accumulator = 0;
  }
  return accumulator;
}

//Returns the current size of the buffer
uint16_t CircularBuffer::recordLength( void )
{
  return cBufferElementsUsed;
}