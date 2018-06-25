#include "Driver.h"
#include <iostream>
#include "HandlerSerial.h"
#include <semaphore.h>
#include <signal.h>
#include <pthread.h>
#include "circularVectorTS.hpp"



using namespace std;


sem_t mainLoop, recvFsmSem;
bool running;
pthread_t recvT, recvFsmT, sendT;
circularVectorTS<char> * rxBuffer;
//int setPoint;

bool updateSetPoint;

enum recvStateEnum {
    RECV_HEADER = 1,
    RECV_LEN_1,
    RECV_LEN_2,
    RECV_DATA,
    RECV_CRC_1,
    RECV_CRC_2
};

recvStateEnum recvState;




void interruptHandler(int32_t);
static void * recvHandler(void * params);
static void * recvFsmHandler(void * params);
static void * sendHandler(void * params);


Driver::Driver() {
    gui_queue = new queueTS<float>(1000);
    gui_queue2 = new queueTS<float>(1000);
    gui_queue3 = new queueTS<float>(1000);
    this->serialDescriptor = new HandlerSerial();
    this->device_measure_variables = new queueTS<Driver::MeasuredVarArray>(1000);
    sem_init(&this->recvFsmSEM, 0, 0);
    pthread_mutex_init(&this->rxBufferMUX, NULL);
    pthread_mutex_init(&this->sendMUX, 0);
    pthread_cond_init(&this->recvCOND,NULL);
    this->rxBuffer = new circularVectorTS<char>(4096);
    this->driverState = DRIVER_STATE_READY;
    pthread_mutex_init(&this->recvMUX, NULL);
    sem_init(&this->recvSEM, 0, 0);
    sem_init(&this->sendSEM, 0, 1);
}


vector<int> Driver::getAvailablePorts() {
    return this->serialDescriptor->getAvailablePorts();
}

#define CRC16_GEN 0x1021
#define CRC16_MSB 0x8000

int Driver::calcCRC(char * buffer, int length) {
    int16_t crcbin, crcOverflow, input;
    int j, i;
    char c;
    crcbin = 0xFFFF;

    for (j=0; j<length; j++) {
    c = buffer[j] & 0xFF;
    for (i = 0x80; i ; i>>=1) {
        crcOverflow = crcbin & CRC16_MSB;
        input = ((i&c) ? CRC16_MSB : 0);
        crcbin <<= 1;
        if (input^crcOverflow) crcbin ^= CRC16_GEN;
    }
    }

    return crcbin;
}

