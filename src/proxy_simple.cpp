

#include "GuestConnector.h"
#include "ServerSocket.h"
#include "Socket.h"
#include "MqttCloud.h"
#include "SocketAdmin.h"
#include "LanServerSocket.h"
#include "CloudSocketCreator.h"
#include "PicoSocket.h"
#include "PicoCloudSocketCreator.h"
#include "LibDebug/Debug.h"
#include "uart_socket_guest_rpc_conventions.h"
#include "utils.h"
#include "TapSocketCreator.h"
#include <chrono>
#include <thread>

int use_pico =0; // By default disable picotcp
extern __thread int in_the_stack;
extern "C" {
extern void pico_tick_thread(void *arg);
extern void pico_wrapper_start();
}
using namespace std;

string UartSocketCommand(UartSocketGuestSocketCommand command)
{
    switch (command)
    {
        case UART_SOCKET_GUEST_CONTROL_SOCKET_COMMAND_OPEN:
        return ("Open");

        case UART_SOCKET_GUEST_CONTROL_SOCKET_COMMAND_OPEN_CNF:
        return ("OpenCnf");

        case UART_SOCKET_GUEST_CONTROL_SOCKET_COMMAND_CLOSE:
        return ("Close");

        case UART_SOCKET_GUEST_CONTROL_SOCKET_COMMAND_CLOSE_CNF:
        return ("CloseCnf");

        default:
        return ("Unknown");
    }
}

void WriteToGuest(SharedResource<string> *pseudoDevice, unsigned int logicalChannel, const vector<char> &buffer)
{
    int writtenBytes;
    GuestConnector guestConnector(pseudoDevice, GuestConnector::GuestDirection::TO_GUEST);

    in_the_stack=1;

    if(use_pico ==1)
    {
    	in_the_stack =0;             // is the per thread variable used by pico-bsd. To use Pico stack it must be zero

    }
    if (!guestConnector.IsOpen())
    {
        Debug_LOG_FATAL("WriteToGuest[%1d]: pseudo device not open.\n", logicalChannel);
        return;
    }

    try
    {
        writtenBytes = guestConnector.Write(PARAM(logicalChannel, logicalChannel), buffer.size(), &buffer[0]);
        if (writtenBytes < 0)
        {
            Debug_LOG_ERROR("WriteToGuest[%1d]: guest write failed.\n", logicalChannel);
        }
    }
    catch (...)
    {
        Debug_LOG_ERROR("WriteToGuest[%1d] exception\n", logicalChannel);
    }
}

void SendResponse(unsigned int logicalChannel, SocketAdmin *socketAdmin, UartSocketGuestSocketCommand command, vector<char> result)
{
    vector<char> response(8,0);


    if (command == UART_SOCKET_GUEST_CONTROL_SOCKET_COMMAND_OPEN)
    {
        response[0] = static_cast<char>(UART_SOCKET_GUEST_CONTROL_SOCKET_COMMAND_OPEN_CNF);
    }

    else if (command == UART_SOCKET_GUEST_CONTROL_SOCKET_COMMAND_GETMAC)
    {
    	response[0] = static_cast<char>(UART_SOCKET_GUEST_CONTROL_SOCKET_COMMAND_GETMAC_CNF);
    	memcpy(&response[2],&result[1],6);

    }

    else
    {
        response[0] = static_cast<char>(UART_SOCKET_GUEST_CONTROL_SOCKET_COMMAND_CLOSE_CNF);
    }

    response[1] = result[0];

    Debug_LOG_INFO("Socket command response: cmd: %s result: %d\n",
        UartSocketCommand(static_cast<UartSocketGuestSocketCommand>(response[0])).c_str(),
        response[1]);

    WriteToGuest(
        socketAdmin->GetPseudoDevice(),
        logicalChannel,
        response);
}

static int IsControlChannel(unsigned int channelId)
{
    return ((channelId == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_CONTROL_CHANNEL) ||
            (channelId == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_CONTROL_CHANNEL_1));
}

