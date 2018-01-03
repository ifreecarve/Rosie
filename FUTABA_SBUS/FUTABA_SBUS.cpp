#include "FUTABA_SBUS.h"

void FUTABA_SBUS::begin() {
	uint8_t loc_sbusData[25] = {
    0x0f, 0x01, 0x04, 0x20, 0x00, 0xff, 0x07, 0x40,
    0x00, 0x02, 0x10, 0x80, 0x2c, 0x64, 0x21, 0x0b,
    0x59, 0x08, 0x40, 0x00, 0x02, 0x10, 0x80, 0x00,
    0x00
  };
	int16_t loc_channels[18] = {
    1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023,
    1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023,
    0, 0
  };
	int16_t loc_servos[18] = {
    1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023,
    1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023,
    0, 0};

  port.begin(BAUDRATE);

	memcpy(mSbusData, loc_sbusData,25);
	memcpy(mChannels, loc_channels,18);
	memcpy(mServos, loc_servos,18);
	mFailsafeStatus = SBUS_SIGNAL_OK;
	mSbusPassthrough = true;
	mToChannels = 0;
	mBufferIndex = 0;
	mFeedState = 0;
}

// Read channel data
int16_t FUTABA_SBUS::Channel(uint8_t ch) {
  if (ch < 0) return 1023;
  if (ch >= 16) return 1023;
  return mChannels[ch - 1];
}

// Read digital channel data
uint8_t FUTABA_SBUS::DigiChannel(uint8_t ch) {
  if (ch < 0) return 0;
  if (ch >= 2) return 0;
  return mChannels[15 + ch];
}

// Set servo position
void FUTABA_SBUS::Servo(uint8_t ch, int16_t position) {
  if (ch < 0) return;
  if (ch >= 16) return;
  if (position > 2048) position = 2048;
  mServos[ch - 1] = position;
}

// Set digital servo position
void FUTABA_SBUS::DigiServo(uint8_t ch, uint8_t position) {
  if (ch < 0) return;
  if (ch >= 2) return;
  if (position > 1) position = 1;
  mServos[15 + ch] = position;
}
uint8_t FUTABA_SBUS::Failsafe(void) {
  return mFailsafeStatus;
}

void FUTABA_SBUS::UpdateServos(void) {
  // Send data to mServos
  // Passtrough mode = false >> send own servo data
  // Passtrough mode = true >> send received channel data
  uint8_t i;
  uint8_t ch;
  uint8_t bit_in_servo;
  uint8_t byte_in_sbus;
  uint8_t bit_in_sbus;
  if (!mSbusPassthrough)
  {
    // clear received channel data
    for (i=1; i<24; i++) {
      mSbusData[i] = 0;
    }

    // reset counters
		ch = 0;
    bit_in_servo = 0;
    byte_in_sbus = 1;
    bit_in_sbus = 0;

    // store servo data
    for (i=0; i<176; i++) {
      if (mServos[ch] & (1<<bit_in_servo)) {
        mSbusData[byte_in_sbus] |= (1<<bit_in_sbus);
      }
      bit_in_sbus++;
      bit_in_servo++;

      if (bit_in_sbus == 8) {
        bit_in_sbus =0;
        byte_in_sbus++;
      }
      if (bit_in_servo == 11) {
        bit_in_servo =0;
        ch++;
      }
    }

    // DigiChannel 1
    if (mChannels[16] == 1) {
      mSbusData[23] |= (1<<0);
    }

    // DigiChannel 2
    if (mChannels[17] == 1) {
      mSbusData[23] |= (1<<1);
    }

    // Failsafe
    if (mFailsafeStatus == SBUS_SIGNAL_LOST) {
      mSbusData[23] |= (1<<2);
    }

    if (mFailsafeStatus == SBUS_SIGNAL_FAILSAFE) {
      mSbusData[23] |= (1<<2);
      mSbusData[23] |= (1<<3);
    }
  }
  // send data out
  //serialPort.write(mSbusData,25);
  for (i=0;i<25;i++) {
    port.write(mSbusData[i]);
  }
}

