#include "http_server.h"
#include <iostream>
#include <sstream>
#include <regex>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #include <arpa/inet.h>
#endif

namespace gis {

HttpServer::HttpServer(int port) : port_(port), running_(false) {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

HttpServer::~HttpServer() {
    stop();
#ifdef _WIN32
    WSACleanup();
#endif
}

void HttpServer::setHandler(RequestHandler handler) {
    handler_ = handler;
}

bool HttpServer::start() {
    if (running_) {
        return false;
    }
    
    running_ = true;
    server_thread_ = std::thread(&HttpServer::serverLoop, this);
    
    std::cout << "HTTP Server starting on port " << port_ << std::endl;
    return true;
}

void HttpServer::stop() {
    running_ = false;
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
}

void HttpServer::serverLoop() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return;
    }
    
    // Set socket options
    int opt = 1;
#ifdef _WIN32
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
#else
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif
    
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);
    
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Failed to bind to port " << port_ << std::endl;
#ifdef _WIN32
        closesocket(server_fd);
#else
        close(server_fd);
#endif
        return;
    }
    
    if (listen(server_fd, 3) < 0) {
        std::cerr << "Failed to listen on socket" << std::endl;
#ifdef _WIN32
        closesocket(server_fd);
#else
        close(server_fd);
#endif
        return;
    }
    
    std::cout << "Server listening on port " << port_ << std::endl;
    
    while (running_) {
        struct sockaddr_in client_address;
        socklen_t client_len = sizeof(client_address);
        
        int client_fd = accept(server_fd, (struct sockaddr*)&client_address, &client_len);
        if (client_fd < 0) {
            if (running_) {
                std::cerr << "Failed to accept connection" << std::endl;
            }
            continue;
        }
        
        // Read request
        char buffer[4096] = {0};
        int bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_read > 0) {
            std::string request(buffer, bytes_read);
            std::string response = handleRequest(request);
            
            send(client_fd, response.c_str(), response.length(), 0);
        }
        
#ifdef _WIN32
        closesocket(client_fd);
#else
        close(client_fd);
#endif
    }
    
#ifdef _WIN32
    closesocket(server_fd);
#else
    close(server_fd);
#endif
}

std::string HttpServer::handleRequest(const std::string& request) {
    try {
        auto [path, query] = parseRequest(request);
        
        if (handler_) {
            std::string content = handler_(path, query);
            return createResponse(content);
        } else {
            return createResponse(R"({"error": "No handler configured"})", "application/json");
        }
    } catch (const std::exception& e) {
        std::string error_content = R"({"error": ")" + std::string(e.what()) + R"("})";
        return createResponse(error_content, "application/json");
    }
}

std::string HttpServer::createResponse(const std::string& content, const std::string& content_type) {
    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Type: " << content_type << "\r\n";
    response << "Content-Length: " << content.length() << "\r\n";
    response << "Access-Control-Allow-Origin: *\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << content;
    
    return response.str();
}

std::pair<std::string, std::string> HttpServer::parseRequest(const std::string& request) {
    std::istringstream iss(request);
    std::string method, path_with_query, version;
    iss >> method >> path_with_query >> version;
    
    // Extract path and query string
    size_t query_pos = path_with_query.find('?');
    std::string path = path_with_query.substr(0, query_pos);
    std::string query = (query_pos != std::string::npos) ? path_with_query.substr(query_pos + 1) : "";
    
    return {path, query};
}

} // namespace gis
