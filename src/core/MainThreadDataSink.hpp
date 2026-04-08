#pragma once

#include "horizon/ClipboardProvider.hpp"
#include "horizon/WaylandWindow.hpp"
#include <memory>

namespace horizon {

/**
 * @class MainThreadDataSink
 * @brief Decorator that ensures all DataSink calls are marshaled to the window's UI thread.
 */
class MainThreadDataSink : public DataSink {
public:
    MainThreadDataSink(WaylandWindow* window, DataSink* target)
        : m_window(window), m_target(target) {}

    void write(const std::vector<uint8_t>& data) override {
        if (!m_window || !m_target) return;
        m_window->post_task([target = m_target, data]() {
            target->write(data);
        });
    }

    void done() override {
        if (!m_window || !m_target) return;
        m_window->post_task([target = m_target]() {
            target->done();
        });
    }

    void error() override {
        if (!m_window || !m_target) return;
        m_window->post_task([target = m_target]() {
            target->error();
        });
    }

private:
    WaylandWindow* m_window;
    DataSink* m_target;
};

} // namespace horizon
