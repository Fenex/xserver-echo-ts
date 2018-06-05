#ifndef _KL_KBRD_H
#define _KL_KBRD_H

#include <linux/input.h>

#define true 1
#define false 0
#define bool int

bool
subscribe_kbrd_events (
	bool (*handler)(struct input_event *event),
	bool (*check)(),
	unsigned int step
);

#endif /* _KL_KBRD_H */
