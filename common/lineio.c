#include "lineio.h"

#include <string.h>

size_t strip_line_ending(char *line) {
	size_t len = strlen(line);

	while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
		line[--len] = '\0';

	return len;
}
