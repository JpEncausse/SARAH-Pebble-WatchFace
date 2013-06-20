
#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
#include "http.h"
#include "itoa.h"

#define SARAH_HTTP_COOKIE 1
#define SARAH_KEY_LATITUDE 1
#define SARAH_KEY_LONGITUDE 2
#define SARAH_KEY_CURRENT 1
	
#define MY_UUID { 0x30, 0xB2, 0x7D, 0x97, 0x22, 0x46, 0x42, 0xB4, 0x98, 0x23, 0x9C, 0x68, 0xD1, 0xF3, 0xB7, 0x95 }
PBL_APP_INFO_SIMPLE(MY_UUID, "S.A.R.A.H.", "Encausse.net", 1 /* App version */);

Window window;

TextLayer textLayer;

// ==========================================
//  HTTP Request callback
// ==========================================


static int our_latitude, our_longitude;
static bool located;

void request_sarah();

void failed(int32_t cookie, int http_status, void* context) {
  if(cookie == 0 || cookie == SARAH_HTTP_COOKIE) {
	// sarah_layer_set_icon(&sarah_layer, SARAH_ICON_NO_DATA);
	text_layer_set_text(&textLayer, "Failed");
  }
}

void success(int32_t cookie, int http_status, DictionaryIterator* received, void* context) {
	//if(cookie != SARAH_HTTP_COOKIE) return;
	//Tuple* data_tuple = dict_find(received, SARAH_KEY_CURRENT);
	/*
	if(data_tuple) {
		// The below bitwise dance is so we can actually fit our precipitation forecast.
		uint16_t value = data_tuple->value->int16;
		uint8_t icon = value >> 11;
		if(icon < 10) {
			sarah_layer_set_icon(&sarah_layer, icon);
		} else {
			sarah_layer_set_icon(&sarah_layer, SARAH_ICON_NO_DATA);
		}
		int16_t temp = value & 0x3ff;
		if(value & 0x400) temp = -temp;
		sarah_layer_set_temperature(&sarah_layer, temp);
	}*/
	
	/*
	Tuple* forecast_tuple = dict_find(received, SARAH_KEY_PRECIPITATION);
	if(forecast_tuple) {
		// It's going to rain!
		memset(precip_forecast, 0, 60);
		memcpy(precip_forecast, forecast_tuple->value->data, forecast_tuple->length > 60 ? 60 : forecast_tuple->length);
		precip_forecast_index = 0;
		sarah_layer_set_precipitation_forecast(&sarah_layer, precip_forecast, 60);
	} else {
		sarah_layer_clear_precipitation_forecast(&sarah_layer);
	}*/
	text_layer_set_text(&textLayer, "Success !");
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


// ==========================================
//  Modify these common button handlers
// ==========================================

void up_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
  (void)recognizer;
  (void)window;

}


void down_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
  (void)recognizer;
  (void)window;

}


void select_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
  (void)recognizer;
  (void)window;
  
  text_layer_set_text(&textLayer, "Requesting !");
  request_sarah();
}


void select_long_click_handler(ClickRecognizerRef recognizer, Window *window) {
  (void)recognizer;
  (void)window;
}


// This usually won't need to be modified

void click_config_provider(ClickConfig **config, Window *window) {
  (void)window;

  config[BUTTON_ID_SELECT]->click.handler = (ClickHandler) select_single_click_handler;

  config[BUTTON_ID_SELECT]->long_click.handler = (ClickHandler) select_long_click_handler;

  config[BUTTON_ID_UP]->click.handler = (ClickHandler) up_single_click_handler;
  config[BUTTON_ID_UP]->click.repeat_interval_ms = 100;

  config[BUTTON_ID_DOWN]->click.handler = (ClickHandler) down_single_click_handler;
  config[BUTTON_ID_DOWN]->click.repeat_interval_ms = 100;
}


// Standard app initialisation

void handle_init(AppContextRef ctx) {
  (void)ctx;
  window_init(&window, "S.A.R.A.H.");
  window_stack_push(&window, true /* Animated */);

  text_layer_init(&textLayer, window.layer.frame);
  text_layer_set_text(&textLayer, "Domotique");
  text_layer_set_font(&textLayer, fonts_get_system_font(FONT_KEY_GOTHAM_30_BLACK));
  layer_add_child(&window.layer, &textLayer.layer);

  // Attach our desired button functionality
  window_set_click_config_provider(&window, (ClickConfigProvider) click_config_provider);
	
	
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

// Send a request to SARAH
void request_sarah() {
  // if(!located) {
  //  http_location_request();
  //  return;
  //}
	
  // Build the HTTP request
  DictionaryIterator *body;
  HTTPResult result = http_out_get("http://192.168.0.8:8080/sarah/pebble", SARAH_HTTP_COOKIE, &body);
  if(result != HTTP_OK) {
	// sarah_layer_set_icon(&sarah_layer, SARAH_ICON_NO_DATA);
	text_layer_set_text(&textLayer, "Failed 1 !");
	return;
  }
  //dict_write_int32(body, SARAH_KEY_LATITUDE, our_latitude);
  //dict_write_int32(body, SARAH_KEY_LONGITUDE, our_longitude);

  // Send it.
  if(http_out_send() != HTTP_OK) {
	// sarah_layer_set_icon(&sarah_layer, SARAH_ICON_NO_DATA);
	text_layer_set_text(&textLayer, "Failed 2 !");
	return;
  }
}
