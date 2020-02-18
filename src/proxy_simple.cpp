

#include "GuestConnector.h"
#include "ServerSocket.h"
#include "SharedResource.h"
#include "Channel.h"
#include "MqttCloud.h"
#include "ChannelAdmin.h"
#include "LanServerChannel.h"
#include "LibDebug/Debug.h"
#include "uart_socket_guest_rpc_conventions.h"
#include "utils.h"
#include <chrono>
#include <thread>
#include <unistd.h>

#include "ChannelCreators.h"

#define LAN_PORT 7999

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

void WriteToGuest(SharedResource<PseudoDevice> *pseudoDevice, unsigned int logicalChannel, const vector<char> &buffer)
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

void SendResponse(unsigned int logicalChannel, ChannelAdmin *channelAdmin, UartSocketGuestSocketCommand command, vector<char> result)
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
        channelAdmin->GetPseudoDevice(),
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
                         ChannelAdmin *channelAdmin,
                         vector<char> &buffer,
                         ChannelCreators* channelCreators)
{
    if (IsControlChannel(logicalChannel))
    {
        if (buffer.size() != 2)
        {
            Debug_LOG_ERROR("[channel %u] invalid control channel buffer size %lu",
                            logicalChannel, buffer.size());
            return;
        }

        UartSocketGuestSocketCommand command = static_cast<UartSocketGuestSocketCommand>(buffer[0]);
        unsigned int commandLogicalChannel = buffer[1];
        vector<char> result(7,0);

        Debug_LOG_INFO("[channel %u] control command for channel %d: %d (%s)",
            logicalChannel,
            commandLogicalChannel,
            command,
            UartSocketCommand(command).c_str());

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
                result[0] = channelAdmin->ActivateChannel(commandLogicalChannel,
                                                        channelCreators->getCreator(commandLogicalChannel)->Create());
                result[0] = result[0] < 0 ? 1 : 0;
                Debug_LOG_INFO("exit Activate Socket\n");
            }
            else if (command == UART_SOCKET_GUEST_CONTROL_SOCKET_COMMAND_CLOSE)
            {
                channelAdmin->RequestClose(commandLogicalChannel);
                result[0] = 0;
            }
            else
            {
                /* handle payload as command */
                OutputDevice *channel = channelAdmin->GetChannel(commandLogicalChannel);
                if (channel)
                {
                    result = channel->HandlePayload(buffer);
                }
            }
        }
        // Debug_LOG_INFO("Handle channel command: result:%d\n", result);

        //SendResponse(channelAdmin, command, PARAM(result, result < 0 ? 1 : 0));
        SendResponse(logicalChannel, channelAdmin, command, result);
    }
    else if (IsDataChannel(logicalChannel))
    {
        channelAdmin->SendDataToChannel(logicalChannel, buffer);
    }
    else
    {   // Command Channel
        OutputDevice *channel = channelAdmin->GetChannel(logicalChannel);
        if (!channel)
        {   // if channel was not created than let's do it by activating it
            if (!channelAdmin->ActivateChannel(logicalChannel,
                                               channelCreators->getCreator(logicalChannel)->Create()))
            {
                channel = channelAdmin->GetChannel(logicalChannel);
                Debug_ASSERT(channel != NULL);
            }
        }
        WriteToGuest(channelAdmin->GetPseudoDevice(),
                     logicalChannel,
                     channel->HandlePayload(buffer));
    }
}

static bool KeepFromGuestThreadAlive = true;

// "RX" only = it receives all data from the guest (=seL4) and a) puts it into the appropriate channel or b) executes the received control command
void FromGuestThread(GuestConnector *guestConnector, ChannelAdmin *channelAdmin, ChannelCreators* channelCreators)
{
    size_t bufSize = 4096;
    vector<char> buffer(bufSize);
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
                                    channelAdmin,
                                    buffer,
                                    channelCreators);
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