void * Driver::recvFsmHandler(void * params) {
    Driver * thisClass = (Driver*) params;
    char newChar;
    int leftBytes;
    char incomingMsg[1024];
    int bufPos;
    FsmRecvState recvState = RECV_HEADER;

    cout << "FSM running" << endl;
    while (thisClass->driverState != DRIVER_STATE_STOPPED) {
        sem_wait(&recvFsmSem);
        while ((thisClass->driverState != DRIVER_STATE_STOPPED)&&(!thisClass->rxBuffer->isEmpty())) {
            newChar = (thisClass->rxBuffer->tailPop() & 0xFF);
            switch (recvState) {
                case RECV_HEADER:
                    if (newChar == 0x7E) {
                        recvState = RECV_LENGTH;
                        leftBytes = 0;
                        bufPos = 0;
                        incomingMsg[bufPos] = newChar;
                        bufPos++;
                    }
                break;
                case RECV_LENGTH:
                    recvState = RECV_DATA;
                    leftBytes = ((newChar & 0xFF));
                    incomingMsg[bufPos] = newChar;
                    bufPos++;
                break;
                case RECV_DATA:
                    leftBytes--;
                    incomingMsg[bufPos] = newChar;
                    bufPos++;
                    if (leftBytes <= 0) {
                        recvState = RECV_CRC_1;
                    }
                break;
                case RECV_CRC_1:
                    recvState = RECV_CRC_2;
                    incomingMsg[bufPos] = newChar;
                    bufPos++;
                break;
                case RECV_CRC_2:
                    incomingMsg[bufPos] = newChar;
                    bufPos++;
                    recvState = RECV_HEADER;
                    //cout << "-->> RECVFSM" << endl;
                    if (!Driver::calcCRC(incomingMsg, bufPos)) {
                        if (incomingMsg[4] == 5) {
                            // Telemetry message
                            MeasuredVarArray array;
                            array.num_elements = 1;
                            MeasuredVar var;
                            var.id = 0;
                            int16_t val = (((int16_t)(incomingMsg[6]<<8)+(incomingMsg[7]&0xFF)));
                           // cout << val << endl;
                            var.value = val / 100.0;
                            MeasuredVar var1;
                            var1.id = 0;
                            val = (((int16_t)(incomingMsg[8]<<8)+(incomingMsg[9]&0xFF)));
                            var1.value = val / 100.0;
                            MeasuredVar var2;
                            var2.id = 0;
                            val = (((int16_t)(incomingMsg[10]<<8)+(incomingMsg[11]&0xFF)));
                            var2.value = val/100.0;//* 10.0;
                            array.measures.push_back(var);
                            array.measures.push_back(var1);
                            array.measures.push_back(var2);
                            thisClass->device_measure_variables->push(array);

                            //TODO Update table aswell
                        } else {
                           // cout << "RECEIVED SOMTHING" << endl;
                            // Command message
                            memcpy(thisClass->recvResponse, incomingMsg, bufPos);
                            thisClass->len = bufPos;
                            int val = thisClass->recvResponse[7];
                             pthread_cond_signal(&thisClass->recvCOND);
                        }

                    } else {
                        cout << "Wrong CRC!!" << endl;
                        for (int i = 0 ; i < bufPos ; i++) {
                            cout << i << "->" << hex << (int)incomingMsg[i] << endl;
                        }
                        thisClass->len = 0;
                        pthread_cond_signal(&thisClass->recvCOND);
                    }
                break;
            }
        }
    }

    return NULL;
}

void * Driver::recvHandler(void * params) {
    Driver * thisClass = (Driver *)params;
    char rxTempBuffer[4096];
    int nbytes;

    cout << "recvHandler started" << endl;
    while (thisClass->driverState != DRIVER_STATE_STOPPED) {
        nbytes = thisClass->serialDescriptor->recv(rxTempBuffer, 255);
        //cout << "received!! " << nbytes << endl;
    if (nbytes > 0) {
        //for (int i = 0 ; i < nbytes ; i++) {
        //    cout << i << "READ->" << hex << (int)rxTempBuffer[i] << endl;
        // }
        thisClass->rxBuffer->headPush(rxTempBuffer, nbytes);
        sem_post(&recvFsmSem);

    }
    }
    cout << "recvHandler stopped" << endl;

    return NULL;
}

//struct timespec { long tv_sec; long tv_nsec; };    //header part
/*
int clock_gettime(int, struct timespec *spec)      //C-file part
{  __int64 wintime; GetSystemTimeAsFileTime((FILETIME*)&wintime);
   wintime      -=116444736000000000LL;  //1jan1601 to 1jan1970
   spec->tv_sec  =wintime / 10000000LL;           //seconds
   spec->tv_nsec =wintime % 10000000LL *100;      //nano-seconds
   return 0;
}
*/



//struct timespec { long tv_sec; long tv_nsec; };   //header part
#define exp7           10000000LL     //1E+7     //C-file part
#define exp9         1000000000LL     //1E+9
#define w2ux 116444736000000000LL     //1.jan1601 to 1.jan1970
void unix_time(struct timespec *spec)
{  __int64 wintime; GetSystemTimeAsFileTime((FILETIME*)&wintime);
   wintime -=w2ux;  spec->tv_sec  =wintime / exp7;
                    spec->tv_nsec =wintime % exp7 *100;
}
int clock_gettime(int, timespec *spec)
{  static  struct timespec startspec; static double ticks2nano;
   static __int64 startticks, tps =0;    __int64 tmp, curticks;
   QueryPerformanceFrequency((LARGE_INTEGER*)&tmp); //some strange system can
   if (tps !=tmp) { tps =tmp; //init ~~ONCE         //possibly change freq ?
                    QueryPerformanceCounter((LARGE_INTEGER*)&startticks);
                    unix_time(&startspec); ticks2nano =(double)exp9 / tps; }
   QueryPerformanceCounter((LARGE_INTEGER*)&curticks); curticks -=startticks;
   spec->tv_sec  =startspec.tv_sec   +         (curticks / tps);
   spec->tv_nsec =startspec.tv_nsec  + (double)(curticks % tps) * ticks2nano;
         if (!(spec->tv_nsec < exp9)) { spec->tv_sec++; spec->tv_nsec -=exp9; }
   return 0;
}

