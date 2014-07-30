#
# Microblog protocol plug-in
#

all: build 

include ../global.mak

TARGETS = twitgin$(PLUGIN_SUFFIX)

LD = $(CC)

ifeq ($(strip $(IS_WIN32)), 1)
TWITGIN_INC_PATHS += -I$(GTK_TOP)/include/gtk-2.0 \
			-I$(GTK_TOP)/include/pango-1.0 \
			-I$(GTK_TOP)/include/atk-1.0 \
			-I$(GTK_TOP)/include/cairo \
			-I$(GTK_TOP)/lib/gtk-2.0/include \
			-I$(PIDGIN_TOP)/win32  \
			-I../microblog/


LIB_PATHS += -L$(GTK_TOP)/lib \
			-L$(PURPLE_TOP) \
			-L$(PIDGIN_TOP)
			
LIBS =	-lgtk-win32-2.0 \
			-lglib-2.0 \
			-lgdk-win32-2.0 \
			-lgobject-2.0 \
			-lintl \
			-lpurple \
			-lpidgin
CFLAGS := $(PURPLE_CFLAGS) $(TWITGIN_INC_PATHS)
else
CFLAGS := $(PURPLE_CFLAGS) $(PIDGIN_CFLAGS) -I../microblog/
LIB_PATHS = 
LIBS = $(PIDGIN_LIBS)
endif

TWITGIN_C_SRC = twitgin.c ../microblog/twitter.c ../microblog/tw_util.c ../microblog/mb_net.c ../microblog/mb_http.c ../microblog/mb_util.c ../microblog/mb_cache.c ../microblog/mb_oauth.c
TWITGIN_H_SRC = $(TWITGIN_C_SRC:%.c=%.h)
TWITGIN_OBJ = $(TWITGIN_C_SRC:%.c=%.o)

DISTFILES = twitgin.c twitpref.h Makefile

OBJECTS = $(TWITGIN_OBJ)

.PHONY: clean install build

build: $(TARGETS)

install: $(TARGETS)
	rm -f $(PURPLE_PLUGIN_DIR)/twitgin$(PLUGIN_SUFFIX)
	install -m 0755 -d $(PURPLE_PLUGIN_DIR)
	cp twitgin$(PLUGIN_SUFFIX) $(PURPLE_PLUGIN_DIR)/twitgin$(PLUGIN_SUFFIX)

uninstall: 
	rm -f $(PURPLE_PLUGIN_DIR)/twitgin$(PLUGIN_SUFFIX)

clean:
	rm -f $(TARGETS) $(OBJECTS)

twitgin$(PLUGIN_SUFFIX): $(TWITGIN_OBJ)
	$(LD) $(LDFLAGS) -shared $(TWITGIN_OBJ) $(LIB_PATHS) $(LIBS) $(DLL_LD_FLAGS) -o twitgin$(PLUGIN_SUFFIX)

