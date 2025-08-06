#include <furi.h>
#include <gui/gui.h>
#include <furi_hal.h>
#include <input/input.h>
#include <furi_hal_serial.h>
#include <stdlib.h>

#define UART_RX_BUFFER_SIZE 128

static FuriStreamBuffer* uart_rx_stream = NULL;
static char uart_display_line[33] = {0};
static size_t uart_display_len = 0;

// Draw callback for the screen
static void draw_callback(Canvas* canvas, void* ctx) {
    (void)ctx;
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 10, 20, "UART Data:");
    canvas_draw_str(canvas, 10, 40, uart_display_line);
}

// Input event callback
static void input_callback(InputEvent* event, void* ctx) {
    bool* running = (bool*)ctx;
    if(event->type == InputTypePress && event->key == InputKeyBack) {
        *running = false;
    }
}

// UART RX callback
static void uart_rx_cb(FuriHalSerialHandle* handle, FuriHalSerialRxEvent event, void* ctx) {
    UNUSED(handle);
    UNUSED(event);
    FuriStreamBuffer* stream = ctx;
    furi_stream_buffer_send(stream, "test", 1, 0);
}

int32_t hello_world_app(void* p) {
    (void)p;

    bool running = true;
    uint8_t rx_char;

    // Setup GUI
    Gui* gui = furi_record_open(RECORD_GUI);
    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, draw_callback, NULL);
    view_port_input_callback_set(view_port, input_callback, &running);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    // Setup UART RX
    uart_rx_stream = furi_stream_buffer_alloc(UART_RX_BUFFER_SIZE, 1);
    FuriHalSerialHandle* handle = furi_hal_serial_control_acquire(FuriHalSerialIdUsart);
    furi_hal_serial_set_br(handle, 115200);
    furi_hal_serial_async_rx_start(handle, uart_rx_cb, uart_rx_stream, false);

    while(running) {
        // Pull from stream buffer
        while(furi_stream_buffer_receive(uart_rx_stream, &rx_char, 1, 0)) {
            if(rx_char == '\n' || uart_display_len >= sizeof(uart_display_line) - 1) {
                uart_display_line[uart_display_len] = '\0';
                uart_display_len = 0;
            } else {
                uart_display_line[uart_display_len++] = rx_char;
                uart_display_line[uart_display_len] = '\0';
            }
        }

        view_port_update(view_port);
        furi_delay_ms(100);
    }

    // Cleanup
    furi_hal_serial_async_rx_stop(handle);
    furi_hal_serial_control_release(handle);
    furi_stream_buffer_free(uart_rx_stream);

    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);
    furi_record_close(RECORD_GUI);

    return 0;
}
