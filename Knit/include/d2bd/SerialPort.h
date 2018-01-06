/*
 *  SerialPort.h
 *  ribbondevice
 *
 *  Created by Daniel Riley on 4/4/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef D2BD_SERIAL_PORT_H
#define D2BD_SERIAL_PORT_H

#if defined(__APPLE__) || defined(TARGET_LINUX)
	#include <termios.h>
	#include <sys/ioctl.h>
	#include <getopt.h>
	#include <dirent.h>
#else
	#include <winbase.h>
	#include <tchar.h>
	#include <iostream>
	#include <string.h>
	#include <setupapi.h>
	#include <regstr.h>
	#include <winioctl.h>
#ifdef __MINGW32__
  #define INITGUID
  #include <initguid.h> // needed for dev-c++ & DEFINE_GUID
#endif // __MINGW32__
#endif

#include <string>
#include <thread>
#include "d2bd/Threads.h"

#define SERIAL_PORT_BUFFER_SIZE 65536UL


namespace d2bd {

	
class SerialPort {
public:
  typedef std::function<void (const unsigned char *, unsigned int)> ReceivedBytesHandler;
  
	SerialPort()
  :inited_(false),
	 bufferPos_(0),
   writeBufferLength_(0)
	{}
	
	virtual ~SerialPort()
	{
		if (isOpen()) {
			close();
    }
	}
	
	// and unlock in order to write to that data
	// otherwise it's possible to get crashes.
	std::string getName() const { return name_; }
	int getBaudRate() const { return baudRate_; }
	bool isOpen() const {return inited_; }
  
  ssize_t queueBytes(const unsigned char* bytes, size_t size);

	void 			close();
	int 			readBytes(int length);
	int 			WriteBytes(const unsigned char * buffer, int length);
	//void			writeBytes(const unsigned char * buffer, int length);
	bool			writeByte(unsigned char singleByte);
	int             readByte();  // returns -1 on no read or error...
	void			flush(bool flushIn = true, bool flushOut = true);
	int				available();
	
	Thread thread_;
	bool			threadRunning;
	bool			cancelThread;

	static void *thread_handler(void *serial_port)
	{
		reinterpret_cast<SerialPort *>(serial_port)->runThread();
		
		return 0;
	}
	
	void runThread(void)
	{
    for (;;) {
      if (available()) {
        bool done = false;
        
        do {
          done = readBytes(SERIAL_PORT_BUFFER_SIZE - bufferPos_) <= 0;
        }
        while (bufferPos_ < SERIAL_PORT_BUFFER_SIZE && !done);

        // Notify and clear buffer
        if (bufferPos_ > 0) {
          received_bytes_handler_(buffer_, bufferPos_);
          bufferPos_ = 0;
        }
      }
      
      // TODO: check if we need to send anything
      write_mutex_.lock();
      
      if (writeBufferLength_ > 0) {
        int bytesWritten = WriteBytes(writeBuffer_, writeBufferLength_);
        if (bytesWritten < writeBufferLength_ && bytesWritten > 0) {
          memmove(writeBuffer_, &writeBuffer_[bytesWritten], writeBufferLength_ - bytesWritten);
        }
        writeBufferLength_ -= bytesWritten;
      }
      
      write_mutex_.unlock();
    }
	}
	
	void startThread(void)
	{ 
		if (inited_) {
      printf("SerialPort::Initializing/starting thread...");
      
      thread_.start(&SerialPort::thread_handler, this);
		} else {
      printf("SerialPort::Can't start thread, it is already running.");
    }

	}
	


	
#ifdef TARGET_WIN32
	bool init(std::string portName, HANDLER portHandle, COMMTIMEOUTS *t, int baud);
#else

	bool init(const std::string& portName, int portHandle, struct termios *oldoptions, int baud);
#endif

  void SetReceivedBytesHandler(ReceivedBytesHandler handler)
  {
    received_bytes_handler_ = handler;
  }
	
protected:
	std::string name_;
	int baudRate_;
	bool inited_;
	unsigned char buffer_[SERIAL_PORT_BUFFER_SIZE];
	unsigned int bufferPos_;
  
  unsigned char writeBuffer_[SERIAL_PORT_BUFFER_SIZE];
  unsigned int writeBufferLength_;

  std::mutex write_mutex_;
  
  ReceivedBytesHandler received_bytes_handler_;

#ifdef TARGET_WIN32
	HANDLE  	hComm;              // the handle to the serial port pc
	COMMTIMEOUTS 	*oldTimeout;		// we alter this, so keep a record	
#else
	int 		fd;					// the handle to the serial port mac
	struct 	termios *oldoptions;
#endif
	
	/** A thread to read the port. I believe SerialPort could simply extend thread but not sure */
	/*
	class PortThread : public OpenThreads::Thread
	{
	public:
		PortThread(SerialPort* port):
		_port(port)
		{
		}
		
		virtual void run()
		{	

			if(_port->useFT)
			{
				FT_STATUS ftStatus;
				
#ifdef TARGET_WIN32				
#else

				DWORD rxBytes;
				EVENT_HANDLE eh;
				
				pthread_mutex_init(&eh.eMutex,NULL);
				pthread_cond_init(&eh.eCondVar,NULL);

				ftStatus = FT_SetEventNotification(_port->ft, FT_EVENT_RXCHAR, (PVOID)&eh);
				
				if(ftStatus != FT_OK)
				{
					printf("SerialPort::Failed to set event notification.\r\n");
				}
				
				for(;;)
				{
					pthread_mutex_lock(&eh.eMutex);
					//eMutex->lock();
					
					ftStatus = FT_GetQueueStatus(_port->ft,&rxBytes);
					
					if(rxBytes == 0)
					{
						printf("SerialPort::Waiting...\r\n");
						
						pthread_cond_wait(&eh.eCondVar, &eh.eMutex);
						//eCondVar->wait(eMutex);
						if(!testCancel())
							ftStatus = FT_GetQueueStatus(_port->ft,&rxBytes);
					}
					
					pthread_mutex_unlock(&eh.eMutex);
					//eMutex->unlock();
					
					if(testCancel())
						break;
					
					printf("SerialPort::Reading...\r\n");
					
					while(rxBytes)
					{
						// Calculate the number of bytes to read, don't go over the buffer size
						DWORD readCount = rxBytes<SERIAL_PORT_BUFFER_SIZE?rxBytes:SERIAL_PORT_BUFFER_SIZE;
						// A variable to store the number of bytes read
						DWORD bytesRead = 0;
						
						printf("\tstart read\r\n");
						ftStatus = FT_Read(_port->ft,_port->_buffer,readCount,&bytesRead);
						printf("\tend read\r\n");
						
						if(ftStatus!=FT_OK)
						{
							printf("SerialPort::FT_Read failed!\r\n");
						}
						
						if(bytesRead)
						{
							_port->notify(bytesRead);
						}
						else
						{
							printf("SerialPort::No bytes read.\r\n");
						}
						
						rxBytes -= bytesRead;
					}
					
					
				}
				
				printf("SerialPort::Quit thread..\r\n");
#endif
			}
			else
			{
			
				do
				{
					// Check that the buffer is not full
					if(_port->_bufferPosition<SERIAL_PORT_BUFFER_SIZE-1)
					{
						// Attempt to read some bytes
						if(_port->readBytes(SERIAL_PORT_BUFFER_SIZE-_port->_bufferPosition)>0)
							_port->notify(_port->_bufferPosition);
					}

					
					//
					// Check if there are bytes to write
					//if(_port->_writeBufferPosition>0)
					//{
						// Attempt to write the bytes
					//	_port->_writeBufferPosition -= _port->_WriteBytes(_port->_writeBuffer,_port->_writeBufferPosition);
					//}

					
				} while (!testCancel());
			}
		}
		
		
		
	protected:
		SerialPort* _port;
		
	} _thread;
	*/
};
	
} // namespace d2bd

#endif // D2BD_SERIAL_PORT_H

