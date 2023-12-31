/**
 *  @file   FEP_RX22.cpp
 *  @brief  FEP受信用ライブラリ
 *  @author 安澤瑠
 *  @date   22/10/6
 */
#include "Ishikawacon.h"

#include <cstring>

Ishikawacon::Ishikawacon(PinName tx, PinName rx, uint8_t addr_, int baud) :
    UnbufferedSerial(tx, rx, baud)
{
    addr     = addr_;
    timeout  = TIMEOUT_COUNT;
    bufindex = 0;
}

void Ishikawacon::StartReceive()
{
    attach(callback(this, &Ishikawacon::ReceiveBytes));
    timeoutTimer.attach(callback(this, &Ishikawacon::TimeoutLoop), 100ms);
}

void Ishikawacon::TimeoutLoop()
{
    if (timeout >= TIMEOUT_COUNT) {
        status = false;
    } else {
        status = true;
        timeout++;
    }
}

void Ishikawacon::ReceiveBytes()
{
    // original: buffer[bufindex] = this->getc(); // Receive 1byte
    this->read(buffer + bufindex, 1);
    this->timeout = 0;

    if ( (!strncmp((char*)(buffer + ((256 + bufindex - 1)%256) ), "\r\n", 2)) ) { // <CR><LF> bufindex = <LF>(='\n')
        CheckData();
    }
    bufindex++;
}

void Ishikawacon::CheckData()
{
    indexofR=0;
    for (uint16_t i_Rbn=0; i_Rbn<256; i_Rbn++) {
        indexofR = (256 + bufindex - i_Rbn) % 256;
        if ( !strncmp((char*)(buffer + indexofR) , "RBN", 3) ) { // check header  indexofR is address of 'R'BN
#if ControllerMode
            getControllerState();
#else
            datalen = (buffer[(indexofR+6)%256]-48)*100 + (buffer[(indexofR+7)%256]-48)*10 + (buffer[(indexofR+8)%256]-48);
            for (int i_msg=0; i_msg<datalen; i_msg++) {
                msgdata[i_msg] = buffer[(indexofR+9+i_msg)%256];
            }
#endif
            
            return;
        }
    }
}

uint8_t Ishikawacon::getData(uint8_t *data)
{
    for(int i=0; i<128; i++) data[i] = msgdata[i];
    return datalen;
}

void Ishikawacon::getControllerState()
{
    /* コントローラの仕様変更時はここを変える */
    for (int i_button=0; i_button<2; i_button++) {
        button[i_button] = buffer[(indexofR+9+i_button)%256];
    }
    for (int i_stick=0; i_stick<4; i_stick++) {
        stick[i_stick] = buffer[(indexofR+11+i_stick)%256];
    }
    for (int i_trigger=0; i_trigger<2; i_trigger++) {
        trigger[i_trigger] = buffer[(indexofR+15+i_trigger)%256];
    }
    
    return;
}

bool Ishikawacon::getButton(uint8_t n)
{
    if (n<8) {
        return (button[0]>>n) & 1;
    } else {
        return (button[1]>>(n-8)) & 1;
    }
}

int16_t Ishikawacon::getStick(uint8_t n)
{
    if (n%2) return 128-stick[n];
    else return stick[n]-128;
}

int16_t Ishikawacon::getTrigger(uint8_t n)
{
    return trigger[n];
}

bool Ishikawacon::getStatus()
{
    return status;
}
