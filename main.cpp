#include <iostream>
#include <fstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <chrono>
#include <thread>

#pragma comment(lib, "Ws2_32.lib")

const char* SERVER_IP = "127.0.0.1";
const int PORT = 3000;

#pragma pack(push, 1)
struct trade_data {
    char symbol[4];
    char side;
    uint32_t qty;
    uint32_t price;
    uint32_t seq;
};
#pragma pack(pop)
// receving trade data
bool receiving_trade_data(SOCKET sock, char* buffer, int len) {
    int received = 0;
    int count = 0;
    while (received < len) {
        int bytes = recv(sock, buffer + received, len - received, 0);
        if (bytes <= 0) {
            if (count++ > 2) return false;  
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
        } else {
            received += bytes;
        }
    }
    return true;
}
// json output
void write_json_output(const std::vector<trade_data>& trades, const std::string& out_file) {
    std::ofstream file(out_file);
    if (!file) {
        std::cerr << "unable to write to file: " << out_file << std::endl;
        return;
    }

    file << "[\n";
    for (size_t i = 0; i < trades.size(); ++i) {
        file << "  {\n";
        file << "    \"trade-tick\": \"" << std::string(trades[i].symbol, 4) << "\",\n";
        file << "    \"orderSide\": \"" << (trades[i].side == 'B' ? "BUY" : "SELL") << "\",\n";
        file << "    \"quantity\": " << trades[i].qty << ",\n";
        file << "    \"price\": " << trades[i].price << ",\n";
        file << "    \"seq\": " << trades[i].seq << "\n";
        file << "  }" << (i < trades.size() - 1 ? "," : "") << "\n";
    }
    file << "]\n";
    file.close();
}

int main() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "error" << WSAGetLastError() << std::endl;
        return 1;
    }

    std::vector<trade_data> trades;
    std::set<uint32_t> seqs;
    uint32_t max_seq = 0;

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "error: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &server.sin_addr);

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        std::cerr << "connecition failuree.\n";
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    std::cout << "sending request to server\n";

    char req[2] = {1, 0};
    send(sock, req, 2, 0);

    char buffer[sizeof(trade_data)];

    while (receiving_trade_data(sock, buffer, sizeof(trade_data))) {
        trade_data trade;
        memcpy(&trade, buffer, sizeof(trade_data));

        trade.qty = ntohl(trade.qty);
        trade.price = ntohl(trade.price);
        trade.seq = ntohl(trade.seq);

        trades.push_back(trade);
        seqs.insert(trade.seq);
        max_seq = std::max(max_seq, trade.seq);

        std::cout << "trade seq " << trade.seq << ": " 
                  << std::string(trade.symbol, 4) << " "
                  << trade.side << " " << trade.qty << "@" << trade.price << std::endl;
    }

    closesocket(sock);

    std::cout << "Stream completed\n";

    std::vector<uint32_t> missing_seqs;
    for (uint32_t i = 1; i <= max_seq; ++i) {
        if (seqs.find(i) == seqs.end()) {
            std::cout << "missing seq " << i << ". Adding it.\n";
            missing_seqs.push_back(i);
        }
    }

    for (uint32_t seq : missing_seqs) {
        std::cout << "getting missed seq " << seq << std::endl;

        SOCKET rsock = socket(AF_INET, SOCK_STREAM, 0);
        if (rsock == INVALID_SOCKET) {
            std::cerr << "Error : no socket.\n";
            continue;
        }

        if (connect(rsock, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
            std::cerr << "connection failure \n";
            closesocket(rsock);
            continue;
        }

        char req[2] = {2, static_cast<char>(seq)};
        send(rsock, req, 2, 0);

        DWORD tmo = 5000;
        setsockopt(rsock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tmo, sizeof(tmo));

        if (receiving_trade_data(rsock, buffer, sizeof(trade_data))) {
            trade_data recovered;
            memcpy(&recovered, buffer, sizeof(trade_data));

            recovered.qty = ntohl(recovered.qty);
            recovered.price = ntohl(recovered.price);
            recovered.seq = ntohl(recovered.seq);

            trades.push_back(recovered);
            seqs.insert(recovered.seq);
        }

        closesocket(rsock);
    }

    write_json_output(trades, "trade_data.json");
    WSACleanup();
    return 0;
}
