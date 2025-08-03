
#include <iostream>
#include <network.h>

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

// Configura el terminal para entrada sin bloqueo
void SetNonBlockingInput() {
    termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag &= ~(ICANON | ECHO); // Modo sin buffer y sin eco
    tcsetattr(STDIN_FILENO, TCSANOW, &term);

    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK); // No bloquear lectura
}



enum class CustomMsgTypes : u_int32_t
{
    ServerAccept,
    ServerDeny,
    ServerPing,
    PackageAll,
    ServerPackage,
};

class CustomClient : public network::client_interface<CustomMsgTypes>
{
public:
    void PingServer()
    {
        network::package<CustomMsgTypes> pkg;
        pkg.header.id = CustomMsgTypes::ServerPing;

        std::string s = "Ping";
        int a = 20;
        bool b = true;
        
        pkg << a << b;
        Send(pkg);
        SendUDP(pkg);
    }

    void SendToAll(){
        network::package<CustomMsgTypes> pkg;
        pkg.header.id = CustomMsgTypes::PackageAll;

        int a = 30;

        pkg << a;

        Send(pkg);
        
    }

    void SendUDPtoServer()
    {
        network::package<CustomMsgTypes> pkg;
        pkg.header.id = CustomMsgTypes::ServerPackage;

        SendUDP(pkg);

    }

};

std::atomic<bool> bQuit = false;

void InputThread(CustomClient& c, std::atomic<bool>& bQuit)
{
    SetNonBlockingInput();

    while (!bQuit)
    {
        char ch;
        if (read(STDIN_FILENO, &ch, 1) > 0)
        {
            if (ch == ' ')
            {
                std::cout << "[Cliente] Enviando ping\n";
                c.PingServer();
            }
            else if (ch == 'h')
            {
                c.SendToAll();
            }
            else if (ch == 'u')
            {
                c.SendUDPtoServer();
            }
            else if (ch == 'q')
            {
                bQuit = true;
                break;
            }
        }
        usleep(10000);
    }
}


int main()
{
    CustomClient c;
    c.Connect("127.0.0.1", 60000, 60001);

    std::thread input_thread(InputThread, std::ref(c), std::ref(bQuit));

    while (c.IsConnected() && !bQuit)
    {
        c.Incoming().wait();

        while (!c.Incoming().empty())
        {
            auto pkg = c.Incoming().pop_front().pkg;

            if(pkg.header.type == network::package_type::server_assign_id)
            {
                c.HandleServerAssignID(pkg);
                continue;
            }



            switch (pkg.header.id)
            {
                case CustomMsgTypes::ServerAccept:
                    std::cout << "Server Accepted Connection\n";
                    break;
                case CustomMsgTypes::ServerPing:
                {
                    std::string s;
                    int a;
                    bool b;
                    //pkg.read(s);
                    pkg >> b >> a;
                    std::cout << "Respuesta del servidor: " << a<< " "<< b<< std::endl;
                }
                break;
                case CustomMsgTypes::PackageAll:
                {
                    std::cout << "[CLIENT] Recibido paquete de tipo PackageAll\n";

                    uint32_t msg;
                    pkg >> msg;
                    std::cout << "[CLIENT] Mensaje recibido: " << msg << "\n";

                }
                break;
            }
        }
    }

    input_thread.join();
}