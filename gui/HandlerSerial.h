/*
 * HandlerSerial.h - DEVE embedded designs 2018
 *
 *  Created on: 	Mar 28, 2018
 *  Author: 		xavier.descarrega
 *
 *  Description: 	Generic Serial Port handler for Windows
 *
 *  Changelog:
 *  				xavier.descarrega 03/28/18
 *  				Creation. Basic local loop test at high throughput
 */

#ifndef _HANDLERSERIAL_H_
#define _HANDLERSERIAL_H_

#include <errno.h>   /* Error number definitions */
#include <fcntl.h>   /* File control definitions */
#include <iostream>
#include <sys/types.h>
#include <sys/file.h> /* flock */
#include <string.h> /* memset */
#include <unistd.h> /* close */
#include <stdlib.h>
#include <windows.h>
#include <vector>

#define MAXEVENTS 64

const int MAX_FILENAME_STRING = 255;
const int MAX_READ_BYTES = 1024;

enum HandlerSerialOpenError {
	ALREADY_OPEN = 1,
	MISSING_PARAMS,
	OPEN_FAILED,
	LOCK_FAILED,
	GET_ATTRIBUTES_FAILED,
	SET_SPEED_FAILED,
	SET_ATTRIBUTES_FAILED,
	FLUSH_FAILED
};


enum HandlerSerialState {
	HANDLER_SERIAL_OPENING = 1,
	HANDLER_SERIAL_STREAMING,
	HANDLER_SERIAL_CLOSING,
	HANDLER_SERIAL_CLOSED,
	HANDLER_SERIAL_ERROR
};

using namespace std ;

class HandlerSerial {

	private:

        char filename_[MAX_FILENAME_STRING];
		bool defFilename_;
		uint32_t baud_;
		bool defBaud_;
        HANDLE fileDescriptor_;
        OVERLAPPED osRead, osWrite;
		HandlerSerialState status_;

		uint8_t open();
		bool close();

		int32_t baudToSpeed(uint32_t baud);


	public:

		HandlerSerial();
		~HandlerSerial();

		uint8_t start();
		bool stop();
		bool reset();
		bool ready();


        int32_t recv(char * buffer, uint32_t len);
        int32_t send(char * buffer, uint32_t len);


		bool setBaud(uint32_t);
        bool setFilename(char * port);

		uint32_t getBaud();
        bool getFilename(char * port);
        vector<int> getAvailablePorts();


};



#endif /* _HANDLERSERIAL_H_ */
