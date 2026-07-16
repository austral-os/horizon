#include "TerminalController.hpp"
#include <cassert>
#include <iostream>
#include <cstring>

using namespace horizon::terminal;

// ---- helpers ----

static void assert_bracketed(TerminalController& tc, bool expected, const char* label) {
    bool got = tc.is_bracketed_paste();
    if (got != expected) {
        std::cerr << "FAIL: " << label
                  << " — expected " << expected
                  << ", got " << got << "\n";
        assert(got == expected);
    }
    std::cout << "[OK] " << label << "\n";
}

// ============================================================
// 1. Single-buffer enable
// ============================================================
void test_enable() {
    TerminalController tc(24, 80);
    assert_bracketed(tc, false, "initial state is false");

    const char seq[] = "\x1b[?2004h";
    tc.push_data(seq, sizeof(seq) - 1);
    assert_bracketed(tc, true, "enable in single chunk");
}

// ============================================================
// 2. Single-buffer disable
// ============================================================
void test_disable() {
    TerminalController tc(24, 80);

    // enable first
    const char en[] = "\x1b[?2004h";
    tc.push_data(en, sizeof(en) - 1);
    assert_bracketed(tc, true, "enabled before disable test");

    const char dis[] = "\x1b[?2004l";
    tc.push_data(dis, sizeof(dis) - 1);
    assert_bracketed(tc, false, "disable in single chunk");
}

// ============================================================
// 3. Split enable across two push_data calls
//    Chunk 1: "\x1b[?2004" (7-byte common prefix)
//    Chunk 2: "h"         (completes the 8-byte sequence)
// ============================================================
void test_split_enable() {
    TerminalController tc(24, 80);

    const char c1[] = "\x1b[?2004";
    tc.push_data(c1, sizeof(c1) - 1);
    assert_bracketed(tc, false, "no change after partial prefix");

    const char c2[] = "h";
    tc.push_data(c2, sizeof(c2) - 1);
    assert_bracketed(tc, true, "enable detected after split");
}

// ============================================================
// 4. Split disable across two push_data calls
//    Chunk 1: "\x1b[?2004" (7-byte common prefix)
//    Chunk 2: "l"         (completes the 8-byte sequence)
// ============================================================
void test_split_disable() {
    TerminalController tc(24, 80);

    // enable first (complete sequence)
    const char en[] = "\x1b[?2004h";
    tc.push_data(en, sizeof(en) - 1);
    assert_bracketed(tc, true, "enabled before split-disable test");

    // split the disable
    const char c1[] = "\x1b[?2004";
    tc.push_data(c1, sizeof(c1) - 1);
    assert_bracketed(tc, true, "still true after partial prefix");

    const char c2[] = "l";
    tc.push_data(c2, sizeof(c2) - 1);
    assert_bracketed(tc, false, "disable detected after split");
}

// ============================================================
// 5. Enable followed by data followed by disable — all in one chunk
// ============================================================
void test_enable_data_disable() {
    TerminalController tc(24, 80);

    const char buf[] = "\x1b[?2004h hello world \x1b[?2004l";
    tc.push_data(buf, sizeof(buf) - 1);
    assert_bracketed(tc, false, "false after enable+data+disable in one chunk");
}

// ============================================================
// 6. Only first 5 bytes of prefix — should NOT trigger
// ============================================================
void test_partial_no_match() {
    TerminalController tc(24, 80);

    const char c1[] = "\x1b[?2";
    tc.push_data(c1, sizeof(c1) - 1);
    assert_bracketed(tc, false, "5-byte partial does not enable");
}

// ============================================================
// 7. Interleaved text does not confuse detection
// ============================================================
void test_interleaved_text() {
    TerminalController tc(24, 80);

    const char buf[] = "some output \x1b[?2004h more text";
    tc.push_data(buf, sizeof(buf) - 1);
    assert_bracketed(tc, true, "enable detected with surrounding text");
}

// ============================================================
// 8. Carried partial across data chunk
//    Split ESC[?2004h such that carry persists correctly
// ============================================================
void test_carry_across_data() {
    TerminalController tc(24, 80);

    // Split: "\x1b" then "[?2004h"
    const char c1[] = "\x1b";
    tc.push_data(c1, 1);
    assert_bracketed(tc, false, "just ESC — no change");

    const char c2[] = "[?2004h";
    tc.push_data(c2, sizeof(c2) - 1);
    assert_bracketed(tc, true, "enable after ESC carried across chunks");
}

// ============================================================
// 9. Multiple split enables in rapid succession
// ============================================================
void test_repeated_enable() {
    TerminalController tc(24, 80);

    const char en[] = "\x1b[?2004h";
    tc.push_data(en, sizeof(en) - 1);
    assert_bracketed(tc, true, "first enable");

    tc.push_data(en, sizeof(en) - 1);
    assert_bracketed(tc, true, "repeated enable — still true");
}

// ============================================================
// main
// ============================================================
int main() {
    test_enable();
    test_disable();
    test_split_enable();
    test_split_disable();
    test_enable_data_disable();
    test_partial_no_match();
    test_interleaved_text();
    test_carry_across_data();
    test_repeated_enable();

    std::cout << "\nAll bracketed-paste tests passed.\n";
    return 0;
}
