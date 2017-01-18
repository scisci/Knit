#ifndef D2BD_SERIAL_PORT_MANAGER_H
#define D2BD_SERIAL_PORT_MANAGER_H



#if defined(__APPLE__) || defined( TARGET_LINUX )
	
	#include <termios.h>
	#include <sys/ioctl.h>
	#include <getopt.h>
	#include <dirent.h>
  #include <signal.h>
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
    #endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>

#include <vector>
#include <memory>



namespace d2bd
{
	class SerialPort;
  
  enum SerialPortParity {
    kSerialPortParityNone,
    kSerialPortParityEven,
    kSerialPortParityOdd,
    kSerialPortParityMark,
    kSerialPortParitySpace
  };
  
  struct SerialPortOptions {
    SerialPortOptions()
    :parity(kSerialPortParityNone),
     timeout(2.0),
     data_size(8),
     stop_bits(1)
    {}

    SerialPortParity parity;
    double timeout;
    int data_size;
    int stop_bits;
  };

#define MAX_SERIAL_PORTS 256
#define MAX_PATH 256

// Added because termios only has standard rates
#define B62500 62500

// notes below
//----------------------------------------------------------------------

class SerialPortManager {

public:
	static SerialPortManager& instance();

	void enumerate();
	
	//SerialPort* getPort(const std::string& portName, int baud);
	//SerialPort* getPort(int deviceNumber, int baud);

	std::string getPortNameAt(int index) const;
  std::string getPortPathAt(int index) const;

	SerialPort* OpenPort(const std::string& portName, int baud, const SerialPortOptions& options);
	SerialPort* OpenPort(int deviceNumber, int baud, const SerialPortOptions& options);
	
	void closeAllPorts();
	
	/** Removes ports that have closed in the meantime */
	void prunePorts();

	size_t getNumPorts() const;
  
  void sigPoll(int fd);

private:
	SerialPortManager();
	~SerialPortManager();
  
  inline bool isPortNameCompatible(const char*);
	

#ifdef TARGET_WIN32
	void 		enumerateWin32Ports();
#else
	struct		termios oldoptions;
#endif

  std::vector<std::unique_ptr<SerialPort>> open_ports_;
  std::vector<std::string> portNames_;
  std::vector<std::string> portPaths_;
  bool portsEnumerated_;

};

inline bool SerialPortManager::isPortNameCompatible(const char* portName)
{
  return strncmp("cu.", portName, 3) == 0 || strncmp("tty.", portName, 4) == 0;
}


	
} // namespace d2bd

//----------------------------------------------------------------------



// this serial code contains small portions of the following code-examples:
// ---------------------------------------------------
// http://todbot.com/arduino/host/arduino-serial/arduino-serial.c
// web.mac.com/miked13/iWeb/Arduino/Serial%20Write_files/main.cpp
// www.racer.nl/docs/libraries/qlib/qserial.htm
// ---------------------------------------------------

// notes:
// ----------------------------
// when calling setup("....") you need to pass in
// the name of the com port the device is attached to
// for example, on a mac, it might look like:
//
// 		setup("/dev/tty.usbserial-3B1", 9600)
//
// and on a pc, it might look like:
//
// 		setup("COM4", 9600)
//
// if you are using an arduino board, for example,
// you should check what ports you device is on in the
// arduino program

// to do:
// ----------------------------
// a) 	support blocking / non-blocking
// b) 	support numChars available type functions
// c)   can we reduce the number of includes here?

// 	useful :
// 	http://en.wikibooks.org/wiki/Serial_Programming:Unix/termios
// 	http://www.keyspan.com/downloads-files/developer/win/USBSerial/html/DevDocsUSBSerial.html
// ----------------------------
// (also useful, might be this serial example - worth checking out:
// http://web.mit.edu/kvogt/Public/osrc/src/
// if has evolved ways of dealing with blocking
// and non-blocking instances)
// ----------------------------


#endif //SERIAL_PORT_MANAGER_H
