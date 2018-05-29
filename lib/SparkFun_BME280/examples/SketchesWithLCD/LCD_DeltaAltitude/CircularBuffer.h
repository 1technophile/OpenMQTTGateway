#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H
#include <stdint.h>

//****************************************************************************//
//
//  Circular Buffer
//
//****************************************************************************//

//Class CircularBuffer is int16_t
//Does not care about over-running real data ( if request is outside length's bounds ).
//For example, the underlying machine writes [48], [49], [0], [1] ... 

class CircularBuffer
{
public:
    CircularBuffer( uint16_t inputSize );
	~CircularBuffer();
    float getElement( uint16_t ); //zero is the push location
    void pushElement( float );
    float averageLast( uint16_t );
    uint16_t recordLength( void );
private:
	uint16_t cBufferSize;
    float *cBufferData;
    int16_t cBufferLastPtr;
    uint8_t cBufferElementsUsed;
};



#endif // CIRCULARBUFFER_H