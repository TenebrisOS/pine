#include "pine.h"
#include <iostream>
#include <ostream>
#include <stdio.h>

#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t

using enum PINE::PCSX2::IPCCommand;

#define PINE_DEFAULT_SLOT 28011

// a portable sleep function
auto msleep(int sleepMs) -> void {
#ifdef _WIN32
    Sleep(sleepMs);
#else
    usleep(sleepMs * 1000);
#endif
}

// this function is an infinite loop reading the game title, this shows you
// how timers can work
auto read_background(PINE::PCSX2 *ipc) -> void {
    while (true) {
        // you can go slower but go higher at your own risk
        msleep(100);

        try {
            // WARNING: all datastreams that are returned by the library changes
            // ownership, it is your duty to free them after use.
            char *title = ipc->GetGameTitle();
            printf("%s\n", title);
            delete[] title;
        } catch (...) {
            // if the operation failed
            printf("ERROR!!!!!\n");
        }
    }
}

static bool sendPINECommand(int slot, unsigned char command)  
{  
#ifdef _WIN32  
    WSADATA wsa = {};  
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)  
        return false;  
  
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);  
    if (sock == INVALID_SOCKET)  
    {  
        WSACleanup();  
        return false;  
    }  
  
    sockaddr_in server = {};  
    server.sin_family = AF_INET;  
    server.sin_addr.s_addr = htonl(INADDR_LOOPBACK);  
    server.sin_port = htons(slot);  
  
    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR)  
    {  
        printf("Failed to connect to PINE server on port %u\n", slot);  
        closesocket(sock);  
        WSACleanup();  
        return false;  
    }  
    
    printf("Connected to PINE server\n");
  
    // Build message: [4-byte size] [command byte]
    u8 buffer[5];
    u32 size = 1;  // Just the command byte
    
    // Little-endian size
    buffer[0] = size & 0xFF;  
    buffer[1] = (size >> 8) & 0xFF;  
    buffer[2] = (size >> 16) & 0xFF;  
    buffer[3] = (size >> 24) & 0xFF;  
    buffer[4] = command;  
    
    printf("Sending command: 0x%02x with size=%u\n", command, size);  
    printf("Raw bytes: %02x %02x %02x %02x %02x\n",   
           buffer[0], buffer[1], buffer[2], buffer[3], buffer[4]);
  
    int bytes_sent = send(sock, (const char*)buffer, 5, 0);  
    printf("Bytes sent: %d\n", bytes_sent);
    
    bool success = (bytes_sent == 5);
    
    // Try to read response
    u8 response[512];
    int bytes_recv = recv(sock, (char*)response, sizeof(response), 0);
    if (bytes_recv > 0)
    {
        printf("Received response: %d bytes\n", bytes_recv);
        if (bytes_recv >= 5)
        {
            printf("Response: %02x %02x %02x %02x %02x\n",   
                   response[0], response[1], response[2], response[3], response[4]);
        }
    }
    
    closesocket(sock);  
    WSACleanup();  
#else  
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);  
    if (sock < 0)  
        return false;  
  
    std::string socket_name;  
    char* runtime_dir = nullptr;  
#ifdef __APPLE__  
    runtime_dir = std::getenv("TMPDIR");  
#else  
    runtime_dir = std::getenv("XDG_RUNTIME_DIR");  
#endif  
    if (runtime_dir == nullptr)  
        socket_name = "/tmp/pcsx2.sock";  
    else  
    {  
        socket_name = runtime_dir;  
        socket_name += "/pcsx2.sock";  
    }  
  
    if (slot != PINE_DEFAULT_SLOT)  
        socket_name += "." + std::to_string(slot);  
  
    struct sockaddr_un server;  
    server.sun_family = AF_UNIX;  
    strncpy(server.sun_path, socket_name.c_str(), sizeof(server.sun_path) - 1);  
  
    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0)  
    {  
        close(sock);  
        return false;  
    }  
  
    u8 buffer[5];
    u32 size = 1;
    buffer[0] = size & 0xFF;  
    buffer[1] = (size >> 8) & 0xFF;  
    buffer[2] = (size >> 16) & 0xFF;  
    buffer[3] = (size >> 24) & 0xFF;  
    buffer[4] = command;
    
    printf("Sending command: 0x%02x with size=%u\n", command, size);
    
    int bytes_sent = write(sock, buffer, 5);  
    printf("Bytes sent: %d\n", bytes_sent);
    
    bool success = (bytes_sent == 5);
    
    u8 response[512];
    int bytes_recv = read(sock, response, sizeof(response));
    if (bytes_recv > 0)
    {
        printf("Received response: %d bytes\n", bytes_recv);
        if (bytes_recv >= 5)
        {
            printf("Response: %02x %02x %02x %02x %02x\n",   
                   response[0], response[1], response[2], response[3], response[4]);
        }
    }
    
    close(sock);  
#endif  
  
    return success;  
}


// the main function that is executed at the start of our program
auto main(int argc, char *argv[]) -> int {

    int slot = PINE_DEFAULT_SLOT;
    unsigned char command = 0x67;   // example command byte

    printf("Sending PINE command...\n");

    bool ok = sendPINECommand(slot, command);

    if (ok) {
        printf("Command sent successfully\n");
    } else {
        printf("Failed to send command\n");
    }

    /*
    while (true) {
        sendPINECommand(slot, command);
        msleep(1000);
    }
    */

    return 0;
}