void LanServer(ChannelAdmin *channelAdmin, unsigned int lanPort)
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

            if (channelAdmin->GetChannel(logicalChannel) == nullptr)
            {
                Debug_LOG_INFO("LanServer: start server thread: in port %d, in address %x (file descriptor: %x)\n", clientAddress.sin_port, clientAddress.sin_addr.s_addr, newsockfd);
                channelAdmin->ActivateChannel(logicalChannel, new LanServerChannel{newsockfd});
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

int main(int argc, char *argv[])
{
    // set default values
    DeviceType type = DEVICE_TYPE_SOCKET;
    string connectionType = "TCP";
    string connectionParam = "4444";
    int lanPort = LAN_PORT;
    int port = SERVER_PORT;
    string hostName {SERVER_NAME};
    int use_tap = 0;

    int opt;
    char delim[] = ":";
    while((opt = getopt(argc, argv, "c:l:p:t:d:h")) != -1)
    {
        switch(opt)
        {
            case 'c':
                connectionType = strtok(optarg, delim);
                if (connectionType == "TCP")
                {
                    type = DEVICE_TYPE_SOCKET;
                }
                else if (connectionType == "PTY")
                {
                    type = DEVICE_TYPE_PSEUDO_CONSOLE;
                }
                else if (connectionType == "UART")
                {
                    type = DEVICE_TYPE_RAW_SERIAL;

                }
                else
                {
                    printf("Unknown Device parameter %s\n", connectionType.c_str());
                    printf("Possible options are: 'UART', 'PTY', 'TCP'\n");
                    break;
                }
                connectionParam = strtok(NULL, delim);
                break;
            case 'l':
                lanPort = atoi(optarg);
                break;
            case 'p':
                port = atoi(optarg);
                break;
            case 't':
                use_tap = atoi(optarg);
                break;
            case 'd':
                hostName = optarg;
                break;
            case 'h':
                printf("Usage: -c [<connectionType>:<Param>] -l [lan port] -d [cloud_host_name] -p [cloud_port] -t [tap_number]\n");
                return 0;
            case '?':
                printf("unknown option: %c\n", optopt);
                break;
        }
    }
    // optind is for the extra arguments which are not parsed
    for(; optind < argc; optind++){
        printf("extra arguments: %s\n", argv[optind]);
    }

    printf("Starting proxy app on lan port: %d of type %s with connection param: %s using cloud host: %s port: %d use_pico:%d, use_tap:%d \n",
        lanPort,
        connectionType.c_str(),
        connectionParam.c_str(),
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

    if(use_pico ==1)
    {
        new thread{PicoTickThread};
    }

    /* TBD: is this for overloading / redirecting the socket function calls for PICO TCP? */
    in_the_stack =1;       // it must be 1 for host system = linux

    /* Shared resource used because multithreaded access to pseudodevice not working. With QEMU using sockets: may not be needed any more.*/
    PseudoDevice parsedDevice{&connectionParam, &type};
    SharedResource<PseudoDevice> pseudoDevice{&parsedDevice};

    GuestConnector guestConnector{&pseudoDevice, GuestConnector::GuestDirection::FROM_GUEST};
    if (!guestConnector.IsOpen())
    {
        Debug_LOG_FATAL("Could not open pseudo device.\n");
        return 0;
    }


   ChannelAdmin channelAdmin{&pseudoDevice};

   // The "GUEST thread" is:
   // a) receiving all hdlc frames and distributing them to the channels
   // b) handling the channel admin commands from the guest

   ChannelCreators channelCreators(hostName, port, use_pico, use_tap);

   thread fromGuestThread{
       FromGuestThread,
       &guestConnector,
       &channelAdmin,
       &channelCreators};

    // Handle the LAN socket
    LanServer(&channelAdmin, lanPort);

    // We only get here if something in the LanServer went wrong.
    KeepFromGuestThreadAlive = false;
    fromGuestThread.join();

    return 0;
}
