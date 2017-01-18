#include "d2bd/SerialPortManager.h"

using namespace d2bd;

#include <string>
#include "d2bd/SerialPort.h"


void sigpoll(int fd)
{
  SerialPortManager::instance().sigPoll(fd);
}

SerialPortManager& SerialPortManager::instance()
{
	static SerialPortManager instance_;
	return instance_;
}

void SerialPortManager::sigPoll(int fd)
{
  printf("Received sigpoll on fd");
}

//---------------------------------------------
#ifdef TARGET_WIN32
//---------------------------------------------

//------------------------------------
   // needed for serial bus enumeration:
   //4d36e978-e325-11ce-bfc1-08002be10318}
   DEFINE_GUID (GUID_SERENUM_BUS_ENUMERATOR, 0x4D36E978, 0xE325,
   0x11CE, 0xBF, 0xC1, 0x08, 0x00, 0x2B, 0xE1, 0x03, 0x18);
//------------------------------------


void SerialPortManager::enumerateWin32Ports(){

    // thanks joerg for fixes...

	if (bPortsEnumerated == true) return;

	HDEVINFO hDevInfo = NULL;
	SP_DEVINFO_DATA DeviceInterfaceData;
	int i = 0;
	DWORD dataType, actualSize = 0;
	unsigned char dataBuf[MAX_PATH + 1];

	// Reset Port List
	nPorts = 0;
	// Search device set
	hDevInfo = SetupDiGetClassDevs((struct _GUID *)&GUID_SERENUM_BUS_ENUMERATOR,0,0,DIGCF_PRESENT);
	if ( hDevInfo ){
      while (TRUE){
         ZeroMemory(&DeviceInterfaceData, sizeof(DeviceInterfaceData));
         DeviceInterfaceData.cbSize = sizeof(DeviceInterfaceData);
         if (!SetupDiEnumDeviceInfo(hDevInfo, i, &DeviceInterfaceData)){
             // SetupDiEnumDeviceInfo failed
             break;
         }

         if (SetupDiGetDeviceRegistryProperty(hDevInfo,
             &DeviceInterfaceData,
             SPDRP_FRIENDLYNAME,
             &dataType,
             dataBuf,
             sizeof(dataBuf),
             &actualSize)){

			sprintf(portNamesFriendly[nPorts], "%s", dataBuf);
			portNamesShort[nPorts][0] = 0;

			// turn blahblahblah(COM4) into COM4

            char *   begin    = NULL;
            char *   end    = NULL;
            begin          = strstr((char *)dataBuf, "COM");


            if (begin)
                {
                end          = strstr(begin, ")");
                if (end)
                    {
                      *end = 0;   // get rid of the )...
                      strcpy(portNamesShort[nPorts], begin);
                }
                if (nPorts++ > MAX_SERIAL_PORTS)
                        break;
            }
         }
            i++;
      }
   }
   SetupDiDestroyDeviceInfoList(hDevInfo);

   bPortsEnumerated = false;
}


//---------------------------------------------
#endif
//---------------------------------------------



//----------------------------------------------------------------
SerialPortManager::SerialPortManager()
:portsEnumerated_(false)
{}

//----------------------------------------------------------------
SerialPortManager::~SerialPortManager()
{
	closeAllPorts();
}

/**
 * Close a port by removing it from the openPorts list an deleting the structure. This
 * method should only be called from the SerialPort itself once all of its observers
 * have been removed.
 */
/*
void SerialPortManager::close(SerialPort *port)
{
	if(openPorts==NULL || port==NULL) return;
	
	bool found = false;
	
	if(port==openPorts)
	{
		openPorts = openPorts->next;
		found = true;
	}
	else
	{
		SerialPort *temp = openPorts;
		
		do
		{
			if(port==temp->next)
			{
				temp->next = port->next;
				found = true;
				break;
			}
		}
		while(temp->next!=NULL);
	}
	
	if(found)
	{
		delete port;
	}
}
 */

/**
 * Closes all ports, called on shutdown of port manager
 */
