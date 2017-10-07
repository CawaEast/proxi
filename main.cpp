#include <sys/signalfd.h>
#include "proxy/proxy_server.h"

int main(int argc, char **args) {
    try {
        uint16_t port = 8080;
        if (argc > 1) {
            port = (uint16_t) std::stoi(args[1]);
        }
        proxy_server proxy(200, port, 200);
        std::string tag = "server on port " + std::to_string(port);
        log(tag, "started");
        proxy.run();

    } catch (annotated_exception const &e) {
        log(e);
    }
}
