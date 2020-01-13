#include <CUESDK.h>

#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <vector>
#include <atomic>
#include <unordered_set>
#include <thread>

const int port = 1235;

std::atomic_uint8_t red = 100;
std::atomic_uint8_t green = 100;
std::atomic_uint8_t blue = 100;

const char* toString(CorsairError error)
{
    switch (error) {
    case CE_Success:
        return "CE_Success";
    case CE_ServerNotFound:
        return "CE_ServerNotFound";
    case CE_NoControl:
        return "CE_NoControl";
    case CE_ProtocolHandshakeMissing:
        return "CE_ProtocolHandshakeMissing";
    case CE_IncompatibleProtocol:
        return "CE_IncompatibleProtocol";
    case CE_InvalidArguments:
        return "CE_InvalidArguments";
    default:
        return "unknown error";
    }
}

std::vector<CorsairLedColor> getAvailableKeys()
{
    auto colorsSet = std::unordered_set<int>();
    for (int deviceIndex = 0, size = CorsairGetDeviceCount(); deviceIndex < size; deviceIndex++) {
        if (const auto ledPositions = CorsairGetLedPositionsByDeviceIndex(deviceIndex)) {
            for (auto i = 0; i < ledPositions->numberOfLed; i++) {
                const auto ledId = ledPositions->pLedPosition[i].ledId;
                colorsSet.insert(ledId);
            }
        }
    }

    std::vector<CorsairLedColor> colorsVector;
    colorsVector.reserve(colorsSet.size());
    for (const auto &ledId : colorsSet) {
        colorsVector.push_back({ static_cast<CorsairLedId>(ledId), 0, 0, 0 });
    }
    return colorsVector;
}

void performPulseEffect(int waveDuration, std::vector<CorsairLedColor> &ledColorsVec)
{
    const auto timePerFrame = 25;
    for (auto &ledColor : ledColorsVec)
    {
        ledColor.r = red;
        ledColor.g = green;
        ledColor.b = blue;
    }
    CorsairSetLedsColorsAsync(static_cast<int>(ledColorsVec.size()), ledColorsVec.data(), nullptr, nullptr);
    std::this_thread::sleep_for(std::chrono::milliseconds(timePerFrame));
}

int startWinsock(void)
{
    WSADATA wsa;
    return WSAStartup(MAKEWORD(2, 0), &wsa);
}

int receiveUdp() {
    long rc;
    SOCKET s;
    char buf[256];
    char buf2[300];
    SOCKADDR_IN addr;
    SOCKADDR_IN remoteAddr;
    int remoteAddrLen = sizeof(SOCKADDR_IN);

    //auto allLeds = getLeds();

    rc = startWinsock();
    if (rc != 0)
    {
        printf("Fehler: startWinsock, fehler code: %d\n", rc);
        return 1;
    }
    else
    {
        printf("Winsock gestartet!\n");
    }

    //UDP Socket erstellen
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s == INVALID_SOCKET)
    {
        printf("Fehler: Der Socket konnte nicht erstellt werden, fehler code: %d\n", WSAGetLastError());
        return 1;
    }
    else
    {
        printf("UDP Socket erstellt!\n");
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = ADDR_ANY;
    rc = bind(s, (SOCKADDR*)&addr, sizeof(SOCKADDR_IN));
    if (rc == SOCKET_ERROR)
    {
        printf("Fehler: bind, fehler code: %d\n", WSAGetLastError());
        return 1;
    }
    else
    {
        std::cout << "Socket an Port " << port << " gebunden" << std::endl;
    }
    while (1)
    {
        rc = recvfrom(s, buf, 256, 0, (SOCKADDR*)&remoteAddr, &remoteAddrLen);
        if (rc == SOCKET_ERROR)
        {
            printf("Fehler: recvfrom, fehler code: %d\n", WSAGetLastError());
            return 1;
        }
        else
        {
            printf("%d Bytes empfangen!\n", rc);
            buf[rc] = '\0';
        }
        auto content = std::string(buf);
        std::cout << "Empfangene Daten: " << content.c_str() << std::endl;

        auto pos = content.find(":");

        auto themeName = content.substr(pos + 1);
        std::cout << "themeName: " << themeName.c_str() << std::endl;

        if (themeName == "off") {
            red = 0;
            green = 0;
            blue = 0;
        }
        else if (themeName == "Nordlichter") {
            red = 0;
            green = 255;
            blue = 255;
        }
        else if (themeName == "Lagerfeuer") {
            red = 255;
            green = 50;
            blue = 0;
        }
        else if (themeName == "Konzentration") {
            red = 255;
            green = 255;
            blue = 255;
        }
        else if (themeName == "Gedimmt") {
            red = 54;
            green = 27;
            blue = 12;
        }
        else if (themeName == "Miami") {
            red = 255;
            green = 0;
            blue = 255;
        }
        else if (themeName == "Bunt") {
            red = 0;
            green = 255;
            blue = 0;
        }
    }
}

int main()
{
    CorsairPerformProtocolHandshake();
    while (CorsairGetLastError() == CE_ServerNotFound) {
        std::cout << "Connection to ICue failed retry in";
        for (int i = 5; i >= 0; i--)
        {
            std::cout << " " << i;
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
        std::cout << std::endl;
        CorsairPerformProtocolHandshake();
    }

    std::atomic_int waveDuration{ 500 };
    std::atomic_bool continueExecution{ true };

    auto colorsVector = getAvailableKeys();
    if (colorsVector.empty()) {
        std::cout << "error: no keys found" << std::endl;
        return 1;
    }

    CorsairSetLayerPriority(100);
    std::thread lightingThread([&waveDuration, &continueExecution, &colorsVector] {
        while (continueExecution) {
            performPulseEffect(waveDuration.load(), colorsVector);
        }
    });

    std::thread udpThread(receiveUdp);

    while (1) {
        char c = getchar();
    };
    lightingThread.join();
    return 0;
}