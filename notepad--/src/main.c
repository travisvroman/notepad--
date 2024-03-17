#include "application.h"
#include "core/kmemory.h"
#include "core/logger.h"
#include "defines.h"
#include "platform/platform.h"

i32 main(i32 argc, const char** argv) {
    application_state app;

    // Initialize application.
    if (!application_create(&app)) {
        KERROR("Failed to create applicaton.");
        return -1;
    }

    // Main application loop.
    while (true) {
        if (!application_update(&app)) {
            break;
        }
    }

    // Shutdown/destroy application
    application_destroy(&app);

    return 0;
}
