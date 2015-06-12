#include <pebble.h>
#define JS_READY 7
#define NEARBY_STOPS 1
#define DEPARTURES 2
#define NUM_MENU_ICONS 3
#define NUM_FIRST_MENU_ITEMS 3
#define NUM_SECOND_MENU_ITEMS 1
#define MAX_STOPS 50
#define MAX_NAME 30
#define MAX_LINES 50
#define MAX_LINE_NAME 30
#define MAX_TIME_LEN 30

static bool js_ready = false;

static Window *window;
static Window *bus_window;
static MenuLayer *s_menu_layer;
static MenuLayer *bus_menu_layer;
static uint16_t lines_count = 1;
static uint16_t close_stops_count = 0;
static char available_stops[MAX_STOPS][MAX_NAME];
static char available_lines[MAX_LINES][MAX_LINE_NAME];
static char time_strings[MAX_LINES][MAX_TIME_LEN];

static uint16_t menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
    return 1;
}

static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    return close_stops_count + 1;    
}

static int16_t menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
    switch (section_index) {
      case 0:
        menu_cell_basic_header_draw(ctx, cell_layer, close_stops_count == 0 ? "Fetching..." : "Stops nearby:");
        break;
    }
}

static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
    // Determine which section we're going to draw in
    switch (cell_index->section) {
      case 0:
        // Use the row to specify which item we'll draw
        switch (cell_index->row) {
          case 0:
            menu_cell_basic_draw(ctx, cell_layer, "Refresh", NULL, NULL);
            break;
          default:
            // This is a basic menu item with a title and subtitle
            menu_cell_basic_draw(ctx, cell_layer, available_stops[cell_index->row - 1], NULL, NULL);
            break;
        }
        break;
    }
}

static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
    layer_mark_dirty(menu_layer_get_layer(s_menu_layer));
    DictionaryIterator* iter;
    APP_LOG(APP_LOG_LEVEL_INFO, "select");
    switch (cell_index->row) {
        case 0:
          close_stops_count = 0;
          app_message_outbox_begin(&iter); 
          dict_write_cstring(iter, 0, "#GET STOPS");
          app_message_outbox_send();
          break;
        default:
          app_message_outbox_begin(&iter); 
          dict_write_cstring(iter, 0, available_stops[cell_index->row - 1]);
          app_message_outbox_send();
          window_stack_push(bus_window, true);
          break;
    }
}

static int16_t menu_get_cell_height(MenuLayer* menu_layer, MenuIndex* cell_index, void* ctx) {
    return 16;
}

static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    s_menu_layer = menu_layer_create(bounds);
    menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks){
      .get_num_sections = menu_get_num_sections_callback,
      .get_num_rows = menu_get_num_rows_callback,
      .get_cell_height = menu_get_cell_height,
      .get_header_height = menu_get_header_height_callback,
      .draw_header = menu_draw_header_callback,
      .draw_row = menu_draw_row_callback,
      .select_click = menu_select_callback,
    });

    menu_layer_set_click_config_onto_window(s_menu_layer, window);
    layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
}

static void window_unload(Window *window) {
    menu_layer_destroy(s_menu_layer);
}

static uint16_t bus_menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
    return 1;
}

static uint16_t bus_menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    return lines_count;    
}

static int16_t bus_menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void bus_menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
    switch (section_index) {
      case 0:
        menu_cell_basic_header_draw(ctx, cell_layer, lines_count == 0 ? "Fetching..." : "Available lines:");
        break;
    }
}

static void bus_menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
    // Determine which section we're going to draw in
    switch (cell_index->section) {
      case 0:
        // Use the row to specify which item we'll draw
        switch (cell_index->row) {
          default:
            // This is a basic menu item with a title and subtitle
            menu_cell_basic_draw(ctx, cell_layer, 
                    available_lines[cell_index->row],
                    time_strings[cell_index->row], NULL);
            break;
        }
        break;
    }
}

