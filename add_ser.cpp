#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <thread>
#include <string>
#include <cstdlib>
#include <pistache/endpoint.h>
#include <pistache/router.h>
#include <pistache/http.h>
#include <iostream>
#include "addition_api.h"



using namespace Pistache;

class APIHandler {
public:
    APIHandler() : additionAPI() {}

    void addHandler(const Rest::Request& request, Http::ResponseWriter response) {
    auto a = request.query().get("a").value_or("0");
    auto b = request.query().get("b").value_or("0");
    int result = additionAPI.add(std::stoi(a), std::stoi(b));
    response.send(Http::Code::Ok, std::to_string(result));
}

void subtractHandler(const Rest::Request& request, Http::ResponseWriter response) {
    auto a = request.query().get("a").value_or("0");
    auto b = request.query().get("b").value_or("0");
    int result = additionAPI.subtract(std::stoi(a), std::stoi(b));
    response.send(Http::Code::Ok, std::to_string(result));
}

private:
    AdditionAPI additionAPI;
};

void setupRoutes(Rest::Router& router, APIHandler& handler) {
    Rest::Routes::Get(router, "/add", Rest::Routes::bind(&APIHandler::addHandler, &handler));
    Rest::Routes::Get(router, "/subtract", Rest::Routes::bind(&APIHandler::subtractHandler, &handler));
}

int main() {
    Pistache::Http::Endpoint server(Pistache::Address(Pistache::Ipv4::any(), Pistache::Port(8080)));

    auto opts = Http::Endpoint::options().threads(1);
    server.init(opts);

    Rest::Router router;
    APIHandler handler;
    setupRoutes(router, handler);

    server.setHandler(router.handler());
    server.serve();

    return 0;
}


namespace beast = boost::beast;
namespace http = beast::http;  // from <boost/beast/http.hpp>
namespace net = boost::asio;    // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;

void handle_request(http::request<http::string_body> const& req, http::response<http::string_body>& res) {
    if (req.method() == http::verb::get) {
        std::string target = req.target().to_string();
        
        // Handle addition: /add?a=5&b=3
        if (target.find("/add") == 0) {
            // Parse query parameters (a and b)
            auto query_pos = target.find("?");
            if (query_pos != std::string::npos) {
                std::string query = target.substr(query_pos + 1);
                size_t a_pos = query.find("a=");
                size_t b_pos = query.find("b=");
                
                if (a_pos != std::string::npos && b_pos != std::string::npos) {
                    int a = std::atoi(query.substr(a_pos + 2).c_str());
                    int b = std::atoi(query.substr(b_pos + 2).c_str());
                    int result = a + b;

                    res.result(http::status::ok);
                    res.set(http::field::content_type, "text/plain");
                    res.body() = "Result of addition: " + std::to_string(result);
                } else {
                    res.result(http::status::bad_request);
                    res.set(http::field::content_type, "text/plain");
                    res.body() = "Invalid query parameters.";
                }
            }
        } else {
            res.result(http::status::not_found);
            res.set(http::field::content_type, "text/plain");
            res.body() = "Resource not found.";
        }
    } else {
        res.result(http::status::method_not_allowed);
        res.set(http::field::content_type, "text/plain");
        res.body() = "Method not allowed.";
    }
    res.prepare_payload();
}

void do_session(tcp::socket socket) {
    try {
        beast::flat_buffer buffer;
        http::request<http::string_body> req;

        // Read the HTTP request from the client
        http::read(socket, buffer, req);

        // Prepare the HTTP response
        http::response<http::string_body> res;
        handle_request(req, res);

        // Send the HTTP response to the client
        http::write(socket, res);
    } catch (std::exception const& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

int main() {
    try {
        net::io_context ioc;
        tcp::acceptor acceptor(ioc, tcp::endpoint(tcp::v4(), 8080));

        std::cout << "Server is listening on port 8080..." << std::endl;

        for (;;) {
            tcp::socket socket(ioc);
            acceptor.accept(socket);
            std::thread(&do_session, std::move(socket)).detach();
        }
    } catch (std::exception const& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
} 