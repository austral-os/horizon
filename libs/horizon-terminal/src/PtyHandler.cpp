#define _XOPEN_SOURCE 600
#include "PtyHandler.hpp"
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <pty.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <cstring>
#include <iostream>
#include <signal.h>

namespace horizon {
namespace terminal {

PtyHandler::PtyHandler() {}

PtyHandler::~PtyHandler() {
    close();
}

bool PtyHandler::spawn(const std::string& command, const std::vector<std::string>& args, int cols, int rows) {
    struct winsize size = {(unsigned short)rows, (unsigned short)cols, 0, 0};
    
    m_child_pid = forkpty(&m_master_fd, NULL, NULL, &size);

    if (m_child_pid < 0) {
        return false;
    }

    if (m_child_pid == 0) {
        // Child
        setenv("TERM", "xterm-256color", 1);
        
        std::vector<char*> argv;
        std::string shell_name = command;
        size_t last_slash = shell_name.find_last_of('/');
        if (last_slash != std::string::npos) {
            shell_name = shell_name.substr(last_slash + 1);
        }
        std::string argv0 = "-" + shell_name;
        argv.push_back(strdup(argv0.c_str()));
        for (const auto& arg : args) {
            argv.push_back(strdup(arg.c_str()));
        }
        argv.push_back(nullptr);

        execvp(command.c_str(), argv.data());
        _exit(1);
    }

    // Parent
    m_running = true;
    m_read_thread = std::thread(&PtyHandler::read_loop, this);
    
    return true;
}

void PtyHandler::close() {
    if (!m_running && m_master_fd == -1) return;
    
    m_running = false;
    
    // Close the master FD first to unblock the read() call in the thread
    if (m_master_fd != -1) {
        ::close(m_master_fd);
        m_master_fd = -1;
    }

    // Terminate the child process shell
    if (m_child_pid != -1) {
        kill(m_child_pid, SIGHUP);
        
        // Wait a bit or use non-blocking wait
        int status;
        pid_t result = waitpid(m_child_pid, &status, WNOHANG);
        if (result == 0) {
            // Still running, wait some more or kill it
            usleep(100000); // 100ms
            result = waitpid(m_child_pid, &status, WNOHANG);
            if (result == 0) {
                kill(m_child_pid, SIGKILL);
                waitpid(m_child_pid, &status, 0);
            }
        }
        m_child_pid = -1;
    }

    if (m_read_thread.joinable()) {
        m_read_thread.join();
    }
}

ssize_t PtyHandler::write(const char* data, size_t len) {
    if (m_master_fd == -1) return -1;
    return ::write(m_master_fd, data, len);
}

void PtyHandler::resize(int cols, int rows) {
    if (m_master_fd == -1) return;
    struct winsize size = {(unsigned short)rows, (unsigned short)cols, 0, 0};
    ioctl(m_master_fd, TIOCSWINSZ, &size);
}

void PtyHandler::read_loop() {
    char buffer[4096];
    while (m_running) {
        ssize_t n = ::read(m_master_fd, buffer, sizeof(buffer));
        if (n <= 0) {
            m_running = false;
            break;
        }
        if (m_read_callback) {
            m_read_callback(buffer, n);
        }
    }
}

} // namespace terminal
} // namespace horizon