static void bus_menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
    layer_mark_dirty(menu_layer_get_layer(bus_menu_layer));
    APP_LOG(APP_LOG_LEVEL_INFO, "select");
    switch (cell_index->row) {
    }
}
static void bus_window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(bus_window);
    GRect bounds = layer_get_bounds(window_layer);

    bus_menu_layer = menu_layer_create(bounds);
    menu_layer_set_callbacks(bus_menu_layer, NULL, (MenuLayerCallbacks){
      .get_num_sections = bus_menu_get_num_sections_callback,
      .get_num_rows = bus_menu_get_num_rows_callback,
      .get_header_height = bus_menu_get_header_height_callback,
      .draw_header = bus_menu_draw_header_callback,
      .draw_row = bus_menu_draw_row_callback,
      .select_click = bus_menu_select_callback,
    });

    menu_layer_set_click_config_onto_window(bus_menu_layer, bus_window);
    layer_add_child(window_layer, menu_layer_get_layer(bus_menu_layer));
}

static void bus_window_unload(Window *window) {
    menu_layer_destroy(bus_menu_layer);
    available_lines[0][0] = 0;
    time_strings[0][0] = 0;
    lines_count = 1;
}

static void inbox_received(DictionaryIterator* iterator, void* context) {
    APP_LOG(APP_LOG_LEVEL_INFO, "inbox received");
    Tuple *tuple = dict_read_first(iterator);
    int type = -1;
    while (tuple) {
        if (tuple->key == 0) {
            type = tuple->value->uint32;
            break;
        }
        tuple = dict_read_next(iterator);
    }
    char qqq[2];
    qqq[1] = 0;
    qqq[0] = type + '0';
    APP_LOG(APP_LOG_LEVEL_INFO, qqq);
    switch (type) {
      case JS_READY:
        js_ready = true;
        break;
      case NEARBY_STOPS:
        close_stops_count = 0;
        layer_mark_dirty(menu_layer_get_layer(s_menu_layer));
        tuple = dict_read_first(iterator);
        while (tuple) {
            if (tuple->key != 0) {
                if (tuple->key < MAX_STOPS) {
                    strncpy(available_stops[tuple->key - 1], tuple->value->cstring,
                            MAX_NAME > tuple->length ? MAX_NAME : tuple->length);
                    available_stops[tuple->key -1][MAX_NAME - 1] = 0;
                    close_stops_count++;
                }
            }
            tuple = dict_read_next(iterator);
        }
        menu_layer_reload_data(s_menu_layer);
        break;
      case DEPARTURES:
        layer_mark_dirty(menu_layer_get_layer(bus_menu_layer));
        tuple = dict_read_first(iterator);
        while (tuple) {
            if (tuple->key != 0) {
                int cur_index = tuple->key / 2;
                if (cur_index < MAX_LINES) {
                    bool vtype = tuple->key % 2;
                    char* dest = vtype ?
                        available_lines[tuple->key / 2] : time_strings[(tuple->key - 1) / 2];
                    int limit = vtype ? MAX_LINE_NAME : MAX_TIME_LEN;
                    strncpy(dest, tuple->value->cstring,
                            limit > tuple->length ? limit : tuple->length);
                    dest[limit - 1] = 0;
                    if (cur_index > lines_count)
                        lines_count = cur_index;
                }
            }
            tuple = dict_read_next(iterator);
        }
        menu_layer_reload_data(bus_menu_layer);
        break;
    }
}
static void inbox_dropped(AppMessageResult reason, void* context) {
    APP_LOG(APP_LOG_LEVEL_INFO, "inbox dropped");
}
static void outbox_failed(DictionaryIterator* iterator, AppMessageResult result, void* context) {
    static char buff[2];
    buff[0] = (int)result + '0';
    buff[1] = 0;
    APP_LOG(APP_LOG_LEVEL_INFO, "outbox failed");
    APP_LOG(APP_LOG_LEVEL_INFO, buff);
}
static void outbox_sent(DictionaryIterator* iterator, void* context) {
    APP_LOG(APP_LOG_LEVEL_INFO, "outbox sent"); 
}


static void init(void) {
    // app msgs
    app_message_register_inbox_received(inbox_received);
    app_message_register_inbox_dropped(inbox_dropped);
    app_message_register_outbox_failed(outbox_failed);
    app_message_register_outbox_sent(outbox_sent);
    app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
    // window
    window = window_create();
    window_set_window_handlers(window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload,
    });
    bus_window = window_create();
    window_set_window_handlers(bus_window, (WindowHandlers) {
        .load = bus_window_load,
        .unload = bus_window_unload,
    });
    const bool animated = true;
    window_stack_push(window, animated);
}

static void deinit(void) {
    window_destroy(window);
}

int main(void) {
    init();

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

    app_event_loop();
    deinit();
}
