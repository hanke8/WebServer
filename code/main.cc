#include "webserver.h"
#include <iostream>

int main() {
    WebServer server(1234, 3, 60000, false, 6);
    std::cout << "----------------- start -----------------" << std::endl;
    server.StartService();
    return 0;
}