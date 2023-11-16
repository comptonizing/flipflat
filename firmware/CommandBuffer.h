#pragma once

#define CMDBUFF_SIZE 32

#include <Arduino.h>
#include <util/crc16.h>

class CommandBuffer {
  public:
    static CommandBuffer &i();
    void clear();
    const char *get();
    const char *getCommand();
    bool add(char c);
    bool verifyChecksum();
  private:
    ~CommandBuffer();
    CommandBuffer();
    CommandBuffer(const CommandBuffer&);
    CommandBuffer & operator=(const CommandBuffer&);
    char m_buff[CMDBUFF_SIZE];
    char *pos = m_buff;
};
