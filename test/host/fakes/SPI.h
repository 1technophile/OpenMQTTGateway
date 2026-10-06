#ifndef TEST_FAKE_SPI_H
#define TEST_FAKE_SPI_H

class SPIClass {
public:
  void begin() {
    began = true;
  }

  void begin(int sckValue, int misoValue, int mosiValue, int ssValue) {
    began = true;
    sck = sckValue;
    miso = misoValue;
    mosi = mosiValue;
    ss = ssValue;
  }

  bool began = false;
  int sck = -1;
  int miso = -1;
  int mosi = -1;
  int ss = -1;
};

inline SPIClass SPI;

#endif
