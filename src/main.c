
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>

#include "../vendor/vblibc/src/vblibc_string.h"
#include "kbrd.h"

#define MAGIC_OFFSET 8

#define MAX_DEATH 5
#define TYPING_STATISTICS "Typing statistics"

#define KEY_VALUE_RELEASE 0
#define KEY_VALUE_PRESS 1
#define KEY_VALUE_AUTOREPEAT 2

static struct ts_socket_t {
	xcb_connection_t *con;
	xcb_window_t wnd;
} ts_socket;

static xcb_window_t
search_window_by_title (
	xcb_connection_t *connection,
	xcb_window_t window,
	int death
) {
	if (death > MAX_DEATH || window == 0)
		return 0;

	xcb_get_property_cookie_t prop_c;
	xcb_get_property_reply_t *prop_r;

	prop_c = xcb_get_property(connection, 0, window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 0, 6);
	prop_r = xcb_get_property_reply(connection, prop_c, NULL);
	if (prop_r) {
		int len = xcb_get_property_value_length(prop_r);
		if (len != 0) {
			char *title = (char *)xcb_get_property_value(prop_r);
			if (is_string_starts_with(title, TYPING_STATISTICS)) {
				return window;
			}
		}
	}

	xcb_query_tree_cookie_t cookie;
	xcb_query_tree_reply_t *reply;

	cookie = xcb_query_tree(connection, window);
	if ((reply = xcb_query_tree_reply(connection, cookie, NULL))) {
		xcb_window_t *children = xcb_query_tree_children(reply);
		for (int i = 0; i < xcb_query_tree_children_length(reply); i++) {
			xcb_window_t rs = search_window_by_title(connection, children[i], death+1);
			if (rs != 0)
				return rs;
		}

		free(reply);
	}

	return 0;
}

static bool
handler (struct input_event *input) {
	xcb_key_press_event_t event;
	xcb_event_mask_t mask;

	event.time = input->time.tv_sec * 1000LL + input->time.tv_usec / 1000;
	event.detail = input->code + MAGIC_OFFSET;
	// event.event = ts_socket.wnd;
	// event.state = 0;

	switch (input->value) {
		case KEY_VALUE_PRESS:
			event.response_type = XCB_KEY_PRESS;
			mask = XCB_EVENT_MASK_KEY_PRESS;
			break;
		case KEY_VALUE_RELEASE:
			event.response_type = XCB_KEY_RELEASE;
			mask = XCB_EVENT_MASK_KEY_RELEASE;
			break;
		default:
			return true;
	}

	xcb_send_event(ts_socket.con, false, ts_socket.wnd, mask, (char *)&event);
	xcb_flush(ts_socket.con);

	return true;
}

int
main (int argc, char *argv[]) {
	ts_socket.con = xcb_connect(NULL, NULL);
	if (xcb_connection_has_error(ts_socket.con)) {
		printf("Connection to X-server failed\r\n");
		exit(1);
	}

	xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(ts_socket.con)).data;

	ts_socket.wnd = search_window_by_title(ts_socket.con, screen->root, 0);
	if (ts_socket.wnd == 0) {
		printf("Is Typing Statistics running?\r\n");
		exit(2);
	}

	if (!subscribe_kbrd_events(handler, NULL, 0))
		printf("Can't read input from keyboard device\r\n");
	else
		printf("Connection closed\r\n");

	xcb_disconnect(ts_socket.con);

	return 0;
}
