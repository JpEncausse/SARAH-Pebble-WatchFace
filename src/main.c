
#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
#include "http.h"
#include "itoa.h"

#define SARAH_HTTP_COOKIE 1
	
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))	
#define MY_UUID { 0x30, 0xB2, 0x7D, 0x97, 0x22, 0x46, 0x42, 0xB4, 0x98, 0x23, 0x9C, 0x68, 0xD1, 0xF3, 0xB7, 0x95 }

static int our_latitude, our_longitude, our_ctx, our_btn;
static bool located;

PBL_APP_INFO_SIMPLE(HTTP_UUID, "SARAH", "Encausse.net", 1 /* App version */);

Window window;
void request_sarah();

// ==========================================
//  DETAIL WINDOW
// ==========================================

Window detailW;
TextLayer detailW_text;

void up_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
  (void)recognizer;
  (void)window;
  our_btn = 1;
  request_sarah();
}
void select_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
  (void)recognizer;
  (void)window;
  our_btn = 2;
  request_sarah();
}
void down_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
  (void)recognizer;
  (void)window;
  our_btn = 3;
  request_sarah();
}
void select_long_click_handler(ClickRecognizerRef recognizer, Window *window) {
  (void)recognizer;
  (void)window;
  our_btn = 4;
  request_sarah();
}

void click_config_provider(ClickConfig **config, Window *window) {
  (void)window;

  config[BUTTON_ID_SELECT]->click.handler = (ClickHandler) select_single_click_handler;
  config[BUTTON_ID_SELECT]->long_click.handler = (ClickHandler) select_long_click_handler;

  config[BUTTON_ID_UP]->click.handler = (ClickHandler) up_single_click_handler;
  config[BUTTON_ID_UP]->click.repeat_interval_ms = 100;

  config[BUTTON_ID_DOWN]->click.handler = (ClickHandler) down_single_click_handler;
  config[BUTTON_ID_DOWN]->click.repeat_interval_ms = 100;
}

char buffer[256];
bool initW = false;
void showDetail(char* msg){
  if (!initW){
	initW = true;
    window_init(&detailW, "Menu Details");

    text_layer_init(&detailW_text, GRect(0,0,144,180));
    text_layer_set_text_alignment(&detailW_text, GTextAlignmentRight); // Center the text.
    text_layer_set_font(&detailW_text, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));

    buffer[0] = 0; // Ensure the message starts with a null so strcat will overwrite it.

    text_layer_set_text(&detailW_text, buffer);
    layer_add_child(&detailW.layer, &detailW_text.layer);
  
    // Attach our desired button functionality
    window_set_click_config_provider(&detailW, (ClickConfigProvider) click_config_provider);
  }
	
  buffer[0] = 0; // Ensure the message starts with a null so strcat will overwrite it.
  strcat(buffer, msg);

  // The back button will dismiss the current window, not close the app.  
  // So just press back to go back to the master view.
  if (our_btn == -1){ 
    window_stack_push(&detailW, true); 
  } else { 
	layer_mark_dirty(&detailW_text.layer);
  }
}

// ==========================================
//  MENULAYER
// ==========================================

MenuLayer mainMenu;

char* status[]  = { "SUCCESS ", "FAILED  ", "FAILED 1", "FAILED 2" };
char* choices[] = { " Domotique", " Scraping", " Powerpoint", " Multimedia", " Blink", " Divers 1", " Divers 2"	};
char* urls[] = {
  "http://192.168.0.36:8080/sarah/pebble",
  "http://192.168.0.36:8080/sarah/pebble",
  "http://192.168.0.42:8080/sarah/pebble",
  "http://192.168.0.8:8080/sarah/pebble",
  "http://192.168.0.8:8080/sarah/pebble",
  "http://192.168.0.8:8080/sarah/pebble",
  "http://192.168.0.8:8080/sarah/pebble"
};

