#include "webserver.h"
#include <iostream>
using namespace std;

int main() {
    cout << "start..." << endl;
    WebServer svr(12345);
    svr.StartService();
    return 0;
}