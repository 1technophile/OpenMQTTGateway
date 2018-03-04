// **********************************************************************************
// Automatic Transmit Power Control class derived from RFM69 library.
// Discussion and details in this forum post: https://lowpowerlab.com/forum/index.php/topic,688.0.html
// **********************************************************************************
// Copyright Thomas Studwell (2014,2015)
// Adjustments by Felix Rusu, LowPowerLab.com
// **********************************************************************************
// License
// **********************************************************************************
// This program is free software; you can redistribute it 
// and/or modify it under the terms of the GNU General    
// Public License as published by the Free Software       
// Foundation; either version 3 of the License, or        
// (at your option) any later version.                    
//                                                        
// This program is distributed in the hope that it will   
// be useful, but WITHOUT ANY WARRANTY; without even the  
// implied warranty of MERCHANTABILITY or FITNESS FOR A   
// PARTICULAR PURPOSE. See the GNU General Public        
// License for more details.                              
//                                                        
// Licence can be viewed at                               
// http://www.gnu.org/licenses/gpl-3.0.txt
//
// Please maintain this license information along with authorship
// and copyright notices in any redistribution of this code
// **********************************************************************************
#include <RFM69_ATC.h>
#include <RFM69.h>   // include the RFM69 library files as well
#include <RFM69registers.h>
#include <SPI.h>

volatile uint8_t RFM69_ATC::ACK_RSSI_REQUESTED;  // new type of flag on ACK_REQUEST

//=============================================================================
// initialize() - some extra initialization before calling base class
//=============================================================================
bool RFM69_ATC::initialize(uint8_t freqBand, uint8_t nodeID, uint8_t networkID) {
  _targetRSSI = 0;        // TomWS1: default to disabled
  _ackRSSI = 0;           // TomWS1: no existing response at init time
  ACK_RSSI_REQUESTED = 0; // TomWS1: init to none
  //_powerBoost = false;    // TomWS1: require someone to explicitly turn boost on!
  _transmitLevel = 31;    // TomWS1: match default value in PA Level register
  return RFM69::initialize(freqBand, nodeID, networkID);  // use base class to initialize most everything
}

//=============================================================================
// setMode() - got to set updated transmit power level before switching to TX mode
//=============================================================================
void RFM69_ATC::setMode(uint8_t newMode) {
  if (newMode == _mode) return;
  //_powerBoost = (_transmitLevel >= 50);  // this needs to be set before changing mode just in case setHighPowerRegs is called
  RFM69::setMode(newMode);  // call base class first

  if (newMode == RF69_MODE_TX)  // special stuff if switching to TX mode
  {
    if (_targetRSSI) setPowerLevel(_transmitLevel);   // TomWS1: apply most recent transmit level if auto power
    //if (_isRFM69HW) setHighPowerRegs(true);
  }
}

//=============================================================================
// sendAck() - updated to call new sendFrame with additional parameters
//=============================================================================
// should be called immediately after reception in case sender wants ACK
void RFM69_ATC::sendACK(const void* buffer, uint8_t bufferSize) {
  ACK_REQUESTED = 0;   // TomWS1 added to make sure we don't end up in a timing race and infinite loop sending Acks
  uint8_t sender = SENDERID;
  int16_t _RSSI = RSSI; // save payload received RSSI value
  bool sendRSSI = ACK_RSSI_REQUESTED;  
  writeReg(REG_PACKETCONFIG2, (readReg(REG_PACKETCONFIG2) & 0xFB) | RF_PACKET2_RXRESTART); // avoid RX deadlocks
  uint32_t now = millis();
  while (!canSend() && millis() - now < RF69_CSMA_LIMIT_MS) receiveDone();
  SENDERID = sender;    // TomWS1: Restore SenderID after it gets wiped out by receiveDone()
  sendFrame(sender, buffer, bufferSize, false, true, sendRSSI, _RSSI);   // TomWS1: Special override on sendFrame with extra params
  RSSI = _RSSI; // restore payload RSSI
}

//=============================================================================
// sendFrame() - the basic version is used to match the RFM69 prototype so we can extend it
//=============================================================================
// this sendFrame is generally called by the internal RFM69 functions.  Simply transfer to our modified version.
void RFM69_ATC::sendFrame(uint8_t toAddress, const void* buffer, uint8_t bufferSize, bool requestACK, bool sendACK) {
  sendFrame(toAddress, buffer, bufferSize, requestACK, sendACK, false, 0);  // default sendFrame
}