void SerialPortManager::closeAllPorts()
{
/*
	SerialPort *port;
	 
	while (openPorts != NULL)
	{
		port = openPorts;
		openPorts = openPorts->next;
		
		port->close();
		
		delete port;
	}
  */
}


//----------------------------------------------------------------
void SerialPortManager::enumerate()
{
#if defined(__APPLE__)
  // List dir
  DIR* dir;
  struct dirent* entry;

  dir = opendir("/dev");
  
  char path[256];
  strcpy(path, "/dev/");
  char* pathEnd = path + strlen(path);
  
  portNames_.clear();
  portPaths_.clear();

  if (dir == 0L){
    printf("SerialPortManager: error listing devices in /dev");
  } else {
    printf("SerialPortManager: listing devices\n");
    while ((entry = readdir(dir)) != 0L) {
      if (isPortNameCompatible(entry->d_name)) {
        portNames_.push_back(entry->d_name);
        strcpy(pathEnd, entry->d_name);
        portPaths_.push_back(path);
      }
    }
  }
  
	//---------------------------------------------
#endif
    //---------------------------------------------

	//---------------------------------------------
	#if defined( TARGET_LINUX )
	//---------------------------------------------

		//----------------------------------------------------
		//We will find serial devices by listing the directory

		DIR *dir;
		struct dirent *entry;
		dir = opendir("/dev");
		string str			= "";
		string device		= "";
		int deviceCount		= 0;

		if (dir == NULL){
			ofLog(OF_LOG_ERROR,"SerialPortManager: error listing devices in /dev");
		} else {
			printf("SerialPortManager: listing devices\n");
			while ((entry = readdir(dir)) != NULL){
				str = (char *)entry->d_name;
				if(portNameFilter(str)){
					printf("device %i - %s\n", deviceCount, str.c_str());
					portNamesFriendly[deviceCount++] = (char *)entry->d_name;
				}
			}
		}
	
		nPorts = deviceCount;

	//---------------------------------------------
	#endif
	//---------------------------------------------

	//---------------------------------------------
	#ifdef TARGET_WIN32
	//---------------------------------------------

		enumerateWin32Ports();
		printf("SerialPortManager: listing devices (%i total)\n", nPorts);
		for (int i = 0; i < nPorts; i++){
			printf("device %i -- %s", i, portNamesFriendly[i]);
		}

	//---------------------------------------------
    #endif
    //---------------------------------------------

}



//----------------------------------------------------------------
size_t SerialPortManager::getNumPorts() const
{
	return portNames_.size();
}

/*
SerialPort* SerialPortManager::getPort(int deviceNumber, SerialPort::PortType type, int baud)
{
	string portName = getPortName(deviceNumber);
	
	if(portName.length()>0)
		return getPort(portName, type, baud);
	else
		return NULL;
}
	   
SerialPort* SerialPortManager::getPort(string portName, SerialPort::PortType type, int baud)
{
	SerialPort *port;
	
	// Check if the port is already open
	for (port = openPorts; port != NULL; port = port->next)
	{
    if (portName.compare(port->name()) == 0 && port->isOpen())
    {
      if(port->baud() == baud && port->getType() == type)
        return port;
			
			// Port is open but at a different settings
      return NULL;
    }
  }
	
	return NULL;
}
*/
std::string SerialPortManager::getPortNameAt(int index) const
{
  std::string portName;

  if (index < portNames_.size()) {
    portName = portNames_[index];
  }
  
  return portName;
}

std::string SerialPortManager::getPortPathAt(int index) const
{
  std::string portPath;

  if (index < portPaths_.size()) {
    portPath = portPaths_[index];
  }
  
  return portPath;
}

#define INVALID_NAME -1
#define PORT_OPEN -2
#define PORT_REF -3

SerialPort* SerialPortManager::OpenPort(int index, int baud,
  const SerialPortOptions& options)
{
	std::string port_path = getPortPathAt(index);
	
	if (!port_path.empty()) {
		return OpenPort(port_path, baud, options);
	}
  
  return nullptr;
}	