void FUTABA_SBUS::UpdateChannels(void) {

  mChannels[0]  = ((mSbusData[1]|mSbusData[2]<< 8) & 0x07FF);
  mChannels[1]  = ((mSbusData[2]>>3|mSbusData[3]<<5) & 0x07FF);
  mChannels[2]  = ((mSbusData[3]>>6|mSbusData[4]<<2|mSbusData[5]<<10) & 0x07FF);
  mChannels[3]  = ((mSbusData[5]>>1|mSbusData[6]<<7) & 0x07FF);
  mChannels[4]  = ((mSbusData[6]>>4|mSbusData[7]<<4) & 0x07FF);
  mChannels[5]  = ((mSbusData[7]>>7|mSbusData[8]<<1|mSbusData[9]<<9) & 0x07FF);
  mChannels[6]  = ((mSbusData[9]>>2|mSbusData[10]<<6) & 0x07FF);
  mChannels[7]  = ((mSbusData[10]>>5|mSbusData[11]<<3) & 0x07FF); // & the other 8 + 2 mChannels if you need them
  #ifdef ALL_CHANNELS
  mChannels[8]  = ((mSbusData[12]|mSbusData[13]<< 8) & 0x07FF);
  mChannels[9]  = ((mSbusData[13]>>3|mSbusData[14]<<5) & 0x07FF);
  mChannels[10] = ((mSbusData[14]>>6|mSbusData[15]<<2|mSbusData[16]<<10) & 0x07FF);
  mChannels[11] = ((mSbusData[16]>>1|mSbusData[17]<<7) & 0x07FF);
  mChannels[12] = ((mSbusData[17]>>4|mSbusData[18]<<4) & 0x07FF);
  mChannels[13] = ((mSbusData[18]>>7|mSbusData[19]<<1|mSbusData[20]<<9) & 0x07FF);
  mChannels[14] = ((mSbusData[20]>>2|mSbusData[21]<<6) & 0x07FF);
    mChannels[15] = ((mSbusData[21]>>5|mSbusData[22]<<3) & 0x07FF);
  #endif

  // Failsafe
  mFailsafeStatus = SBUS_SIGNAL_OK;
  if (mSbusData[23] & (1<<2)) {
    mFailsafeStatus = SBUS_SIGNAL_LOST;
  }
  if (mSbusData[23] & (1<<3)) {
    mFailsafeStatus = SBUS_SIGNAL_FAILSAFE;
  }

}

void FUTABA_SBUS::FeedLine(void){
  //check to see if 25 or more characters are available at the serial port
  if (port.available() < 25) return;

  uint8_t inBuffer[25];
  uint8_t inData;

  //loop while available characters
  while(port.available() > 0) {
    inData = port.read();
    switch (mFeedState){
    case 0:					//mFeedState = 0 indicates start of a packet
      if (inData != 0x0f){				//if first byte isn't 0x0F this is a problem
        while(port.available() > 0) { //read the contents of in buffer this should resync the transmission
          inData = port.read();
        }
        return;
      } else {					//mFeedState = 0, and first byte IS 0x0F (all good)
        mBufferIndex = 0;				//read first byte, set last byte in buffer = 24, set mFeedState = 1
        inBuffer[mBufferIndex] = inData;
        inBuffer[24] = 0xff;
        mFeedState = 1;
      }
      break;
    case 1:					//mFeedState = 1, we're in the middle of a packet, read the next byte
      mBufferIndex ++;
      inBuffer[mBufferIndex] = inData;
      if (mBufferIndex < 24 && port.available() == 0){	//we're reading a packet. if no more bytes coming, reset mFeedState to 0 (problem)
        mFeedState = 0;
      }
      if (mBufferIndex == 24){				//we're at 24 characters, end of packet.  set mFeedState to 0, update mSbusData array, set mToChannels = 1
        mFeedState = 0;
        if (inBuffer[0]==0x0f && inBuffer[24] == 0x00){
          memcpy(mSbusData,inBuffer,25);
          mToChannels = 1;
        }
      }
      break;
    }
  }
}

