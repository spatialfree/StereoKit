﻿#include "virtual_keyboard.h"
#include "virtual_keyboard_layouts.h"
#include "../stereokit_ui.h"
#include "../systems/input_keyboard.h"
#include "../libraries/array.h"
#include "../libraries/unicode.h"

namespace sk {

bool32_t           keyboard_open         = false;
pose_t             keyboard_pose         = pose_identity;
array_t<key_>      keyboard_pressed_keys = {};
const keylayout_t* keyboard_active_layout;
text_context_      keyboard_text_context;

bool32_t           keyboard_fn    = false;
bool32_t           keyboard_altgr = false;
bool32_t           keyboard_shift = false;
bool32_t           keyboard_ctrl  = false;
bool32_t           keyboard_alt   = false;

///////////////////////////////////////////

const keylayout_t* virtualkeyboard_get_system_keyboard_layout() {
	return &virtualkeyboard_layout_en_us;
}

///////////////////////////////////////////

void virtualkeyboard_open(bool open, text_context_ type) {
	keyboard_text_context = type;
	if (open != keyboard_open) {
		// Position the keyboard in front of the user
		keyboard_pose = *input_head();
		keyboard_pose.position   += keyboard_pose.orientation * vec3_forward * 0.5f + vec3{0, -.2f, 0};
		keyboard_pose.orientation = quat_lookat(keyboard_pose.position, input_head()->position);

		if (open) {
			if (keyboard_alt  ) { input_keyboard_inject_release(key_alt  ); keyboard_alt   = false; }
			if (keyboard_altgr) { input_keyboard_inject_release(key_alt  ); keyboard_altgr = false; }
			if (keyboard_ctrl ) { input_keyboard_inject_release(key_ctrl ); keyboard_ctrl  = false; }
			if (keyboard_shift) { input_keyboard_inject_release(key_shift); keyboard_shift = false; }
			if (keyboard_fn   ) {                                           keyboard_fn    = false; }
		}
		keyboard_open = open;
	}
}

///////////////////////////////////////////

bool virtualkeyboard_get_open() {
	return keyboard_open;
}

///////////////////////////////////////////

void virtualkeyboard_initialize() {
	keyboard_active_layout = virtualkeyboard_get_system_keyboard_layout();
}

///////////////////////////////////////////

void send_key_data(const char16_t* charkey,key_ key) {
	keyboard_pressed_keys.add(key);
	input_keyboard_inject_press(key);
	char32_t c = 0;
	while (utf16_decode_fast_b(charkey, &charkey, &c)) {
		input_text_inject_char(c);
	}
}

///////////////////////////////////////////

void remove_last_clicked_keys() {
	for (int i = 0; i < keyboard_pressed_keys.count; i++) {
		input_keyboard_inject_release(keyboard_pressed_keys[0]);
		keyboard_pressed_keys.remove(0);
	}
}

///////////////////////////////////////////

void virtualkeyboard_keypress(keylayout_key_t key) {
	send_key_data(key.clicked_text, key.key_event_type);
	if (key.special_key == special_key_close_keyboard) {
		keyboard_open = false;
	}
}

///////////////////////////////////////////

void virtualkeyboard_update() {
	if (!keyboard_open) return;
	if (keyboard_active_layout == nullptr) return;

	remove_last_clicked_keys();
	hierarchy_push(render_get_cam_root());
	ui_window_begin("SK/Keyboard", keyboard_pose, {0,0}, ui_win_body);
	ui_push_no_keyboard_loss(true);

	// Check layer keys for switching between keyboard layout layers
	int layer_index = 0;
	if      (keyboard_shift && keyboard_altgr) layer_index = 3;
	else if (keyboard_fn)                      layer_index = 4;
	else if (keyboard_shift)                   layer_index = 1;
	else if (keyboard_altgr)                   layer_index = 2;

	// Get the right layout for this text context
	const keylayout_layer_t* layer = nullptr;
	switch (keyboard_text_context) {
	case text_context_text:  layer = &keyboard_active_layout->text  [layer_index]; break;
	case text_context_number:layer = &keyboard_active_layout->number[layer_index]; break;
	case text_context_uri:   layer = &keyboard_active_layout->uri   [layer_index]; break;
	default:                 layer = &keyboard_active_layout->text  [layer_index]; break;
	//case text_context_number_decimal:        layer = &keyboard_active_layout->number_decimal_layer       [layer_index]; break;
	//case text_context_number_signed:         layer = &keyboard_active_layout->number_signed_layer        [layer_index]; break;
	//case text_context_number_signed_decimal: layer = &keyboard_active_layout->number_signed_decimal_layer[layer_index]; break;
	}

	// Draw the keyboard
	float button_size = ui_line_height();
	for (int row = 0; row < 6; row++) {
		for (int i = 0; i < 35; i++) {
			keylayout_key_t key  = layer->keys[row][i];
			vec2            size = vec2{ button_size * key.width, button_size };
			if (key.width == 0) continue;

			ui_push_idi((i * row) + 1000);
			if (key.special_key == special_key_spacer) {
				ui_layout_reserve(size);
			} else if (key.special_key == special_key_fn) {
				ui_toggle_sz_16(key.display_text, keyboard_fn, size);
			} else if (key.special_key == special_key_alt) {
				if (ui_toggle_sz_16(key.display_text, keyboard_alt, size)) {
					if (keyboard_alt) input_keyboard_inject_press  (key_alt);
					else              input_keyboard_inject_release(key_alt);
				}
			} else if (key.special_key == special_key_ctrl) {
				if (ui_toggle_sz_16(key.display_text, keyboard_ctrl, size)) {
					if (keyboard_ctrl) input_keyboard_inject_press  (key_ctrl);
					else               input_keyboard_inject_release(key_ctrl);
				}
			} else if (key.special_key == special_key_shift) {
				if (ui_toggle_sz_16(key.display_text, keyboard_shift, size)) {
					if (keyboard_shift) input_keyboard_inject_press  (key_shift);
					else                input_keyboard_inject_release(key_shift);
				}
			} else if (key.special_key == special_key_alt_gr) {
				if (ui_toggle_sz_16(key.display_text, keyboard_altgr, size)) {
					if (keyboard_altgr) input_keyboard_inject_press  (key_alt);
					else                input_keyboard_inject_release(key_alt);
				}
			} else if (ui_button_sz_16(key.display_text, size)) {
				virtualkeyboard_keypress(key);
			}
			ui_pop_id();
			ui_sameline();
		}
		ui_nextline();
	}
	ui_pop_no_keyboard_loss();
	ui_window_end();
	hierarchy_pop();
}

}