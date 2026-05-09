#include <horizon/NotificationSender.hpp>
#include <iostream>
#include <unistd.h>

int main()
{
    std::cout << "Sending test notification..." << std::endl;
    
    horizon::NotificationSender::send(
        "Horizon System", 
        "Esta es una notificación de prueba desde el core.",
        "utilities-terminal",
        5000
    );

    std::cout << "Done." << std::endl;
    return 0;
}