//=============================================================================
// sendFrame() - the new one with additional parameters.  This packages recv'd RSSI with the packet, if required.
//=============================================================================
void RFM69_ATC::sendFrame(uint8_t toAddress, const void* buffer, uint8_t bufferSize, bool requestACK, bool sendACK, bool sendRSSI, int16_t lastRSSI) {
  setMode(RF69_MODE_STANDBY); // turn off receiver to prevent reception while filling fifo
  while ((readReg(REG_IRQFLAGS1) & RF_IRQFLAGS1_MODEREADY) == 0x00); // wait for ModeReady
  writeReg(REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_00); // DIO0 is "Packet Sent"
  
  bufferSize += (sendACK && sendRSSI)?1:0;  // if sending ACK_RSSI then increase data size by 1
  if (bufferSize > RF69_MAX_DATA_LEN) bufferSize = RF69_MAX_DATA_LEN;

  // write to FIFO
  select();
  SPI.transfer(REG_FIFO | 0x80);
  SPI.transfer(bufferSize + 3);
  SPI.transfer(toAddress);
  SPI.transfer(_address);

  // control byte
  if (sendACK) {                   // TomWS1: adding logic to return ACK_RSSI if requested
    SPI.transfer(RFM69_CTL_SENDACK | (sendRSSI?RFM69_CTL_RESERVE1:0));  // TomWS1  TODO: Replace with EXT1
    if (sendRSSI) {
      SPI.transfer(abs(lastRSSI)); //RSSI dBm is negative expected between [-100 .. -20], convert to positive and pass along as single extra header byte
      bufferSize -=1;              // account for the extra ACK-RSSI 'data' byte
    }
  }
  else if (requestACK) {  // TODO: add logic to request ackRSSI with ACK - this is when both ends of a transmission would dial power down. May not work well for gateways in multi node networks
    SPI.transfer(_targetRSSI ? RFM69_CTL_REQACK | RFM69_CTL_RESERVE1 : RFM69_CTL_REQACK);
  }
  else SPI.transfer(0x00);

  for (uint8_t i = 0; i < bufferSize; i++)
    SPI.transfer(((uint8_t*) buffer)[i]);
  unselect();

  // no need to wait for transmit mode to be ready since its handled by the radio
  setMode(RF69_MODE_TX);
  uint32_t txStart = millis();
  while (digitalRead(_interruptPin) == 0 && millis() - txStart < RF69_TX_LIMIT_MS); // wait for DIO0 to turn HIGH signalling transmission finish
  //while (readReg(REG_IRQFLAGS2) & RF_IRQFLAGS2_PACKETSENT == 0x00); // wait for ModeReady
  setMode(RF69_MODE_STANDBY);
}

//=============================================================================
// interruptHook() - gets called by the base class interrupt handler right after the header is fetched.
//=============================================================================
void RFM69_ATC::interruptHook(uint8_t CTLbyte) {
  ACK_RSSI_REQUESTED = CTLbyte & RFM69_CTL_RESERVE1; // TomWS1: extract the ACK RSSI request bit (could potentially merge with ACK_REQUESTED)
  // TomWS1: now see if this was an ACK with an ACK_RSSI response
  if (ACK_RECEIVED && ACK_RSSI_REQUESTED) {
    // the next two bytes contain the ACK_RSSI (assuming the datalength is valid)
    if (DATALEN >= 1) {
      _ackRSSI = -1 * SPI.transfer(0); //rssi was sent as single byte positive value, get the real value by * -1
      DATALEN -= 1;   // and compensate data length accordingly
      // TomWS1: Now dither transmitLevel value (register update occurs later when transmitting);
      if (_targetRSSI != 0) {
        // if (_isRFM69HW) {
          // if (_ackRSSI < _targetRSSI && _transmitLevel < 51) _transmitLevel++;
          // else if (_ackRSSI > _targetRSSI && _transmitLevel > 32) _transmitLevel--;
        // } else {
        if (_ackRSSI < _targetRSSI && _transmitLevel < 31) { _transmitLevel++; /*Serial.println("\n ======= _transmitLevel ++   ======");*/ }
        else if (_ackRSSI > _targetRSSI && _transmitLevel > 0) { _transmitLevel--; /*Serial.println("\n ======= _transmitLevel --   ======");*/ }
        //}
      }
    }
  }
}

//=============================================================================
//  sendWithRetry() - overrides the base to allow increasing power when repeated ACK requests fail
//=============================================================================
bool RFM69_ATC::sendWithRetry(uint8_t toAddress, const void* buffer, uint8_t bufferSize, uint8_t retries, uint8_t retryWaitTime) {
  uint32_t sentTime;
  for (uint8_t i = 0; i <= retries; i++)
  {
    send(toAddress, buffer, bufferSize, true);
    sentTime = millis();
    while (millis() - sentTime < retryWaitTime)
    {
      if (ACKReceived(toAddress))
      {
        return true;
      }
    }
  }
  if (_transmitLevel < 31) _transmitLevel++;
  return false;
}

//=============================================================================
//  receiveBegin() - need to clear out our flag before calling base class.
//=============================================================================
void RFM69_ATC::receiveBegin() {
  ACK_RSSI_REQUESTED = 0;
  RFM69::receiveBegin();
}

