#pragma once

#include <functional>
#include <string>
#include <vector>
#include <mutex>

namespace horizon
{
    /**
     * @brief Context information passed to every signal handler.
     */
    struct SignalContext
    {
        void *sender{nullptr};        /**< Pointer to the object that emitted the signal. */
        std::string signal;           /**< The signal identifier (e.g., "option_save_clicked"). */
        bool stop_propagation{false}; /**< If set to true, subsequent handlers won't be called. */
        void *data{nullptr};          /**< Custom data associated with the signal. */
    };

    /**
     * @class SignalManager
     * @brief Centralized manager for signal callbacks using string-based identifiers.
     */
    class SignalManager
    {
    public:
        using Callback = std::function<void(SignalContext &)>;

        /**
         * @brief Connects a callback to a specific signal.
         * @param signal The signal identifier (e.g., "option_save_clicked").
         * @param callback The function to execute when the signal is emitted.
         * @return A unique subscription ID.
         */
        size_t connect(const std::string &signal, Callback callback);

        /**
         * @brief Disconnects a callback using its ID.
         * @param id The ID returned by connect().
         */
        void disconnect(size_t id);

        /**
         * @brief Emits a signal, executing all registered callbacks for it.
         * @param signal The signal identifier to emit.
         * @param context The signal context to pass to handlers.
         */
        void emit(const std::string &signal, SignalContext &context);

        /**
         * @brief Convenience overload that creates a context automatically.
         * @param signal The signal identifier to emit.
         * @param sender The object emitting the signal.
         */
        void emit(const std::string &signal, void *sender = nullptr);

    private:
        struct Handler
        {
            size_t id;
            std::string signal;
            Callback callback;
        };

        std::vector<Handler> m_handlers;
        size_t m_next_id{0};
        mutable std::mutex m_mutex;
    };

} // namespace horizon