bool Driver::start() {
    bool bOk = true;
    cout << "Starting driver!!" << endl;
    if (this->driverState == DRIVER_STATE_READY) {
        if (!this->serialDescriptor->start()) {
            pthread_create(&recvHandlerTID, NULL, Driver::recvHandler, this);
            pthread_create(&recvFsmHandlerTID, NULL, Driver::recvFsmHandler, this);
            this->driverState = DRIVER_STATE_RUNNING;
        } else {
            bOk = false;
        }
    } else {
        cout << "[ERROR] Unable to start driver, not ready..." << endl;
        bOk = false;
    }
   // recvState = RECV_HEADER;
    return bOk;
}

bool Driver::stop() {
    bool bOk = false;

    if (this->driverState != DRIVER_STATE_STOPPED) {
        this->driverState = DRIVER_STATE_STOPPED;
        this->serialDescriptor->stop();
        sem_post(&recvFsmSem);
        cout << "Closing Receive thread..." << endl;
        pthread_join(recvHandlerTID, NULL);
        cout << "Closing FSM thread..." << endl;
        pthread_join(recvFsmHandlerTID, NULL);
        cout << "eMotion driver stopped" << endl;
        bOk = true;
    }
    this->driverState = DRIVER_STATE_READY;

    return bOk;
}

bool Driver::setRecvCB(void *(*cbFunction)( char *, int)) {
    bool bOk = false;

    this->recvCB = cbFunction;
   // this->cbClass = funcClass;
    bOk = true;

    return bOk;
}


bool Driver::send(char * buffer, int len, char * response, int * lenResponse, int blocking) {
    bool bOk = false;
    timespec ts;

    if (this->driverState == DRIVER_STATE_RUNNING) {
       // pthread_mutex_lock(&this->sendMUX);


        updateSetPoint = false;
        int8_t str[] = "\x7e\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xFF\xFF";
        str[1] = ((len + 2)) & 0xFF;

        str[2] = 0;
        str[3] = 0;

        memcpy(&str[4], buffer, len);

        int crc = calcCRC((char *)str, len+4);
        str[len + 4] = (crc >> 8) & 0xFF;
        str[len + 4 +1] = crc & 0xFF;


        timespec sendTS;
        //this->len = 0;
       // this->recvResponse = NULL;
        this->serialDescriptor->send((char *)str, len + 4 + 2);
        if (!blocking) {

        } else {

        clock_gettime(0, &ts);
         clock_gettime(0, &sendTS);
        ts.tv_nsec += 200000000;
        ts.tv_sec += ts.tv_nsec / 1000000000L;
        ts.tv_nsec = ts.tv_nsec % 1000000000L;

        int rc = pthread_cond_timedwait(&this->recvCOND, &this->recvMUX, &ts);
        //TODO check rc
        if (rc == ETIMEDOUT) {
            cout << "[ERROR] Timeout while receiving board telemetry" << endl;
        } else {
            if (this->len > 0) {
                if ((response != NULL)) {
// TODO check if command is correct
                    //if ((this->recvResponse[5]) == (int)str[5]) {
                        timespec recvTS;
                        clock_gettime(0, &recvTS);
                        float delay = (recvTS.tv_sec - sendTS.tv_sec)*1000000.0 + (recvTS.tv_nsec - sendTS.tv_nsec)/1000.0;
                       // cout << "dt: " << delay << "us" << endl;
                        memcpy(response, this->recvResponse, this->len);
                        *lenResponse = this->len;
                    //cout << "RECV " << (int)this->len << endl;
                        bOk = true;
                        for (int i = 0 ; i < this->len ; i++) {
                            cout << (int)i << "->" << (int)response[i] << endl;
                        }
                   // } else {
                   //   cout << "[WARNING] Wrong variable ID received: " << (int)this->recvResponse[5] << " expected: " << (int)str[5] << endl;
                               // for (int i = 0 ; i<(this->len) ; i++) {
                                //    cout << i << "->" << hex << (int)str[i] << endl;
                       // }
                   // }
                }
            } else {
                cout << "Bad CRC message" << endl;
            }
        }
        }

       // pthread_mutex_unlock(&this->recvMUX);
   // pthread_mutex_unlock(&this->sendMUX);
    }
      //  sem_post(&this->sendSEM);
  //  cout << "UNLOCK" << endl;

    return bOk;
}

