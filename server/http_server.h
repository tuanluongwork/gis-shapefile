#pragma once

#include <string>
#include <functional>
#include <thread>
#include <atomic>

namespace gis {

/**
 * @brief Simple HTTP server for GIS API endpoints
 * 
 * This is a minimal HTTP server implementation for demonstration purposes.
 * In production, you would use a robust framework like Crow, Beast, or similar.
 */
class HttpServer {
public:
    using RequestHandler = std::function<std::string(const std::string& path, const std::string& query)>;
    
private:
    int port_;
    std::atomic<bool> running_;
    std::thread server_thread_;
    RequestHandler handler_;
    
public:
    explicit HttpServer(int port = 8080);
    ~HttpServer();
    
    /**
     * @brief Set the request handler function
     */
    void setHandler(RequestHandler handler);
    
    /**
     * @brief Start the server
     */
    bool start();
    
    /**
     * @brief Stop the server
     */
    void stop();
    
    /**
     * @brief Check if server is running
     */
    bool isRunning() const { return running_; }

private:
    void serverLoop();
    std::string handleRequest(const std::string& request);
    std::string createResponse(const std::string& content, const std::string& content_type = "application/json");
    std::pair<std::string, std::string> parseRequest(const std::string& request);
};

} // namespace gis
