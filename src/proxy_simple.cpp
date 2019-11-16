

#include "GuestConnector.h"
#include "ServerSocket.h"
#include "Socket.h"
#include "MqttCloud.h"
#include "SocketAdmin.h"
#include "LanServerSocket.h"
#include "LibDebug/Debug.h"
#include "uart_socket_guest_rpc_conventions.h"
#include "utils.h"
#include <chrono>
#include <thread>

#include "SocketCreators.h"

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
    vector<char> response(2,0);

    if (command == UART_SOCKET_GUEST_CONTROL_SOCKET_COMMAND_OPEN)

    {
        response[0] = static_cast<char>(UART_SOCKET_GUEST_CONTROL_SOCKET_COMMAND_OPEN_CNF);
        printf(" Tx Send response =%d\n",response[0]);
    }
    else if (command == UART_SOCKET_GUEST_CONTROL_SOCKET_COMMAND_GETMAC)
    {
        response.resize(8);
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
    return ((channelId == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_LAN_CONTROL_CHANNEL) ||
            (channelId == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_WAN_CONTROL_CHANNEL) ||
            (channelId == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_CONTROL_NW)          ||
            (channelId == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_CONTROL_NW_2));
}

static int IsDataChannel(unsigned int channelId)
{
    return ((channelId == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_LAN) ||
            (channelId == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_WAN) ||
            (channelId == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_NW)  ||
            (channelId == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_NW_2));
}

void HandleSocketCommand(unsigned int logicalChannel,
                         SocketAdmin *socketAdmin,
                         vector<char> &buffer,
                         SocketCreators* socketCreators)
{
    Debug_LOG_DEBUG("%s: Handling channel %d", __func__, logicalChannel);

    if (IsControlChannel(logicalChannel))
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
        else
        {
            /* Generic handling fo all types of control channels*/
            if (command == UART_SOCKET_GUEST_CONTROL_SOCKET_COMMAND_OPEN)
            {
                Debug_LOG_INFO("entry Activate Socket\n");
                result[0] = socketAdmin->ActivateSocket(commandLogicalChannel,
                                                        socketCreators->getCreator(commandLogicalChannel)->Create());
                result[0] = result[0] < 0 ? 1 : 0;
                Debug_LOG_INFO("exit Activate Socket\n");
            }
            else if (command == UART_SOCKET_GUEST_CONTROL_SOCKET_COMMAND_CLOSE)
            {
                socketAdmin->RequestClose(commandLogicalChannel);
                result[0] = 0;
                /* The reason for this is not 100% known; at the time it seemed more stable to do it. */
                std::this_thread::sleep_for(std::chrono::milliseconds(1000 * 2));
            }
            else
            {
                /* handle payload as command */
                OutputDevice *socket = socketAdmin->GetSocket(commandLogicalChannel);
                if (socket)
                {
                    result = socket->HandlePayload(buffer);
                }
            }
        }
        // Debug_LOG_INFO("Handle socket command: result:%d\n", result);

        //SendResponse(socketAdmin, command, PARAM(result, result < 0 ? 1 : 0));
        SendResponse(logicalChannel, socketAdmin, command, result);
    }
    else if (IsDataChannel(logicalChannel))
    {   // all the code above should at a certain point move to some payload
        // handler and leave in this 2 lines the actual implementation
        OutputDevice *socket = socketAdmin->GetSocket(logicalChannel);
        if (socket)
        {
            socketAdmin->SendDataToSocket(logicalChannel, buffer);
        }
        else
        {
            Debug_LOG_ERROR("%s: socket object not found channel %d", __func__, logicalChannel);
        }
    }
    else
    {   // Command Channel
        OutputDevice *socket = socketAdmin->GetSocket(logicalChannel);
        if (!socket)
        {   // if socket was not created than let's do it by activating it
            if (!socketAdmin->ActivateSocket(logicalChannel,
                                             socketCreators->getCreator(logicalChannel)->Create()))
            {
                socket = socketAdmin->GetSocket(logicalChannel);
                Debug_ASSERT(socket != NULL);
            }
        }
        WriteToGuest(socketAdmin->GetPseudoDevice(),
                     logicalChannel,
                     socket->HandlePayload(buffer));
    }
}

static bool KeepFromGuestThreadAlive = true;

// "RX" only = it receives all data from the guest (=seL4) and a) puts it into the appropriate socket or b) executes the received control command
void FromGuestThread(GuestConnector *guestConnector, SocketAdmin *socketAdmin, SocketCreators* socketCreators)
{
    size_t bufSize = 4096;
    vector<char> buffer(bufSize);
    int readBytes, writtenBytes;
    string s = "FromGuestThread";

    Debug_LOG_INFO("FromGuestThread: starting.\n");

    /* Why is this necessary ? */
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
                // Handle the commands arriving on the control channel
                HandleSocketCommand(logicalChannel,
                                    socketAdmin,
                                    buffer,
                                    socketCreators);
            }
            else
            {
                // Not used because: not meaningful "Resource temporarily unavailable"
                // Debug_LOG_ERROR("GuestConnectorFromGuest: guest read failed.\n");
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


    /* Has to be called because pico_wrapper_start() steals the socket api's to decide to use linux or pico.
     * I see that when we link Pico as static lib, all the socket calls such as socket(), connect(), bind() etc
     * of the pico library is always used instead of the direct socket calls from the C lib. Hence its important to call this
     * without which nothing works.
     */
    pico_wrapper_start();

    thread *pPico_tick = NULL;
    if(use_pico ==1)
    {
        pPico_tick = new thread{PicoTickThread};
    }

    /* TBD: is this for overloading / redirecting the socket function calls for PICO TCP? */
    in_the_stack =1;       // it must be 1 for host system = linux

    /* Shared resource used because multithreaded access to pseudodevice not working. With QEMU using sockets: may not be needed any more.*/
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

   SocketCreators socketCreators(hostName, port, use_pico, use_tap);

   thread fromGuestThread{
       FromGuestThread,
       &guestConnector,
       &socketAdmin,
       &socketCreators};

    // Handle the LAN socket
    LanServer(&socketAdmin, lanPort);

    // We only get here if something in the LanServer went wrong.
    KeepFromGuestThreadAlive = false;
    fromGuestThread.join();

    return 0;
}