SerialPort* SerialPortManager::OpenPort(const std::string& portName, int baud,
  const SerialPortOptions& user_options)
{
  // Remove any dead ports
	prunePorts();

  std::unique_ptr<SerialPort> port;
  
	if (portName.length() == 0) {
    return nullptr;
  }

#if defined(__APPLE__) || defined( TARGET_LINUX )

		printf("SerialPortManagerInit: opening port %s @ %d bps\n", portName.c_str(), baud);
		int fd = open(portName.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
		
		if (fd == -1) {
			printf("SerialPortManager: unable to open port\n");
			return nullptr;
		}

		struct termios options;
		tcgetattr(fd, &oldoptions);
		options = oldoptions;
  
		switch(baud){
		   case 300: 	cfsetispeed(&options,B300);
						cfsetospeed(&options,B300);
						break;
		   case 1200: 	cfsetispeed(&options,B1200);
						cfsetospeed(&options,B1200);
						break;
		   case 2400: 	cfsetispeed(&options,B2400);
						cfsetospeed(&options,B2400);
						break;
		   case 4800: 	cfsetispeed(&options,B4800);
						cfsetospeed(&options,B4800);
						break;
		   case 9600: 	cfsetispeed(&options,B9600);
						cfsetospeed(&options,B9600);
						break;
		   case 14400: 	cfsetispeed(&options,B14400);
						cfsetospeed(&options,B14400);
						break;
		   case 19200: 	cfsetispeed(&options,B19200);
						cfsetospeed(&options,B19200);
						break;
		   case 28800: 	cfsetispeed(&options,B28800);
						cfsetospeed(&options,B28800);
						break;
		   case 38400: 	cfsetispeed(&options,B38400);
						cfsetospeed(&options,B38400);
						break;
		   case 57600:  cfsetispeed(&options,B57600);
						cfsetospeed(&options,B57600);
						break;
		   case 62500:  cfsetispeed(&options,B62500);
						cfsetospeed(&options,B62500);
						break;
		   case 115200: cfsetispeed(&options,B115200);
						cfsetospeed(&options,B115200);
						break;

			default:	cfsetispeed(&options,B9600);
						cfsetospeed(&options,B9600);
						printf("SerialPortManagerInit: cannot set %i baud setting baud to 9600\n", baud);
						break;
		}

  
    options.c_cflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

    options.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                    // enable reading
    options.c_cflag &= ~(PARENB | PARODD);      // shut off parity
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CRTSCTS;
  
  
    options.c_cflag &= ~CSIZE;
    if (user_options.data_size == 7) {
      options.c_cflag |= CS7; // 7 data bits
    } else {
      options.c_cflag |= CS8; // 8 data bits
    }

    switch (user_options.parity) {
      case kSerialPortParityEven:
        options.c_cflag |= PARENB;
        options.c_cflag &= ~PARODD;
        break;
      case kSerialPortParityOdd:
        options.c_cflag |= PARENB;
        options.c_cflag |= PARODD;
        break;
      case kSerialPortParityMark:
        options.c_cflag &= ~PARENB;
        options.c_cflag |= CSTOPB;
        options.c_cflag &= ~CSIZE;
        options.c_cflag |= CS7;
        break;
      default:
        options.c_cflag &= ~PARENB;
    }
  
    if (user_options.stop_bits == 2) {
      options.c_cflag |= CSTOPB;  // 2 Stop bits
    } else {
      options.c_cflag &= ~CSTOPB; // 1 Stop bit
    }
  
    // Enable hardware flow control
    //options.c_cflag |= CRTSCTS;//CNEW_RTSCTS;

		options.c_cc[VMIN] = 0;
		options.c_cc[VTIME] = (int)(user_options.timeout * 10); // Tenths of a second
		
		if(tcsetattr(fd, TCSANOW, &options) < 0) {
			int error = errno;
			printf("SerialPortManagerInit: error %i in port settings.\n", error);
		} else {
    
      //signal(SIGPOLL, sigpoll);

      /* request SIGPOLL signal when receive data is available */
      //ioctl(fd, I_SETSIG, S_INPUT | S_HIPRI);
      
      
      //fcntl(fd, F_SETFL, FNDELAY);             /* Configure port reading */
      /*
      memset(&theTermios, 0, sizeof(struct termios));
      cfmakeraw(&theTermios);
      cfsetspeed(&theTermios, 115200);
      
      theTermios.c_cflag = CREAD | CLOCAL;     // turn on READ
      theTermios.c_cflag |= CS8;
      theTermios.c_cc[VMIN] = 0;
      theTermios.c_cc[VTIME] = 10;     // 1 sec timeout
      ioctl(fileDescriptor, TIOCSETA, &theTermios);
    */
      printf("sucess in opening serial connection");
      
      // Add the port to the list of open ports
      port.reset(new SerialPort);
      port->init(portName, fd, &oldoptions, baud);
    }

	//---------------------------------------------
#endif
    //---------------------------------------------


    //---------------------------------------------
#ifdef TARGET_WIN32
	//---------------------------------------------

		// open the serial port:
		// "COM4", etc...

		HANDLE hComm=CreateFileA(portName.c_str(),GENERIC_READ|GENERIC_WRITE,0,0,
						OPEN_EXISTING,0,0);

		if(hComm==INVALID_HANDLE_VALUE){
			ofLog(OF_LOG_ERROR,"SerialPortManager: unable to open port");
			return false;
		}


		// now try the settings:
		COMMCONFIG cfg;
		DWORD cfgSize;
		char  buf[80];

		cfgSize=sizeof(cfg);
		GetCommConfig(hComm,&cfg,&cfgSize);
		int bps = baud;
		sprintf(buf,"baud=%d parity=N data=8 stop=1",bps);

		#if (_MSC_VER)       // microsoft visual studio
			// msvc doesn't like BuildCommDCB,
			//so we need to use this version: BuildCommDCBA
			if(!BuildCommDCBA(buf,&cfg.dcb)){
				ofLog(OF_LOG_ERROR,"SerialPortManager: unable to build comm dcb; (%s)",buf);
			}
		#else
			if(!BuildCommDCB(buf,&cfg.dcb)){
				ofLog(OF_LOG_ERROR,"SerialPortManager: Can't build comm dcb; %s",buf);
			}
		#endif


		// Set baudrate and bits etc.
		// Note that BuildCommDCB() clears XON/XOFF and hardware control by default

		if(!SetCommState(hComm,&cfg.dcb)){
			ofLog(OF_LOG_ERROR,"SerialPortManager: Can't set comm state");
		}
		//ofLog(OF_LOG_NOTICE,buf,"bps=%d, xio=%d/%d",cfg.dcb.BaudRate,cfg.dcb.fOutX,cfg.dcb.fInX);

		// Set communication timeouts (NT)
		COMMTIMEOUTS tOut;
		GetCommTimeouts(hComm,&oldTimeout);
		tOut = oldTimeout;
		// Make timeout so that:
		// - return immediately with buffered characters
		tOut.ReadIntervalTimeout=MAXDWORD;
		tOut.ReadTotalTimeoutMultiplier=0;
		tOut.ReadTotalTimeoutConstant=0;
		SetCommTimeouts(hComm,&tOut);

		// Add the port to the list of open ports
    // Add the port to the list of open ports
    std::unique_ptr<SerialPort> port(new SerialPort);
		port->init(portName, hComm, &oldTimeout, baud);
	//---------------------------------------------
#endif

	if (port.get() != nullptr) {
    open_ports_.push_back(std::move(port));
  }
	
  return open_ports_.back().get();
}

void SerialPortManager::prunePorts()
{
  for (auto it = open_ports_.begin(); it != open_ports_.end(); ++it) {
    if (!(*it)->isOpen()) {
      // TODO: close
    }
  }
}


