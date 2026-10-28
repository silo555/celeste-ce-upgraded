# ----------------------------
# Makefile Options
# ----------------------------

NAME = CELESTE
ICON = icon.png
DESCRIPTION = "Celeste Classic"
COMPRESSED = YES
ARCHIVED = YES

CFLAGS = -Wall -Wextra -Oz -DNDEBUG
CXXFLAGS = -Wall -Wextra -Oz -DNDEBUG

# ----------------------------

include $(shell cedev-config --makefile)
