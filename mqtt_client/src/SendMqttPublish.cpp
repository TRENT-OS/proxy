
#include "Socket.h"
#include "MqttCloud.h"

#include <chrono>
#include <thread>

using namespace std;

int main(int argc, const char *argv[])
{
    int port = SERVER_PORT;
    string hostName {"127.0.0.1"};

    if (argc > 2)
    {
        hostName = string{argv[2]};
    }


    vector<char> publish =
    {
       0x32, 0x29, 0x00, 0x21, 0x64, 0x65, 0x76, 0x69, 
       0x63, 0x65, 0x73, 0x2f, 0x74, 0x65, 0x73, 0x74, 
       0x5f, 0x64, 0x65, 0x76, 0x2f, 0x6d, 0x65, 0x73, 
       0x73, 0x61, 0x67, 0x65, 0x73, 0x2f, 0x65, 0x76, 
       0x65, 0x6e, 0x74, 0x73, 0x2f, 0x00, 0x02, 0x63, 
       0x69, 0x61, 0x6f
    };

    while (true)
    {
        Socket socket {port, hostName};
        int result = socket.Write(publish);
        if (result < 0)
        {
            fprintf(stderr,"Error: write to socket\n");
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    return 0;
}