void mainMenu_select_click(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context){
  // Show the detail view when select is pressed.
  our_ctx = cell_index->row;
  our_btn = -1;
  request_sarah();
}
void mainMenu_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context){
  // Adding the row number as text on the row cell.
  graphics_context_set_text_color(ctx, GColorBlack); // This is important.
  graphics_text_draw(ctx, choices[cell_index->row], fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(0,0,cell_layer->frame.size.w,cell_layer->frame.size.h), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  // Just saying cell_layer->frame for the 4th argument doesn't work.  Probably because the GContext is relative to the cell already, 
  // but the cell_layer.frame is relative to the menulayer or the screen or something.
}
uint16_t mainMenu_get_num_rows_in_section(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context){
  return ARRAY_SIZE(choices);
}
int16_t mainMenu_get_cell_height(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context){
  return 32; // Always 20px tall for a normal cell
}

uint16_t mainMenu_get_num_sections(struct MenuLayer *menu_layer, void *callback_context) { 
  return 1; // ARRAY_SIZE(headers);
}
int16_t mainMenu_get_header_height(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context){ 
  return 0; // Always 30px tall for a header cell
}
/*
void mainMenu_draw_header(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *callback_context){
  // Adding the header number as text on the header cell.
  graphics_context_set_text_color(ctx, GColorBlack); // This is important.
  graphics_text_draw(ctx, "HEADER", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(0,0,cell_layer->frame.size.w,cell_layer->frame.size.h), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}*/

MenuLayerCallbacks cbacks;

// ==========================================
//  HTTP REQUEST
// ==========================================

// POST variables
#define SARAH_KEY_LATITUDE 1
#define SARAH_KEY_LONGITUDE 2
#define SARAH_KEY_CTX 3
#define SARAH_KEY_BTN 4

// Received variables
#define SARAH_KEY_MENU 1

void failed(int32_t cookie, int http_status, void* context) {
  if(cookie == 0 || cookie == SARAH_HTTP_COOKIE) {
	// sarah_layer_set_icon(&sarah_layer, SARAH_ICON_NO_DATA);
	showDetail(status[1]);
  }
}

void success(int32_t cookie, int http_status, DictionaryIterator* received, void* context) {
  if(cookie != SARAH_HTTP_COOKIE) return;
	
  Tuple* data_tuple = dict_find(received, SARAH_KEY_MENU);
  if(data_tuple) {
	char* str = (char*) data_tuple->value->cstring;
	showDetail(str);
	return;
  }

  showDetail(status[0]);
}

void location(float latitude, float longitude, float altitude, float accuracy, void* context) {
  // Fix the floats
  our_latitude = latitude * 10000;
  our_longitude = longitude * 10000;
  located = true;
  request_sarah();
  // Timeout to refresh data
  // set_timer((AppContextRef)context); 
}

// void set_timer(AppContextRef ctx) {
//   app_timer_send_event(ctx, 1740000, 1);
// }

void request_sarah() {
  if(!located) {
    http_location_request();
    return;
  }
	
  // Build the HTTP request
  DictionaryIterator *body;
	HTTPResult result = http_out_get(urls[our_ctx], SARAH_HTTP_COOKIE, &body);
  if(result != HTTP_OK) {
	// sarah_layer_set_icon(&sarah_layer, SARAH_ICON_NO_DATA);
	showDetail(status[2]);
	return;
  }
  dict_write_int32(body, SARAH_KEY_LATITUDE, our_latitude);
  dict_write_int32(body, SARAH_KEY_LONGITUDE, our_longitude);
  dict_write_cstring(body, SARAH_KEY_CTX, choices[our_ctx]);
  dict_write_int32(body, SARAH_KEY_BTN, our_btn);

  // Send it.
  if(http_out_send() != HTTP_OK) {
	// sarah_layer_set_icon(&sarah_layer, SARAH_ICON_NO_DATA);
	showDetail(status[3]);
	return;
  }
}

// ==========================================
//  INITIALISATION
// ==========================================

void handle_init(AppContextRef ctx) {
  (void)ctx;
  window_init(&window, "S.A.R.A.H.");
  window_stack_push(&window, true /* Animated */);

  // -15 because of the title bar.  I could go full screen, but opted not to.
  menu_layer_init(&mainMenu, GRect(0,0,window.layer.frame.size.w,window.layer.frame.size.h-15)); 
	
  // I assume this sets the Window's button callbacks to the MenuLayer's default button callbacks.
  menu_layer_set_click_config_onto_window(&mainMenu, &window); 
	
  // Set all of our callbacks.
  cbacks.get_num_sections = &mainMenu_get_num_sections;    // Gets called at the beginning of drawing the table.
  cbacks.get_num_rows = &mainMenu_get_num_rows_in_section; // Gets called at the beginning of drawing each section.
  cbacks.get_cell_height = &mainMenu_get_cell_height;      // Gets called at the beginning of drawing each normal cell.
  cbacks.get_header_height = &mainMenu_get_header_height;  // Gets called at the beginning of drawing each header cell.
  cbacks.select_click = &mainMenu_select_click;            // Gets called when select is pressed.
  cbacks.draw_row = &mainMenu_draw_row;                    // Gets called to set the content of a normal cell.
  // cbacks.draw_header = &mainMenu_draw_header;              // Gets called to set the content of a header cell.
  // cbacks.selection_changed = &func(struct MenuLayer *menu_layer, MenuIndex new_index, MenuIndex old_index, void *callback_context) // I assume this would be called whenever an up or down button was pressed.
  // cbacks.select_long_click = &func(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) // I didn't use this.
  menu_layer_set_callbacks(&mainMenu, NULL, cbacks); // I have no idea what the middle argumnte, void *callback_context, is for.  It seems to work fine with NULL there.
  layer_add_child(&window.layer, menu_layer_get_layer(&mainMenu));
	
	
  // HTTP Callback registration
  http_register_callbacks((HTTPCallbacks){
	.failure=failed,
	.success=success,
	.location=location
	}, (void*)ctx);
	
  located = false;
}


void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
    .messaging_info = {
	  .buffer_sizes = {
        .inbound = 256,
		.outbound = 256
	  }
	},
  };
  app_event_loop(params, &handlers);
}


