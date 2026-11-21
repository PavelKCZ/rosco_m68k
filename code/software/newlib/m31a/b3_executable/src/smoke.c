#include "b3_integration.h"
#include "runtime.h"
#include "diagnostic.h"
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* Application storage only. The backend allocations remain unchanged. */
static uint8_t bytes[513];
static volatile unsigned bss_probe;
static volatile unsigned data_probe = 0x31b3;
static volatile unsigned ctor_probe;
static int console_error;
#ifdef SDRO_CSD_DIAGNOSTIC
#include "csd_format.h"
static sdro_csd_diagnostic csd_snapshot;
static char csd_text[CSDD_BUFFER_SIZE];
#endif

static int line(const char *format, ...) __attribute__((format(printf, 1, 2)));
static int line(const char *format, ...) {
    va_list args;
    int result;
    va_start(args, format);
    result = vprintf(format, args);
    va_end(args);
    if (result < 0 || fflush(stdout) != 0) console_error = 1;
    return console_error ? -1 : 0;
}

static void __attribute__((constructor)) app_construct(void) { ctor_probe = 0x31b3; }
static void __attribute__((destructor)) app_destruct(void) { (void)line("APP_DTOR\n"); }
static void app_atexit(void) { (void)line("APP_ATEXIT\n"); }

