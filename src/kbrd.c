#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "../vendor/vblibc/src/vblibc_string.h"
#include "kbrd.h"

#define NAME_DEVICE_LENGTH 256
#define PATH_DEVICE_LENGTH (NAME_DEVICE_LENGTH + 30)
#define PATH_DEVICE_LIST "/dev/input/by-path/"
#define KBRD_SIGNATURE "-event-kbd"
#define KBRD_MAX_DEVICES 10

typedef struct kbrd_device_t {
	int fd;
	char name[NAME_DEVICE_LENGTH];
	char path[PATH_DEVICE_LENGTH];
} kbrd_device_t;

static unsigned int
detect_kbrd_devices (kbrd_device_t *kbrds) {
	unsigned int count = 0;
	DIR *dir;
	struct dirent *f;

	if (NULL != (dir = opendir(PATH_DEVICE_LIST))) {
		while (NULL != (f = readdir(dir)) && count < KBRD_MAX_DEVICES) {
			if (false == vb_string_is_ends_with(f->d_name, KBRD_SIGNATURE))
				continue;

			kbrds[count].fd = -1;
			strncpy(kbrds[count].name, f->d_name, NAME_DEVICE_LENGTH);
			strncpy(kbrds[count].path, PATH_DEVICE_LIST, PATH_DEVICE_LENGTH);
			strncat(kbrds[count].path, f->d_name, NAME_DEVICE_LENGTH);
			count++;
		}

		closedir(dir);
	}

	return count;
}

bool
subscribe_kbrd_events (
	bool (*handler)(struct input_event *event),
	bool (*check)(),
	unsigned int step
) {
	kbrd_device_t kbrds[KBRD_MAX_DEVICES];
	unsigned int devices_count, i, count = 0;
	struct input_event event;
	bool listen = false;

	if (NULL == handler)
		return false;

	devices_count = detect_kbrd_devices(kbrds);
	if (devices_count < 1)
		return false;


	for (i=0; i<devices_count; i++) {
		if (kbrds[i].fd == -1)
			kbrds[i].fd = open(kbrds[i].path, O_RDONLY | O_NONBLOCK);
		if (kbrds[i].fd != -1)
			listen = true;
	}

	if (!listen)
		return false;

	while (listen) {
		for (i=0; i<devices_count; i++) {
			if (kbrds[i].fd == -1)
				continue;

			int rd = read(kbrds[i].fd, &event, sizeof(struct input_event));
			if (rd > 0 && event.type == EV_KEY) {
				if (!handler(&event))
					listen = false;
			}
		}

		if (NULL != check && step > 0 && count++ > step && listen)
			listen = check();
	}

	for (i=0; i<devices_count; i++) {
		if (kbrds[i].fd != -1) {
			close(kbrds[i].fd);
			kbrds[i].fd = -1;
		}
	}

	return true;
}
