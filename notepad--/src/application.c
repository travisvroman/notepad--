#include "application.h"

#include "core/event.h"
#include "core/input.h"
#include "core/kmemory.h"
#include "core/logger.h"
#include "platform/platform.h"
#include "renderer/renderer.h"
#include "systems/font_system.h"

// Event handlers
static b8 application_on_event(u16 code, void* sender, void* listener_inst, event_context context);
static b8 application_on_resized(u16 code, void* sender, void* listener_inst, event_context context);

b8 application_create(application_state* out_state) {
    if (!out_state) {
        KFATAL("application_create requires a valid pointer to an application state.");
        return false;
    }

    kzero_memory(out_state, sizeof(application_state));

    if (!memory_system_initialize()) {
        KERROR("Failed to initialize memory system. Aborting application.");
        return false;
    }

    event_system_initialize(&out_state->event_state.state_memory_requirement, 0, 0);
    out_state->event_state.state = kallocate(out_state->event_state.state_memory_requirement, MEMORY_TAG_GAME);
    if (!event_system_initialize(&out_state->event_state.state_memory_requirement, out_state->event_state.state, 0)) {
        KERROR("Failed to startup event system. Aborting application.");
        return false;
    }

    input_system_initialize(&out_state->input_state.state_memory_requirement, 0, 0);
    out_state->input_state.state = kallocate(out_state->input_state.state_memory_requirement, MEMORY_TAG_GAME);
    if (!input_system_initialize(&out_state->input_state.state_memory_requirement, out_state->input_state.state, 0)) {
        KERROR("Failed to startup input system. Aborting application.");
        return false;
    }

    platform_system_startup(&out_state->platform_state.state_memory_requirement, 0, 0);
    out_state->platform_state.state = kallocate(out_state->platform_state.state_memory_requirement, MEMORY_TAG_GAME);
    platform_system_config plat_config = {0};
    plat_config.application_name = "Notepad--";
    plat_config.x = 100;
    plat_config.y = 100;
    plat_config.width = 1280;
    plat_config.height = 720;
    if (!platform_system_startup(&out_state->platform_state.state_memory_requirement, out_state->platform_state.state, &plat_config)) {
        KERROR("Failed to startup platform layer. Aborting application.");
        return false;
    }

    // Register for global events.
    event_register(EVENT_CODE_APPLICATION_QUIT, out_state, application_on_event);
    event_register(EVENT_CODE_RESIZED, out_state, application_on_resized);
    event_register(EVENT_CODE_KEY_PRESSED, out_state, application_on_event);

    out_state->is_suspended = false;
    out_state->width = 1280;
    out_state->height = 720;
    out_state->resizing = false;
    out_state->frames_since_resize = 0;

    // Renderer.
    renderer_config render_config = {0};
    render_config.platform = out_state->platform_state.state;
    renderer_initialize(&out_state->renderer_state.state_memory_requirement, 0, render_config);
    out_state->renderer_state.state = kallocate(out_state->renderer_state.state_memory_requirement, MEMORY_TAG_GAME);
    if (!renderer_initialize(&out_state->renderer_state.state_memory_requirement, out_state->renderer_state.state, render_config)) {
        KERROR("Failed to initialize renderer system. Aborting application.");
        return false;
    }

    font_system_config font_sys_config = {0};
    font_sys_config.renderer = out_state->renderer_state.state;
    font_sys_config.auto_release = true;
    font_sys_config.max_system_font_count = 23;
    font_system_initialize(&out_state->font_state.state_memory_requirement, 0, &font_sys_config);
    out_state->font_state.state = kallocate(out_state->font_state.state_memory_requirement, MEMORY_TAG_GAME);
    if (!font_system_initialize(&out_state->font_state.state_memory_requirement, out_state->font_state.state, &font_sys_config)) {
        KERROR("Failed to initialize font system. Aborting application.");
        return false;
    }

    system_font_config default_font = {0};
    default_font.name = "MesloLGS NF";
    default_font.default_size = 20;
    default_font.resource_name = "";  // TODO: not used.
    if (font_system_system_font_load(&default_font)) {
        KINFO("Default font loaded.");
    } else {
        KERROR("Failed to load default font. Aborting application.");
        return false;
    }

    // Ensure the state flag is flipped.
    out_state->is_running = true;

    return true;
}

