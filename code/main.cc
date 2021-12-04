#include "webserver.h"
#include <iostream>

int main() {
    WebServer server(1316);
    server.StartService();
    return 0;
}