int main(void) {
    const char *phase = "RUNTIME";
    const char *reason = "initialization probes";
    b3_status status = B3_OK;
    b1_handle handle = 0;
    b3_diagnostic before = {0}, after = {0};
    uint32_t position = 0, size = 0;
    size_t delivered = 0;
    unsigned mismatch = 0;
    int attempted = 0, opened = 0, failed = 0;

#define REQUIRE(test, why) do { if (!(test)) { reason = (why); failed = 1; goto finish; } } while (0)
    M31_MARK('M');
    /* No constructor performs stdio, so buffering is selected before first use. */
    REQUIRE(setvbuf(stdout, NULL, _IONBF, 0) == 0, "stdout buffering");
    M31_MARK('O');
    REQUIRE(line("APP_BEGIN top=%08lx stack=%08lx heap=%08lx..%08lx\n",
                 (unsigned long)m31_memory.top, (unsigned long)m31_memory.stack_bottom,
                 (unsigned long)m31_memory.heap_start, (unsigned long)m31_memory.heap_end) == 0,
            "console output");
    M31_MARK('F');
    REQUIRE(!bss_probe && data_probe == 0x31b3 && ctor_probe == 0x31b3, "runtime probes");
    REQUIRE(atexit(app_atexit) == 0, "atexit registration");

    phase = "INIT";
    REQUIRE(line("INIT_BEGIN\n") == 0, "console output before init");
    attempted = 1;
    status = b3_init();
    REQUIRE(status == B3_OK, "B3 initialization");
    status = b3_diagnostics(&before);
    REQUIRE(status == B3_OK && before.state == B3_READY && before.capacity != 0,
            "READY capacity");
    /* Two hexadecimal words retain the entire uint64_t capacity. */
    REQUIRE(line("INIT_OK sectors=%08lx%08lx\n",
                 (unsigned long)(uint32_t)(before.capacity >> 32),
                 (unsigned long)(uint32_t)before.capacity) == 0, "console output");

    phase = "OPEN";
    status = b3_open("/M31RO.BIN", &handle);
    REQUIRE(status == B3_OK, "open");
    opened = 1;
    REQUIRE(line("OPEN_OK\n") == 0, "console output");
    phase = "INFO";
    status = b3_info(handle, &position, &size);
    REQUIRE(status == B3_OK, "file info");
    REQUIRE(size == sizeof(bytes) && position == 0, "fixture size/position");

    phase = "READ";
    delivered = 0;
    status = b3_read(handle, bytes, sizeof(bytes), &delivered);
    /* B1 promises OK only for the full request. EOF/error may deliver a prefix;
     * never retry that prefix or misreport it as normal EOF. */
    REQUIRE(status == B3_OK && delivered == sizeof(bytes), "complete fixture read");
    phase = "CONTENT";
    for (mismatch = 0; mismatch < sizeof(bytes); ++mismatch)
        REQUIRE(bytes[mismatch] == (uint8_t)(mismatch & 0xffU), "fixture byte mismatch");
    REQUIRE(line("READ_OK bytes=513\n") == 0, "console output");

    phase = "EOF";
    delivered = 0;
    status = b3_read(handle, bytes, 1, &delivered);
    REQUIRE(status == B3_EOF && delivered == 0, "EOF status/count");
    REQUIRE(line("EOF_OK\n") == 0, "console output");
    phase = "SEEK";
    status = b3_seek(handle, 511, B1_SET);
    REQUIRE(status == B3_OK, "seek SET");
    delivered = 0;
    status = b3_read(handle, bytes, 2, &delivered);
    REQUIRE(status == B3_OK && delivered == 2 && bytes[0] == 0xff && bytes[1] == 0,
            "seek boundary bytes");
    status = b3_seek(handle, 0, B1_END);
    REQUIRE(status == B3_OK, "seek END");
    status = b3_info(handle, &position, &size);
    REQUIRE(status == B3_OK && position == 513 && size == 513, "END position");
    delivered = 0;
    status = b3_read(handle, bytes, 1, &delivered);
    REQUIRE(status == B3_EOF && delivered == 0, "END EOF");
    REQUIRE(line("SEEK_OK\n") == 0, "console output");

    phase = "CLOSE";
    status = b3_close(handle);
    opened = 0; /* B3 removes its tracked token even when close reports error. */
    REQUIRE(status == B3_OK, "close");
    REQUIRE(line("CLOSE_OK\n") == 0, "console output");

finish:
    /* Capture the primary diagnostic BEFORE cleanup. App validation failures
     * need not have a B3 first error (e.g. wrong fixture content). */
    if (attempted) (void)b3_diagnostics(&before);
    if (failed) {
        (void)line("APP_FAIL phase=%s status=%u bytes=%lu reason=%s index=%u\n",
                   phase, (unsigned)status, (unsigned long)delivered, reason, mismatch);
        if (attempted)
            (void)line("BACKEND first=%u status=%u phase=%u fat=%u sd=%u fw=%08lx delivered=%08lx%08lx\n",
                       (unsigned)before.first_set, (unsigned)before.first_status, (unsigned)before.phase,
                       (unsigned)before.fat_status, (unsigned)before.sd_status,
                       (unsigned long)before.sd.firmware_init_status,
                       (unsigned long)(uint32_t)(before.delivered >> 32),
                       (unsigned long)(uint32_t)before.delivered);
    }
#ifdef SDRO_CSD_DIAGNOSTIC
    if (failed && attempted) {
        /* Copy inert evidence after B3's internal cleanup; never replace status. */
        if (b3_csd_snapshot(&csd_snapshot) < 0 ||
            !csdd_format(&csd_snapshot, csd_text, sizeof(csd_text))) {
            console_error = 1;
        } else {
            (void)line("%s", csd_text);
        }
    }
#endif
    if (opened) {
        b3_status secondary = b3_close(handle);
        if (secondary != B3_OK)
            (void)line("CLEANUP_FAIL close=%u\n", (unsigned)secondary);
        /* Preserve original failure, never retry close. */
    }
    if (attempted) {
        b3_shutdown();
        if (b3_diagnostics(&after) != B3_OK || after.state != B3_UNINITIALIZED || after.capacity ||
            (!failed && after.first_set)) {
            if (!failed) (void)line("APP_FAIL phase=SHUTDOWN status=%u bytes=0\n", (unsigned)B3_STATE);
            failed = 1;
        } else if (line("SHUTDOWN_OK\n") != 0) failed = 1;
    }
    if (!failed && !console_error && line("APP_PASS\n") == 0) return 0;
    return 1;
#undef REQUIRE
}
