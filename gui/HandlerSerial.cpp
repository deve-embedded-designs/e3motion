/*
 * HandlerSerial.cpp - DEVE embedded designs 2018
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

#include "HandlerSerial.h"


HandlerSerial::HandlerSerial() {
    this->fileDescriptor_ = 0;
	this->defBaud_ = false;
	this->defFilename_ = false;
	this->status_ = HANDLER_SERIAL_CLOSED;
    this->baud_ = 0;
    memset(&osRead, 0, sizeof(osRead));
    memset(&osWrite, 0, sizeof(osWrite));
    osRead.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    osWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
}


HandlerSerial::~HandlerSerial() {
	if ((this->status_ != HANDLER_SERIAL_CLOSING)&&(this->status_ != HANDLER_SERIAL_CLOSED)) {
		cout << "[WARNING] The handler should be stopped before deleting it" << endl;
		close();
	}
    CloseHandle(osRead.hEvent);
    CloseHandle(osWrite.hEvent);
}


vector<int> HandlerSerial::getAvailablePorts() {
    bool bSuccess;
    char szPort[32];
    vector<int> availPorts;

    for (int i = 0 ; i < 256 ; i++) {
        sprintf(szPort, "\\\\.\\COM%u", i);
        bSuccess = false;
        HANDLE port = CreateFileA(szPort, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);

        if (port == INVALID_HANDLE_VALUE)
        {
          DWORD dwError = GetLastError();

          //Check to see if the error was because some other app had the port open or a general failure
          if (dwError == ERROR_ACCESS_DENIED || dwError == ERROR_GEN_FAILURE || dwError == ERROR_SHARING_VIOLATION || dwError == ERROR_SEM_TIMEOUT) {
            bSuccess = true;
          }
        }
        else
        {
          //The port was opened successfully
          bSuccess = true;
        }

        //Add the port number to the array which will be returned
        if (bSuccess) {
            availPorts.push_back(i);
        }
        CloseHandle(port);
    }

    return availPorts;
}


/*
 * open - Opens the file descriptor as a serial port with the previously configured parameters
 *
 */
uint8_t HandlerSerial::open()
{
    char szPort[255];

    sprintf(szPort, "\\\\.\\%s", this->filename_);

    cout << this->filename_ << endl;
    cout << "Opening serial port: " << szPort << " @ " << this->baud_ << "bps";

	if ((this->status_ == HANDLER_SERIAL_STREAMING)&&((this->status_ == HANDLER_SERIAL_OPENING)))
	{
		cout << ", FAILED!" << endl << "[ERROR] Port already open, state" << this->status_ << endl;
		return ALREADY_OPEN;
	}

	this->status_ = HANDLER_SERIAL_OPENING;

    if (!defBaud_ || !defFilename_)
    {
        cout << ", FAILED!" << endl << "[ERROR] Baud rate or filename not defined" << errno << endl;
        return MISSING_PARAMS;
    }

    this->fileDescriptor_ = CreateFileA(szPort,
                                        GENERIC_READ | GENERIC_WRITE,
                                        0,
                                        0,
                                        OPEN_EXISTING,
                                        FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED,
                                        0
                                        );

    if (this->fileDescriptor_ == INVALID_HANDLE_VALUE) {
        cout << ", FAILED!" << endl << "[ERROR] Unable to open port, error: " << GetLastError() << endl;
        return OPEN_FAILED;
    }


    DCB dcbConfig;

    if(GetCommState(this->fileDescriptor_, &dcbConfig))
    {
        //TODO use configured baudrate
        dcbConfig.BaudRate = CBR_256000;
        dcbConfig.ByteSize = 8;
        dcbConfig.Parity = NOPARITY;
        dcbConfig.StopBits = ONESTOPBIT;
        dcbConfig.fBinary = TRUE;
        dcbConfig.fParity = FALSE;
    }

    else {
        cout << "[ERROR] Getting port parameters" << endl;
        CloseHandle(this->fileDescriptor_);
        return OPEN_FAILED;
    }
    if(!SetCommState(this->fileDescriptor_, &dcbConfig)) {
        cout << "[ERROR] Setting port parameters" << endl;
        CloseHandle(this->fileDescriptor_);
        return OPEN_FAILED;
    }

    COMMTIMEOUTS commTimeout;



    if(GetCommTimeouts(this->fileDescriptor_, &commTimeout))
    {
        commTimeout.ReadIntervalTimeout     = MAXDWORD;
        commTimeout.ReadTotalTimeoutConstant     = 0;
        commTimeout.ReadTotalTimeoutMultiplier     = 0;
        commTimeout.WriteTotalTimeoutConstant     = 0;
        commTimeout.WriteTotalTimeoutMultiplier = 0;
    }

    else {
        cout << "[ERROR] Getting port timeouts" << endl;
        CloseHandle(this->fileDescriptor_);
        return OPEN_FAILED;
    }

    if(!SetCommTimeouts(this->fileDescriptor_, &commTimeout)) {
         cout << "[ERROR] Setting port timeouts" << endl;
         CloseHandle(this->fileDescriptor_);
         return OPEN_FAILED;
    }

    DWORD dwStoredFlags = EV_BREAK | EV_CTS | EV_DSR | EV_ERR | EV_RING | EV_RLSD | EV_RXCHAR | EV_RXFLAG | EV_TXEMPTY ;

    if (!SetCommMask(this->fileDescriptor_, dwStoredFlags)) {
          cout << "[ERROR] Error setting mask" << endl;
          CloseHandle(this->fileDescriptor_);
          return OPEN_FAILED;
    }

    if (!PurgeComm(this->fileDescriptor_, PURGE_RXABORT | PURGE_TXABORT | PURGE_RXCLEAR | PURGE_TXCLEAR)) {
        cout << "[ERROR] Error purging, error: "<< GetLastError() << endl;
        CloseHandle(this->fileDescriptor_);
        return OPEN_FAILED;
    }

	this->status_ = HANDLER_SERIAL_STREAMING;

	cout << ", OK!" << endl;

	return 0;
}