void application_destroy(application_state* state) {
    if (state) {
    }
}

b8 application_update(application_state* state) {
    if (!state) {
        return false;
    }

    if (!platform_pump_messages(state->platform_state.state)) {
        return false;
    }

    if (!state->is_running) {
        return false;
    }
    if (!state->is_suspended) {
        // TODO: Update clock and get delta time.
        /* kclock_update(&state->clock);
        f64 current_time = state->clock.elapsed;
        f64 delta = (current_time - state->last_time);
        f64 frame_start_time = platform_get_absolute_time(); */

        // TODO: Update systems except for input (handled below).

        // Make sure the window is not currently being resized by waiting a designated
        // number of frames after the last resize operation before performing the backend updates.
        if (state->resizing) {
            state->frames_since_resize++;

            // If the required number of frames have passed since the resize, go ahead and perform the actual updates.
            if (state->frames_since_resize >= 30) {
                // TODO: notify the renderer of the resize.
                /* renderer_on_resized(state->width, state->height); */

                // NOTE: Don't bother checking the result of this, since this will likely
                // recreate the swapchain and boot to the next frame anyway.
                /* renderer_frame_prepare(&state->p_frame_data); */

                // Notify the application of the resize.

                state->frames_since_resize = 0;
                state->resizing = false;
            } else {
                // Skip rendering the frame and try again next time.
                // NOTE: Simulate a frame being "drawn" at 60 FPS.
                platform_sleep(state->platform_state.state, 16);
            }

            // Either way, don't process this frame any further while resizing.
            // Try again next frame.
            return true;
        }

        // Render loop - prepare, begin, end.
        if (!renderer_frame_prepare(state->renderer_state.state)) {
            KERROR("Render frame prepare failed.");
            return false;
        }

        if (!renderer_frame_begin(state->renderer_state.state)) {
            KERROR("Render frame begin failed.");
            return false;
        }

        // TODO: Draw stuff.

        if (!renderer_frame_end(state->renderer_state.state)) {
            KERROR("Render frame end failed.");
            return false;
        }

        // NOTE: Input update/state copying should always be handled
        // after any input should be recorded; I.E. before this line.
        // As a safety, input is the last thing to be updated before
        // this frame ends.
        input_update();
    }

    return true;
}

static b8 application_on_event(u16 code, void* sender, void* listener_inst, event_context context) {
    application_state* state = listener_inst;
    switch (code) {
        case EVENT_CODE_APPLICATION_QUIT: {
            KINFO("EVENT_CODE_APPLICATION_QUIT recieved, shutting down.\n");
            state->is_running = false;
            return true;
        }
        case EVENT_CODE_KEY_PRESSED: {
            u16 key_code = context.data.u16[0];
            /* u16 repeat_count = context.data.u16[1]; */
            if (key_code == KEY_ESCAPE) {
                event_fire(EVENT_CODE_APPLICATION_QUIT, state, (event_context){});
            }
        }
    }

    return false;
}

static b8 application_on_resized(u16 code, void* sender, void* listener_inst, event_context context) {
    application_state* state = listener_inst;
    if (code == EVENT_CODE_RESIZED) {
        // Flag as resizing and store the change, but wait to regenerate.
        state->resizing = true;
        // Also reset the frame count since the last  resize operation.
        state->frames_since_resize = 0;

        u16 width = context.data.u16[0];
        u16 height = context.data.u16[1];

        // Check if different. If so, trigger a resize event.
        if (width != state->width || height != state->height) {
            state->width = width;
            state->height = height;

            KDEBUG("Window resize: %i, %i", width, height);

            // Handle minimization
            if (width == 0 || height == 0) {
                KINFO("Window minimized, suspending application.");
                state->is_suspended = true;
                return true;
            } else {
                if (state->is_suspended) {
                    KINFO("Window restored, resuming application.");
                    state->is_suspended = false;
                }
            }
        }
    }

    // Event purposely not handled to allow other listeners to get this.
    return false;
}