void HandleSocketCommand(unsigned int logicalChannel, SocketAdmin *socketAdmin, vector<char> &buffer, IoDeviceCreator *ioDeviceCreator)
{
    if (buffer.size() != 2)
    {
        Debug_LOG_ERROR("FromGuestThread: incoming socket command with wrong length.\n");
        return;
    }


    UartSocketGuestSocketCommand command = static_cast<UartSocketGuestSocketCommand>(buffer[0]);
    unsigned int commandLogicalChannel = buffer[1];
    //int result;
    vector<char> result(7,0);

    Debug_LOG_INFO("Handle socket command: cmd: %s channel: %d\n",
        UartSocketCommand(command).c_str(),
        commandLogicalChannel);

    if (commandLogicalChannel == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_LAN)
    {
        result[0] = 0; // We do not allow the guest to handle the LAN socket -> fake success results
    }
    else if (IsControlChannel(commandLogicalChannel))
    {
        result[0] = -1; // We do not allow the guest to handle the control channel -> return a failure
    }
    else if (commandLogicalChannel == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_WAN)
    {
        if (command == UART_SOCKET_GUEST_CONTROL_SOCKET_COMMAND_OPEN)
        {
        	Debug_LOG_INFO("entry Activate Socket\n");
        	result[0] = socketAdmin->ActivateSocket(UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_WAN, ioDeviceCreator->Create());
        	Debug_LOG_INFO("exit Activate Socket\n");
        }
        else if(command ==UART_SOCKET_GUEST_CONTROL_SOCKET_COMMAND_CLOSE )
        {
            socketAdmin->RequestClose(UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_WAN);
            result[0] = 0;
            std::this_thread::sleep_for(std::chrono::milliseconds(1000 * 2));
        }
        else if(command == UART_SOCKET_GUEST_CONTROL_SOCKET_COMMAND_GETMAC)
        {
             // handle get mac here.
        	OutputDevice *socket = socketAdmin->GetSocket(UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_WAN);
        	vector<char> mac(6,0);
        	socket->getMac("tap0",&mac[0]);
        	printf("Mac read = %x %x %x %x %x %x\n", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
            result[0] = 0;
            memcpy(&result[1],&mac[0],6);
        }

    }

    // Debug_LOG_INFO("Handle socket command: result:%d\n", result);

    //SendResponse(socketAdmin, command, PARAM(result, result < 0 ? 1 : 0));
    SendResponse(logicalChannel, socketAdmin, command, result);
}

static bool KeepFromGuestThreadAlive = true;

// "RX" only = it receives all data from the guest (=seL4) and a) puts it into the appropriate socket or b) executes the received control command
void FromGuestThread(GuestConnector *guestConnector, SocketAdmin *socketAdmin, IoDeviceCreator *ioDeviceCreator)
{
    size_t bufSize = 1024;
    vector<char> buffer(bufSize);
    int readBytes, writtenBytes;
    string s = "FromGuestThread";

    Debug_LOG_INFO("FromGuestThread: starting.\n");
    if(use_pico ==1)
    {
    	in_the_stack =0;             // is the per thread variable used by pico-bsd. To use Pico stack it must be zero
    	pthread_setname_np(pthread_self(), s.c_str());
    }

    try
    {
        while (KeepFromGuestThreadAlive)
        {
            unsigned int logicalChannel;
            buffer.resize(bufSize);
            int readBytes = guestConnector->Read(buffer.size(), &buffer[0], &logicalChannel);

            if (readBytes > 0)
            {
                //dumpFrame(&buffer[0], readBytes);
                buffer.resize(readBytes);

                if (IsControlChannel(logicalChannel))
                {
                    // Handle the commands arriving on the control channel
                    HandleSocketCommand(logicalChannel, socketAdmin, buffer, ioDeviceCreator);
                }
                else
                {
                    // For LAN, WAN and TAP: write the received data to the according socket
                    socketAdmin->SendDataToSocket(logicalChannel, buffer);
                }
            }
            else
            {
                // Not used because: not meaningful "Resource temporarily unavailable" 
                //Debug_LOG_ERROR("GuestConnectorFromGuest: guest read failed.\n");
            }
        }
    }
    catch (...)
    {
        Debug_LOG_ERROR("FromGuestThread exception\n");
    }
}

void LanServer(SocketAdmin *socketAdmin, unsigned int lanPort)
{
    ServerSocket serverSocket(lanPort);
    socklen_t clientLength;
    struct sockaddr_in clientAddress;
    unsigned int logicalChannel = UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_LAN;


    if (!serverSocket.IsOpen())
    {
        Debug_LOG_ERROR("LanServer: Error: could not create server socket\n");
        return;
    }

    serverSocket.Listen(5);

    try
    {
        while (true)
        {
            clientLength = sizeof(clientAddress);
            int newsockfd = serverSocket.Accept((struct sockaddr *) &clientAddress, &clientLength);

            if (socketAdmin->GetSocket(logicalChannel) == nullptr)
            {
                Debug_LOG_INFO("LanServer: start server thread: in port %d, in address %x (file descriptor: %x)\n", clientAddress.sin_port, clientAddress.sin_addr.s_addr, newsockfd);
                socketAdmin->ActivateSocket(logicalChannel, new LanServerSocket{newsockfd});
            }
            else
            {
                Debug_LOG_ERROR("LanServer: Error: do not start a new to-guest thread for LAN because such a thread is already active\n");
                close(newsockfd);
            }
        }
    }
    catch (...)
    {
        Debug_LOG_ERROR("LanServer exception\n");
    }
}

// Tick thread of PicoTCP. Thread called every 1 ms
void PicoTickThread()
{
	string s = "PicoTickThread";
	Debug_LOG_INFO("PicoTickThread: starting.\n");
	pthread_setname_np(pthread_self(), s.c_str());
	pico_tick_thread(NULL);
}

int main(int argc, const char *argv[])
{
	pthread_t ticker;
	if (argc < 2)
    {
        printf("Usage: mqtt_proxy_demo QEMU_pseudo_terminal | QEMU_tcp_port [lan port] [cloud_host_name] [cloud_port] [use_pico] [use_tap]\n");
        return 0;
    }

    string pseudoDeviceName{argv[1]};

    int lanPort = SERVER_PORT;
    if (argc > 2)
    {
        lanPort = atoi(argv[2]);
    }

    string hostName {SERVER_NAME};
    if (argc > 3)
    {
        hostName = string{argv[3]};
    }

    int port = SERVER_PORT;
    if (argc > 4)
    {
        port = atoi(argv[4]);
    }

    if(argc > 5)
    {
    	use_pico = atoi(argv[5]);
    }

    int use_tap = 0;
    if(argc > 6)
    {
    	use_tap = atoi(argv[6]);
    }

    printf("Starting mqtt proxy on lan port: %d with pseudo device: %s using cloud host: %s port: %d use_pico:%d, use_tap:%d \n",
        lanPort, 
        pseudoDeviceName.c_str(),
        hostName.c_str(), 
        port,
        use_pico,
        use_tap);

	pico_wrapper_start();

    thread *pPico_tick = NULL;
  	if(use_pico ==1)
	{
  		pPico_tick = new thread{PicoTickThread};
	}


	in_the_stack =1;       // it must be 1 for host system = linux

    SharedResource<string> pseudoDevice{&pseudoDeviceName};
    GuestConnector guestConnector{&pseudoDevice, GuestConnector::GuestDirection::FROM_GUEST};
    if (!guestConnector.IsOpen())
    {
        Debug_LOG_FATAL("Could not open pseudo device.\n");
        return 0;
    }


   SocketAdmin socketAdmin{&pseudoDevice};

   // The "GUEST thread" is:
   // a) receiving all hdlc frames and distributing them to the sockets
   // b) handling the socket admin commands from the guest

   IoDeviceCreator *pCreator;
   if (use_pico)
   {
	   pCreator = new PicoCloudSocketCreator{port, hostName};
   }
   else if(use_tap == 0 &&  use_pico ==0)
   {
	   pCreator = new CloudSocketCreator{port, hostName};

   }
   else
   {
	   pCreator = new TapSocketCreator("tap0");
   }

   thread fromGuestThread{
	   FromGuestThread,
	   &guestConnector,
	   &socketAdmin,
	   pCreator};

    // Handle the LAN socket
    LanServer(&socketAdmin, lanPort);

    // We only get here if something in the LanServer went wrong.
    KeepFromGuestThreadAlive = false;
    fromGuestThread.join();

    return 0;
}
