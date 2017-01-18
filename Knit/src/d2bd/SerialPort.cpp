/*
 *  SerialPort.cpp
 *  ribbondevice
 *
 *  Created by Daniel Riley on 4/4/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include "d2bd/SerialPort.h"
#include "d2bd/SerialPortManager.h"

using namespace d2bd;


#ifdef TARGET_WIN32
bool SerialPort::init(string portName, HANDLE portHandle, COMMTIMEOUTS * timeouts, int baud);
{
	if(!bInited && !threadRunning/*_thread.isRunning()*/)
	{
		bInited = true;
		hComm = portHandle;
		baudRate = baud;
		oldTimeout = timeouts;
		
		//_thread.startThread();
		
		return true;
	}
	
	return false;
		
}
#else
bool SerialPort::init(const std::string& portName, int portHandle, struct termios *oldops, int baud)
{
	if (!inited_) {
		inited_ = true;
		fd = portHandle;
		baudRate_ = baud;
		oldoptions = oldops;
    
    startThread();

		return true;
	}
	
	return false;
}
#endif



//----------------------------------------------------------------
void SerialPort::close()
{
	
	if (inited_) {
		inited_ = false;

    //stopThread();

      
      //---------------------------------------------
#ifdef TARGET_WIN32
      //---------------------------------------------
      
			SetCommTimeouts(hComm,oldTimeout);
			CloseHandle(hComm);
			hComm 		= INVALID_HANDLE_VALUE;
      
      //---------------------------------------------
#else
      //---------------------------------------------
			tcsetattr(fd,TCSANOW,oldoptions);
			::close(fd);
      // [CHECK] -- anything else need to be reset?
      //---------------------------------------------
#endif
      //---------------------------------------------

		
	//	SerialPortManager::instance().prunePorts();

	}
	
	
}

/*
void SerialPort::writeBytes(const unsigned char * buffer, int length)
{
	if(length>SERIAL_PORT_BUFFER_SIZE-_writeBufferPosition)
		return;
	
	for(int i=0;i<length;i++)
		_writeBuffer[_writeBufferPosition++] = buffer[i];
}
 */

//----------------------------------------------------------------
int SerialPort::WriteBytes(const unsigned char * buffer, int length){
	
	if (!inited_){
		printf("SerialPort: serial not inited");
		return 0;
	}
	
	//---------------------------------------------
#if defined(__APPLE__) || defined( TARGET_LINUX )
		ssize_t bytes_written = write(fd, buffer, length);
		if(bytes_written <= 0){
			printf("SerialPort: Can't write to com port\n");
      
      if (bytes_written < 0) {
        printf("SerialPortError: %i\n", errno);
      }
      
			return 0;
		}
		
		//printf("SerialPort: numWritten %i", bytes_written);
		
		return (int)bytes_written;
#endif
	//---------------------------------------------
	
	//---------------------------------------------
#ifdef TARGET_WIN32
		DWORD written;
		if(!WriteFile(hComm, buffer, length, &written,0)){
			printf("SerialPort: Can't write to com port\n");
			return 0;
		}
		printf("SerialPort: numWritten %i", (int)written);
		return (int)written;
#endif
	//---------------------------------------------
	
	
}

//----------------------------------------------------------------
int SerialPort::readBytes(int length){
	
	if (!inited_){
		printf("SerialPort: serial not inited");
		return 0;
	}

	//---------------------------------------------
#if defined( __APPLE__ ) || defined( TARGET_LINUX )
		ssize_t nRead = read(fd, buffer_, length);
		if(nRead < 0){
			printf("SerialPort: trouble reading from port");
			return 0;
		}
		bufferPos_ += nRead;
		return (int)nRead;
		
#endif
	//---------------------------------------------
	
	//---------------------------------------------
#ifdef TARGET_WIN32
	DWORD nRead = 0;
	if (!ReadFile(hComm,_buffer,length,&nRead,0)){
		printf("SerialPort: trouble reading from port");
		return 0;
	}
	_bufferPosition += (int)nRead;
	return (int)nRead;
#endif
	//---------------------------------------------
}