/*
 * close
 *
 */
bool HandlerSerial::close()
{

	bool bOk = false;


	if ((this->status_ != HANDLER_SERIAL_CLOSING)&&(this->status_ != HANDLER_SERIAL_CLOSED)) {
        cout << "Closing serial port..." << endl;
		this->status_ = HANDLER_SERIAL_CLOSING;

        if(this->fileDescriptor_ != INVALID_HANDLE_VALUE)
        {
            CloseHandle(this->fileDescriptor_);
            this->fileDescriptor_ = INVALID_HANDLE_VALUE;
        }


		this->status_ = HANDLER_SERIAL_CLOSED;
		bOk = true;
	}


	return bOk;
}


/*
 * baudToSpeed - return the associated constant to a particular baudrate, 0 if non-existent
 *
 */
int32_t HandlerSerial::baudToSpeed(uint32_t baud) {

	switch(baud) {
    /*
        case 9600   : return B9600   ; break ;
        case 19200  : return B19200  ; break ;
        case 38400  : return B38400  ; break ;
        case 57600  : return B57600  ; break ;
		case 115200 : return B115200 ; break ;
		case 230400 : return B230400 ; break ;
        case 460800 : return B460800 ; break ;
		case 921000 : return B921600 ; break ;
        */
	}

	return 0;
}


uint8_t HandlerSerial::start() {
	return open();

}


bool HandlerSerial::stop() {
	return close();

}


bool HandlerSerial::reset() {
	bool bOk;

	bOk = close();
	bOk = (!open());

	return bOk;
}


bool HandlerSerial::ready() {
	return (this->status_ == HANDLER_SERIAL_STREAMING);
}


int32_t HandlerSerial::recv(char * buffer, uint32_t len) {
    DWORD dwBytesRead;
    DWORD      dwCommEvent;
    int nbytes = 0;
    DWORD ret;


    if (!ReadFile(this->fileDescriptor_,buffer, len, &dwBytesRead,&osRead)) {
    }
    if (!WaitCommEvent(this->fileDescriptor_,&dwCommEvent, &osRead)) {
        ret = GetLastError();
        if (ret != 997) {
            cout << "ERROR " << ret << endl;
        }
    }
    ret = WaitForSingleObject(osRead.hEvent,INFINITE);
    switch (ret) {
        case WAIT_OBJECT_0:
            nbytes = dwBytesRead;
            break;
        case WAIT_TIMEOUT:
            // Handle timeout
            break;
        case WAIT_FAILED:
            // Handle failure
            break;
        default:
            // what else to handle?
            break;
    }
    ResetEvent ( osRead.hEvent );

    return nbytes;
}


int32_t HandlerSerial::send(char * buffer, uint32_t len) {


       DWORD dwWritten;
       DWORD dwRes;
       bool fRes;


       if (!WriteFile(this->fileDescriptor_, buffer, len, &dwWritten, &osWrite)) {
          if (GetLastError() != ERROR_IO_PENDING) {
             // WriteFile failed, but isn't delayed. Report error and abort.
             fRes = FALSE;
             cout << "[ERROR]"  <<endl;
          }
          else
             // Write is pending.
             dwRes = WaitForSingleObject(osWrite.hEvent, INFINITE);
             switch(dwRes)
             {
                // OVERLAPPED structure's event has been signaled.
                case WAIT_OBJECT_0:
                     if (!GetOverlappedResult(this->fileDescriptor_, &osWrite, &dwWritten, FALSE))
                           fRes = FALSE;
                     else
                      // Write operation completed successfully.
                      fRes = TRUE;
                     break;

                default:
                     // An error has occurred in WaitForSingleObject.
                     // This usually indicates a problem with the
                    // OVERLAPPED structure's event handle.
                     fRes = FALSE;
                     break;
             }
          }

       else
          // WriteFile completed immediately.
          fRes = TRUE;

       ResetEvent(osWrite.hEvent);
       return fRes;

}


bool HandlerSerial::setBaud(uint32_t baud)
{
	bool bOk = false;
	defBaud_ = true;
	baud_ = baud;
	bOk = true;

	return bOk;
}


bool HandlerSerial::setFilename(char * port)
{
	bool bOk = false;
	defFilename_ = true;
	// TODO do not like that!!!
	strcpy(filename_, port);
	bOk = true;

	return bOk;
}


uint32_t HandlerSerial::getBaud() {
	return this->baud_;
}


bool HandlerSerial::getFilename(char * filename) {
	bool bOk = false;
	strcpy(filename, this->filename_);
	bOk = true;

	return bOk;
}



