#pragma once

#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>

namespace horizon {
namespace terminal {

class PtyHandler {
public:
    PtyHandler();
    ~PtyHandler();

    bool spawn(const std::string& command, const std::vector<std::string>& args, int cols, int rows);
    void close();

    ssize_t write(const char* data, size_t len);
    void resize(int cols, int rows);

    void set_read_callback(std::function<void(const char*, size_t)> callback) {
        m_read_callback = callback;
    }

private:
    void read_loop();

    int m_master_fd = -1;
    pid_t m_child_pid = -1;
    std::thread m_read_thread;
    std::atomic<bool> m_running{false};
    std::function<void(const char*, size_t)> m_read_callback;
};

} // namespace terminal
} // namespace horizon