ssize_t SerialPort::queueBytes(const unsigned char* bytes, size_t size)
{
  size_t numBytes = SERIAL_PORT_BUFFER_SIZE - writeBufferLength_;
  if (size < numBytes) {
    numBytes = size;
  }
  
  write_mutex_.lock();
  memcpy(&writeBuffer_[writeBufferLength_], bytes, numBytes);
  writeBufferLength_ += numBytes;
  write_mutex_.unlock();
  
  return numBytes;
}

//----------------------------------------------------------------
bool SerialPort::writeByte(unsigned char singleByte){
	
	if (!inited_){
		printf("SerialPort: serial not inited");
		return false;
	}
	
	
	
	unsigned char tmpByte[1];
	tmpByte[0] = singleByte;
	
	//---------------------------------------------
#if defined( __APPLE__ ) || defined( TARGET_LINUX )
		ssize_t numWritten = 0;
		numWritten = write(fd, tmpByte, 1);
		if(numWritten <= 0 ){
			printf("SerialPort: Can't write to com port");
			return false;
		}
		printf("SerialPort: written byte");
		
		
		return (numWritten > 0 ? true : false);
#endif
    //---------------------------------------------
	
    //---------------------------------------------
#ifdef TARGET_WIN32
		DWORD written = 0;
		if(!WriteFile(hComm, tmpByte, 1, &written,0)){
			printf("SerialPort: Can't write to com port");
			return false;
		}
		
		printf("SerialPort: written byte");
		
		return ((int)written > 0 ? true : false);
#endif
	//---------------------------------------------
	
}

//----------------------------------------------------------------
int SerialPort::readByte(){
	
	if (!inited_){
		printf("SerialPort: serial not inited");
		return 0;
	}
	
	unsigned char tmpByte[1];
	memset(tmpByte, 0, 1);
	//---------------------------------------------
#if defined( __APPLE__ ) || defined( TARGET_LINUX )
		ssize_t nRead = read(fd, tmpByte, 1);
		if(nRead < 0){
			printf("SerialPort: trouble reading from port");
			return -1;
		}
		if(nRead == 0) {
			return -2;
    }
#endif
	//---------------------------------------------
	
	//---------------------------------------------
#ifdef TARGET_WIN32
		DWORD nRead;
		if (!ReadFile(hComm, tmpByte, 1, &nRead, 0)){
			printf("SerialPort: trouble reading from port");
			return OF_SERIAL_ERROR;
		}
#endif

	return (int)(tmpByte[0]);
}


//----------------------------------------------------------------
void SerialPort::flush(bool flushIn, bool flushOut){
	
	if (!inited_){
		printf("SerialPort: serial not inited");
		return;
	}
	
	int flushType = 0;

	
	//---------------------------------------------
#if defined( __APPLE__ ) || defined( TARGET_LINUX )
		if( flushIn && flushOut) flushType = TCIOFLUSH;
		else if(flushIn) flushType = TCIFLUSH;
		else if(flushOut) flushType = TCOFLUSH;
		else return;

		tcflush(fd, flushType);
#endif
	//---------------------------------------------
	
	//---------------------------------------------
#ifdef TARGET_WIN32
		if( flushIn && flushOut) flushType = PURGE_TXCLEAR | PURGE_RXCLEAR;
		else if(flushIn) flushType = PURGE_RXCLEAR;
		else if(flushOut) flushType = PURGE_TXCLEAR;
		else return;
		
		PurgeComm(hComm, flushType);
#endif

}

//-------------------------------------------------------------
int SerialPort::available(){
	
	if (!inited_){
		printf("SerialPort: serial not inited");
		return 0;
	}
	
	int numBytes = 0;
	
	//---------------------------------------------
#if defined( __APPLE__ ) || defined( TARGET_LINUX )
	ioctl(fd,FIONREAD,&numBytes);
#endif
	//---------------------------------------------
	
	//---------------------------------------------
#ifdef TARGET_WIN32
	COMSTAT stat;
	DWORD err;
	if(hComm!=INVALID_HANDLE_VALUE){
		if(!ClearCommError(hComm, &err, &stat)){
			numBytes = 0;
		} else {
			numBytes = stat.cbInQue;
		}
	} else {
		numBytes = 0;
	}
#endif
    //---------------------------------------------
	
	return numBytes;
}


