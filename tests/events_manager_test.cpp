#include <horizon/EventsManager.hpp>
#include <cassert>
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <future>
#include <stdexcept>

using namespace horizon;

// ============================================================
// 1. disconnect prevents future run invocation
// ============================================================
void test_disconnect_prevents_future_run()
{
    EventsManager<EventContext> em;
    int call_count = 0;

    auto id = em.connect([&](EventContext&) { ++call_count; });

    em.disconnect(id);

    EventContext ctx;
    em.run(ctx);

    assert(call_count == 0);
    std::cout << "[OK] test_disconnect_prevents_future_run\n";
}

// ============================================================
// 2. Self-disconnect during run does not deadlock
//    and future runs skip the disconnected handler
// ============================================================
void test_self_disconnect_during_run()
{
    EventsManager<EventContext> em;
    size_t self_id{0};
    int second_count = 0;
    int after_disconnect_all = 0;

    // Handler A: disconnects itself during execution
    self_id = em.connect([&](EventContext&) {
        em.disconnect(self_id);
    });

    // Handler B: normal handler that just counts
    auto second_id = em.connect([&](EventContext&) {
        ++second_count;
    });

    EventContext ctx1;
    em.run(ctx1);

    // Handler B must still run despite self-disconnect in handler A
    assert(second_count == 1);

    // Run #2 — handler A is gone (self-disconnected from m_handlers),
    // but handler B is still connected
    EventContext ctx2;
    em.run(ctx2);

    assert(second_count == 2);

    // Now disconnect handler B normally and verify it stops
    em.disconnect(second_id);

    EventContext ctx3;
    em.run(ctx3);

    assert(second_count == 2); // no change

    std::cout << "[OK] test_self_disconnect_during_run\n";
}

// ============================================================
// 3. Cross-thread disconnect: disconnect from another thread
//    must block until in-flight callback finishes
// ============================================================
void test_cross_thread_disconnect_waits()
{
    EventsManager<EventContext> em;
    std::promise<void> callback_entered;
    std::promise<void> callback_can_finish;
    auto entered_future = callback_entered.get_future();
    auto can_finish_future = callback_can_finish.get_future();

    std::promise<void> disconnect_entered;
    std::promise<void> disconnect_done;
    auto disconnect_entered_future = disconnect_entered.get_future();
    auto disconnect_done_future = disconnect_done.get_future();

    auto id = em.connect([&](EventContext&) {
        callback_entered.set_value();
        can_finish_future.wait();
    });

    EventContext ctx;
    std::thread runner([&]() { em.run(ctx); });

    // Wait until callback is executing (handler->running == 1)
    entered_future.wait();

    std::thread disconnector([&]() {
        disconnect_entered.set_value();       // signal: about to call disconnect()
        em.disconnect(id);
        disconnect_done.set_value();          // signal: disconnect() returned
    });

    // Wait until the disconnector has signaled it is about to enter disconnect()
    disconnect_entered_future.wait();

    // At this point the disconnector thread is inside (or about to enter)
    // disconnect(). Since the callback is still blocked on can_finish_future,
    // disconnect() MUST block waiting for the running count to drop to zero.
    // A non-blocking wait_for(0) proves disconnect_done is NOT yet ready.
    assert(disconnect_done_future.wait_for(std::chrono::seconds(0))
               == std::future_status::timeout
           && "disconnect() from another thread MUST block while callback runs");

    // Let callback complete — RunningGuard fires, running--, idle notified
    callback_can_finish.set_value();

    disconnector.join();
    runner.join();

    // Now disconnect_done must be ready
    assert(disconnect_done_future.wait_for(std::chrono::seconds(0))
               == std::future_status::ready
           && "disconnect() MUST return after in-flight callback finishes");

    std::cout << "[OK] test_cross_thread_disconnect_waits\n";
}

// ============================================================
// 4. Callback exception must not leave running count stuck;
//    RunningGuard dtor must fire during stack unwinding so
//    disconnect() does not deadlock afterwards
// ============================================================
void test_exception_does_not_stick_running()
{
    EventsManager<EventContext> em;

    auto id = em.connect([&](EventContext&) {
        throw std::runtime_error("expected error");
    });

    EventContext ctx;
    try {
        em.run(ctx);
    } catch (const std::runtime_error&) {
        // expected — stack unwinding fires RunningGuard
    }

    // If running count were stuck, disconnect would deadlock
    em.disconnect(id);

    std::cout << "[OK] test_exception_does_not_stick_running\n";
}

// ============================================================
// 5. stop_propagation still stops later handlers
// ============================================================
void test_stop_propagation()
{
    EventsManager<EventContext> em;
    int after_stop_count = 0;

    em.connect([&](EventContext& ctx) {
        ctx.stop_propagation = true;
    });

    em.connect([&](EventContext&) {
        ++after_stop_count;
    });

    EventContext ctx;
    em.run(ctx);

    assert(after_stop_count == 0);
    std::cout << "[OK] test_stop_propagation\n";
}

// ============================================================
// 6. Disconnected handler is skipped during run, but
//    remaining connected handlers still execute
// ============================================================
void test_remaining_handlers_run_after_disconnect()
{
    EventsManager<EventContext> em;
    int count = 0;

    auto id = em.connect([&](EventContext&) { ++count; });
    em.disconnect(id);

    // Add another handler — should still run
    em.connect([&](EventContext&) { ++count; });

    EventContext ctx;
    em.run(ctx);

    assert(count == 1);
    std::cout << "[OK] test_remaining_handlers_run_after_disconnect\n";
}

int main()
{
    std::cout << "Running EventsManager concurrency/lifecycle tests...\n";

    test_disconnect_prevents_future_run();
    test_self_disconnect_during_run();
    test_cross_thread_disconnect_waits();
    test_exception_does_not_stick_running();
    test_stop_propagation();
    test_remaining_handlers_run_after_disconnect();

    std::cout << "All EventsManager tests passed successfully!\n";
    return 0;
}
