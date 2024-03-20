#include "logger.h"

#include "asserts.h"
#include "core/kmemory.h"
#include "core/kstring.h"

// TODO: temporary
#include <stdarg.h>
#include <stdio.h>

struct platform_state;

typedef struct logger_system_state {
    struct platform_state* platform;
} logger_system_state;

static logger_system_state* state_ptr;

b8 logging_initialize(u64* memory_requirement, void* state, void* config) {
    *memory_requirement = sizeof(logger_system_state);
    if (state == 0) {
        return true;
    }

    state_ptr = state;
    state_ptr->platform = 0;

    return true;
}

void logging_shutdown(void* state) {
    // TODO: cleanup logging/write queued entries.
    state_ptr = 0;
}

void log_output(log_level level, const char* message, ...) {
    // TODO: These string operations are all pretty slow. This needs to be
    // moved to another thread eventually, along with the file writes, to
    // avoid slowing things down while the engine is trying to run.
    const char* level_strings[6] = {"[FATAL]: ", "[ERROR]: ", "[WARN]:  ", "[INFO]:  ", "[DEBUG]: ", "[TRACE]: "};
    b8 is_error = level < LOG_LEVEL_WARN;

    // Technically imposes a 32k character limit on a single log entry, but...
    // DON'T DO THAT!
    char out_message[32000];
    kzero_memory(out_message, sizeof(out_message));

    // Format original message.
    // NOTE: Oddly enough, MS's headers override the GCC/Clang va_list type with a "typedef char* va_list" in some
    // cases, and as a result throws a strange error here. The workaround for now is to just use __builtin_va_list,
    // which is the type GCC/Clang's va_start expects.
    __builtin_va_list arg_ptr;
    va_start(arg_ptr, message);
    string_format_v(out_message, message, arg_ptr);
    va_end(arg_ptr);

    // Prepend log level to message.
    string_format(out_message, "%s%s\n", level_strings[level], out_message);

    // Print accordingly
    if (is_error) {
        fprintf(stderr, "%s", out_message);
    } else {
        fprintf(stdout, "%s", out_message);
    }
}

void report_assertion_failure(const char* expression, const char* message, const char* file, i32 line) {
    log_output(LOG_LEVEL_FATAL, "Assertion Failure: %s, message: '%s', in file: %s, line: %d\n", expression, message, file, line);
}
