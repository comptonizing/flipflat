#include "CommandBuffer.h"

CommandBuffer &CommandBuffer::i() {
  static CommandBuffer theInstance;
  return theInstance;
}

CommandBuffer::CommandBuffer() {
  clear();
}

CommandBuffer::~CommandBuffer() {
}

void CommandBuffer::clear() {
  m_buff[0] = '\0';
  pos = m_buff;
}

const char *CommandBuffer::get() {
  return m_buff;
}

bool CommandBuffer::add(char c) {
  if ( (pos - m_buff + 1) > CMDBUFF_SIZE ) {
    return false;
  }
  *pos++ = c;
  *pos = '\0';
  return true;
}

bool CommandBuffer::verifyChecksum() {
  char *start = m_buff + 1;
  int len = pos - m_buff - 5;
  uint16_t crcRef;
  memcpy(&crcRef, start + len + 1, 2);
  uint16_t crc = 0;
  for (int ii=0; ii<len; ii++) {
    crc = _crc16_update(crc, start[ii]);
  }
  char *crcChar = (char *) &crc;
  if ( crcChar[0] == '$' ) {
    crcChar[0] = '1';
  }
  if ( crcChar[1] == '$' ) {
    crcChar[1] = '1';
  }
  return crc == crcRef;
}

const char *CommandBuffer::getCommand() {
  return m_buff + 1;
}