//=============================================================================
// setPowerLevel() - outright replacement for base class.  Provides finer granularity for RFM69HW.
//=============================================================================
// set output power: 0=min, 31=max (for RFM69W or RFM69CW), 0-31 or 32->51 for RFM69HW (see below)
// this results in a "weaker" transmitted signal, and directly results in a lower RSSI at the receiver
// allows power level selections above 31 (as in original base RFM69 lib) & selects appropriate PA based on the value
// more discussion and details in this forum post: https://lowpowerlab.com/forum/index.php/topic,688.0.html
// void RFM69_ATC::setPowerLevel(uint8_t powerLevel) {
  // _transmitLevel = powerLevel;    // save this for later in case we do auto power control.
  // _powerBoost = (powerLevel >= 50);
  // if (!_isRFM69HW || powerLevel < 32) {     // use original code without change
    // _powerLevel = powerLevel;
    // writeReg(REG_PALEVEL, (readReg(REG_PALEVEL) & 0xE0) | (_powerLevel > 31 ? 31 : _powerLevel));
  // } else {
    // // the allowable range of power level value, if >31 is: 32 -> 51, where...
    // // 32->47 use PA2 only and sets powerLevel register 0-15,
    // // 48->49 uses both PAs, and sets powerLevel register 14-15,
    // // 50->51 uses both PAs, sets powerBoost, and sets powerLevel register 14-15.
    // if (powerLevel < 48) {
      // _powerLevel = powerLevel & 0x0f;  // just use 4 lower bits when in high power mode
      // _PA_Reg = 0x20;
    // } else {
      // _PA_Reg = 0x60;
      // if (powerLevel < 50) {
        // _powerLevel = powerLevel - 34;  // leaves 14-15
      // } else {
        // if (powerLevel > 51) 
          // powerLevel = 51;  // saturate
        // _powerLevel = powerLevel - 36;  // leaves 14-15
      // }
    // }
    // writeReg(REG_OCP, (_PA_Reg==0x60) ? RF_OCP_OFF : RF_OCP_ON);
    // writeReg(REG_PALEVEL, _powerLevel | _PA_Reg);
  // }
// }

//=============================================================================
// setHighPower() - only set High power bits on RFM69HW IFF the power level is set to MAX.  Otherwise it is kept off.
//=============================================================================
// void RFM69_ATC::setHighPower(bool onOff, byte PA_ctl) {
  // _isRFM69HW = onOff;
  // writeReg(REG_OCP, (_isRFM69HW && PA_ctl==0x60) ? RF_OCP_OFF : RF_OCP_ON);
  // if (_isRFM69HW) { //turning ON based on module type 
    // _powerLevel = readReg(REG_PALEVEL) & 0x1F; // make sure internal value matches reg
    // _powerBoost = (PA_ctl == 0x60);
    // _PA_Reg = PA_ctl;
    // writeReg(REG_PALEVEL, _powerLevel | PA_ctl ); //TomWS1: enable selected P1 & P2 amplifier stages
  // }
  // else {
    // _PA_Reg = RF_PALEVEL_PA0_ON;        // TomWS1: save to reflect register value
    // writeReg(REG_PALEVEL, RF_PALEVEL_PA0_ON | RF_PALEVEL_PA1_OFF | RF_PALEVEL_PA2_OFF | _powerLevel); //enable P0 only
  // }
// }

//=============================================================================
// ditto from above.
//=============================================================================
// void RFM69_ATC::setHighPowerRegs(bool onOff) {
  // if ((0x60 != (readReg(REG_PALEVEL) & 0xe0)) || !_powerBoost)    // TomWS1: only set to high power if we are using both PAs... and boost range is requested.
    // onOff = false;
  // writeReg(REG_TESTPA1, onOff ? 0x5D : 0x55);
  // writeReg(REG_TESTPA2, onOff ? 0x7C : 0x70);
// }

//=============================================================================
// enableAutoPower() - call with target RSSI, use 0 to disable (default), any other value with turn on autotransmit control.
//=============================================================================
// TomWS1: New methods to address autoPower control
void  RFM69_ATC::enableAutoPower(int16_t targetRSSI){    // TomWS1: New method to enable/disable auto Power control
  _targetRSSI = targetRSSI;         // no logic here, just set the value (if non-zero, then enabled), caller's responsibility to use a reasonable value
}

//=============================================================================
// getAckRSSI() - returns the RSSI value ack'd by the far end.
//=============================================================================
int16_t  RFM69_ATC::getAckRSSI(void){                     // TomWS1: New method to retrieve the ack'd RSSI (if any)
  return (_targetRSSI==0?0:_ackRSSI);
}

//=============================================================================
// setLNA() - used for power level testing.
//=============================================================================
byte RFM69_ATC::setLNA(byte newReg) {  // TomWS1: New method used to disable LNA AGC for testing purposes
  byte oldReg;
  oldReg = readReg(REG_LNA);
  writeReg(REG_LNA, ((newReg & 7) | (oldReg & ~7)));   // just control the LNA Gain bits for now
  return oldReg;  // return the original value in case we need to restore it
}