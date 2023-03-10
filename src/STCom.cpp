#include <stdlib.h>  // for EXIT_SUCCESS

#include "server/Server.h"
#include "client/Client.h"

int main(int argc, const char* argv[]) {
    // ENet init.
    if (enet_initialize() != 0)
    {
        fprintf(stderr, "An error occurred while initializing ENet.\n");
        return EXIT_FAILURE;
    }
    else
    {
        printf_s("Enet initialized successfuly.\n");
    }
    atexit(enet_deinitialize);

    // Start server or client.
    if (argc > 1 && !strcmp(argv[1], "-server"))
    {
        printf_s("Starting server mode.\n");

        stc::Server ServerInstance;
        if (ServerInstance.InitConnection())
        {
            ServerInstance.Run();
        }
        else
        {
            printf_s("Server cannot be initialized.\n");
            return EXIT_FAILURE;
        }
    }
    else
    {
        printf_s("Starting client mode.\n");

        stc::Client ClientInstance;
        ClientInstance.Run();
    }

    return EXIT_SUCCESS;
}
