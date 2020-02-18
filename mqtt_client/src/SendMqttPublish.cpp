
#include "Channel.h"
#include "MqttCloud.h"

#include <chrono>
#include <thread>

using namespace std;

int main(int argc, const char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: SendMqttPublish host [port] [delay between publishes in seconds]\n");
        return 0;
    }

    string hostName {"127.0.0.1"};
    hostName = string{argv[1]};

    int port = 7999;
    if (argc > 2)
    {
        port = atoi(argv[2]);
    }

    int delay = 10;
    if (argc > 3)
    {
        delay = atoi(argv[3]);
    }

    printf("Publishing to host: %s port: %d with delay: %d", hostName.c_str(), port, delay);

    vector<char> publish =
    {
       0x32, 0x29, 0x00, 0x21, 0x64, 0x65, 0x76, 0x69, 
       0x63, 0x65, 0x73, 0x2f, 0x74, 0x65, 0x73, 0x74, 
       0x5f, 0x64, 0x65, 0x76, 0x2f, 0x6d, 0x65, 0x73, 
       0x73, 0x61, 0x67, 0x65, 0x73, 0x2f, 0x65, 0x76, 
       0x65, 0x6e, 0x74, 0x73, 0x2f, 0x00, 0x02, 0x63, 
       0x69, 0x61, 0x6f
    };

    //printf("SendMqttPublish to port: make a pause before starting...\n");
    //std::this_thread::sleep_for(std::chrono::milliseconds(1000 * 15));

    while (true)
    {
        Channel socket {port, hostName};

        //printf("SendMqttPublish: created socket - make a pause before writing...\n");
        //std::this_thread::sleep_for(std::chrono::milliseconds(1000 * 15));

        int result = socket.Write(publish);
        if (result < 0)
        {
            fprintf(stderr,"Error: write to socket\n");
        }

        //printf("SendMqttPublish: data written - make a pause before closing the socket...\n");
        //std::this_thread::sleep_for(std::chrono::milliseconds(1000 * 15));

        printf("SendMqttPublish: socket closed - make a pause before creating next socket...\n");
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 * delay));
    }

    return 0;
}
