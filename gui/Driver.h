#ifndef _DRIVER_H_
#define _DRIVER_H_

#include "HandlerSerial.h"
#include "circularVectorTS.hpp"
#include <pthread.h>
#include <semaphore.h>
#include "ThreadSafeDS/Queue.hpp"
#include <vector>

class Driver {

public:
    typedef struct TelemetrySharedVariable {
        int code;
        int length;
        bool editable;
        char * name;
        int value;
    } TelemetrySharedVariable;

    enum FsmRecvState {
        RECV_HEADER = 1,
        RECV_LENGTH,
        RECV_DATA,
        RECV_CRC_1,
        RECV_CRC_2
    };

    enum DriverReturnParam {
        OK = 1
    };

    enum DriverState {
        DRIVER_STATE_RUNNING = 1,
        DRIVER_STATE_STOPPED,
        DRIVER_STATE_READY
    };

    enum PacketCode {
        PACKET_CODE_GET = 0,
        PACKET_CODE_SET,
        PACKET_CODE_NUM

    };

    enum MessageCode {
        MESSAGE_CODE_FIRMWARE = 0,
        MESSAGE_CODE_SETPOINT = 1,
        MESSAGE_CODE_CONTROL_MODE = 2,
        MESSAGE_CODE_MOTOR_SPEED = 3,
        MESSAGE_CODE_KP = 4,
        MESSAGE_CODE_KI = 5,
        MESSAGE_CODE_KD = 6,
        MESSAGE_CODE_PWM_FREQ = 7,
        MESSAGE_CODE_CURRENT = 8,
        MESSAGE_CODE_VOLTAGE = 9,
        MESSAGE_CODE_TEMPERATURE = 10,
        MESSAGE_CODE_POWER = 11,
        MESSAGE_CODE_COUNTS = 12,
        MESSAGE_CODE_REGEN_BRAKE = 13,
        MESSAGE_CODE_OVERCURRENT = 14,
        MESSAGE_CODE_OVERVOLTAGE = 15,
        MESSAGE_CODE_OVERTEMPERATURE = 16,
        MESSAGE_CODE_RELEASE_SAFETY_BRAKE = 17,
        MESSAGE_CODE_EMERGENCY_BRAKE = 18,
        MESSAGE_CODE_FAULT_FLAGS = 19,
        MESSAGE_CODE_SPWM_FREQ = 20,
        MESSAGE_CODE_OUTPUT_SIGNAL = 21,
        MESSAGE_CODE_NUM    // Leave at the last position

    };

    struct Message {
        char buffer[255];
        int length;
    };


        TelemetrySharedVariable * shared_vars;
         pthread_mutex_t * var_mux;
        HandlerSerial * serialDescriptor;
        pthread_t recvHandlerTID, recvFsmHandlerTID;
        sem_t recvFsmSEM, recvSEM, sendSEM;

        pthread_cond_t recvCOND;
        pthread_mutex_t recvMUX, sendMUX;
        DriverState driverState;
        circularVectorTS<char> * rxBuffer;
        pthread_mutex_t rxBufferMUX;
        char recvResponse[1024];
        int len;
        int setPoint;


        void * (*recvCB)(char * buffer, int len);
        void * cbClass;
        static void * recvHandler(void * params);
        static void * recvFsmHandler(void * params);

private:
        bool fetchRegister(char * buffer, int requestLen, char * response, int * len);
        bool fetchRegister2(char * buffer, int requestLen, char * response, int * len);
        bool writeRegister(char * buffer, int len);

public:



        struct MeasuredVar {
            int id;
            int value;
        };

        struct MeasuredVarArray {
          int num_elements;
          int timestamp;
          vector<MeasuredVar> measures;
        };



    typedef enum ControlMode {
            CONTROL_MODE_OPEN_LOOP = 0,
            CONTROL_MODE_SPEED,
            CONTROL_MODE_POSITION
     } ControlMode;

     typedef enum OutputSignal {
            OUTPUT_SIGNAL_SINUSOIDAL = 0,
            OUTPUT_SIGNAL_TRAPEZOIDAL = 1

     } OutputSignal;

    Driver();

    queueTS<float> * gui_queue, *gui_queue2, *gui_queue3;
    queueTS<MeasuredVarArray> * device_measure_variables;
    //queueTS<Driver::MeasuredVar> * device_measure_variables;

    static int calcCRC(char * buffer, int length);
   // bool setRecvCB(void *(*)(char * buffer, int len), void *);
    bool setRecvCB(void *(*cbFunction)( char *, int));
    bool send(char * buffer, int len, char *, int *, int blocking);
    bool start();
    bool stop();
    bool setFilename(char * fileName);
    bool setBaud(int baudRate);
    bool setCommand(int cmd_code, int16_t val);

    bool setVar(TelemetrySharedVariable * var, pthread_mutex_t *);

    int getRequest(int variable_code);

   /* bool setKp(uint16_t Kp);
    bool setKi(uint16_t Kp);
    bool setKd(uint16_t Kp);
    bool setActiveBrake(bool);
    bool setSetpoint(int16_t setPoint);
    bool setControlMode(ControlMode mode);
    bool setOvercurrent(float);
    bool setOvertemperature(uint8_t);
    bool setOvervoltage(float);
    bool setPWMFreq(uint16_t);
    bool setEmergencyBrake();
    bool setSPWMFrequency(uint32_t);
    bool setOutputSignalType(OutputSignal mode);


    bool getKp(uint16_t * Kp);
    bool getKi(uint16_t * Ki);
    bool getKd(uint16_t * Kd);
    bool getPWMFreq(uint16_t * freq);
    bool getVoltage(float * voltage);
    bool getSpeed(int * speed);
    bool getCurrent(float * current);
    bool getTemperature(float * current);
    bool getControlMode(ControlMode * mode);
    bool getPower(int16_t * setPoint);
    bool getCounts(int32_t *);
    bool getActiveBrake(bool *);
    bool getParkingBrake(bool *);
    bool getOvercurrent(float *);
    bool getOvertemperature(uint8_t *);
    bool getOvervoltage(float *);
    bool getFaultFlags(bool * safetyBrake, bool * emergencyBrake, bool * overcurrent, bool * overvoltage, bool * overtemperature);
    bool getOutputSignalType(OutputSignal * mode);
*/
    vector<int> getAvailablePorts();

   // bool releaseSafetyBrake();
};

#endif