bool Driver::setCommand(int cmd_code, int16_t val) {

    bool bOk = false;
    char resp[1024];
//    int lenresp;
    updateSetPoint = false;

    if (this->driverState == DRIVER_STATE_RUNNING) {
       // cout << "SETTING" << endl;
    int8_t str[] = "\x01\x00\x00\x00";
    str[0] = PACKET_CODE_SET;
    str[1] = cmd_code;
    str[2] = (val >> 8) & 0xFF;
    str[3] = (val) & 0xFF;
    send((char *)str, 4, NULL, NULL, 0);

    //TODO check response ACK/NACK
    bOk = true;
    }

    return bOk;
}





//void interruptHandler(int32_t signal) {
//    //TODO check signal
//    cout << "Signal caught, stopping..." << endl;
//    running = false;
//    sem_post(&mainLoop);
//    sem_post(&recvFsmSem);
//}

bool Driver::setFilename(char * fileName) {
    bool bOk = false;
    this->serialDescriptor->setFilename(fileName);

    return bOk;
}

bool Driver::setBaud(int baudRate) {
    bool bOk = false;
    this->serialDescriptor->setBaud(baudRate);

    return bOk;
}


bool Driver::fetchRegister(char * buffer, int requestLen, char * response, int * len) {
    bool bOk = false;
    if (this->driverState == DRIVER_STATE_RUNNING) {
        bOk = send(buffer, requestLen, response, len, 1);

    } else {
        cout << "[ERROR] Driver is not running" << endl;
    }

    return bOk;
}


bool Driver::fetchRegister2(char * buffer, int requestLen, char * response, int * len) {
    bool bOk = false;
    if (this->driverState == DRIVER_STATE_RUNNING) {
        bOk = send(buffer, requestLen, response, len, 1);

    } else {
        cout << "[ERROR] Driver is not running" << endl;
    }

    return bOk;
}

bool Driver::writeRegister(char * buffer, int len) {
    bool bOk = false;
    char response[255];
    int respLen;

    if (this->driverState == DRIVER_STATE_RUNNING) {
        bOk = send(buffer, len, response, &respLen,0);

    }

    return bOk;
}




 int Driver::getRequest(int var_id) {
     bool bOk = false;
     int val;

     //cout << "-->> GETREQUEST" << endl;

     char response[255];
     char request[] = "\x03\x00";
     request[1] = var_id;
     int len;
     bOk = fetchRegister2(request, 2, response, &len);
     if (bOk) val = (((uint8_t)response[len-3]));
     pthread_mutex_lock(this->var_mux);
     this->shared_vars[response[5]].value = val;
     pthread_mutex_unlock(this->var_mux);

     return val;
 }


 bool Driver::setVar(TelemetrySharedVariable *var,  pthread_mutex_t * mux) {
    this->shared_vars = var;
     this->var_mux = mux;
    // cout << "setting vars" << endl;
    // for (int i = 0 ; i < 8 ; i++) {
    //     cout << var[i].code << endl;
    // }
 }
