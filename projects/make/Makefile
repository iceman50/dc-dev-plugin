# This is a rudimentary Makefile that compiles files from the pluginsdk & src directories into a
# shared library. Adapt to your needs.

# Rename to your plugin's name.
TARGET = DevPlugin

CC = gcc
CXX = g++

CPPFLAGS += -Wall -Wextra -g -pipe -O3 -I../../
CXXFLAGS += -std=gnu++0x
LINKFLAGS += -static-libgcc -g -O3 -shared -Wl,--add-stdcall-alias
LINKXXFLAGS += -static-libstdc++

VPATH = ../../

OUTPUT_DIR = build

OBJS = \
	pluginsdk/Config.o \
	pluginsdk/Connections.o \
	pluginsdk/Core.o \
	pluginsdk/Hooks.o \
	pluginsdk/Hubs.o \
	pluginsdk/Logger.o \
	pluginsdk/Queue.o \
	pluginsdk/Tagger.o \
	pluginsdk/UI.o \
	pluginsdk/Util.o \
	src/GUI.o \
	src/main.o \
	src/Plugin.o \
	src/stdafx.o

COMPILER_SPEC = $(shell $(CC) -dumpmachine)

ifeq ($(findstring mingw, $(COMPILER_SPEC)),)
	BINEXT =
	LIBEXT = .so
else
	BINEXT = .exe
	CPPFLAGS += -D_WIN32_WINNT=0x502 -DWINVER=0x502 -D_WIN32_IE=0x600 \
				-DNOMINMAX -DSTRICT -DWIN32_LEAN_AND_MEAN \
				-DWIN32 -D_WIN32 -DUNICODE -D_UNICODE
	LIBEXT = .dll
	OBJS += src/resource.o
	OUTPUT_DIR := $(OUTPUT_DIR)-mingw
endif

ifeq ($(findstring x86_64, $(COMPILER_SPEC)),)
	CPPFLAGS += -march=i686
	OUTPUT_DIR := $(OUTPUT_DIR)-x86
else
	OUTPUT_DIR := $(OUTPUT_DIR)-x64
endif

ifeq ($(OS), Windows_NT)
	ifeq ($(findstring Cygwin, $(SHELL)),)
		MKDIR = if not exist $(subst /,\,$1) md $(subst /,\,$1)
		RMDIR = if exist $(subst /,\,$1) rd /s /q $(subst /,\,$1)
	else
		MKDIR = mkdir -p $1
		RMDIR = $(RM) -r $1
	endif
else
	MKDIR = mkdir -p $1
	RMDIR = $(RM) -r $1
endif

# boost
CPPFLAGS += -I../../boost
ifneq ($(findstring mingw, $(COMPILER_SPEC)),)
	CPPFLAGS += -DBOOST_ALL_NO_LIB -DBOOST_USE_WINDOWS_H
endif
OBJS += \
	boost/libs/atomic/src/lockpool.o \
	boost/libs/regex/src/c_regex_traits.o \
	boost/libs/regex/src/cpp_regex_traits.o \
	boost/libs/regex/src/cregex.o \
	boost/libs/regex/src/fileiter.o \
	boost/libs/regex/src/icu.o \
	boost/libs/regex/src/instances.o \
	boost/libs/regex/src/posix_api.o \
	boost/libs/regex/src/regex.o \
	boost/libs/regex/src/regex_debug.o \
	boost/libs/regex/src/regex_raw_buffer.o \
	boost/libs/regex/src/regex_traits_defaults.o \
	boost/libs/regex/src/static_mutex.o \
	boost/libs/regex/src/usinstances.o \
	boost/libs/regex/src/w32_regex_traits.o \
	boost/libs/regex/src/wc_regex_traits.o \
	boost/libs/regex/src/wide_posix_api.o \
	boost/libs/regex/src/winstances.o

# dwt
CPPFLAGS += -I../../dwt/include -DDWT_SHARED
ifeq ($(findstring mingw, $(COMPILER_SPEC)),)
$(error "dwt requires a compiler with Windows as target")
else
	LINKFLAGS += -lcomctl32 -lcomdlg32 -lgdi32 -lole32 -lshlwapi -luuid -luxtheme
endif
OBJS += dwt/src/Application.o dwt/src/Bitmap.o dwt/src/Brush.o dwt/src/CanvasClasses.o \
	dwt/src/Clipboard.o dwt/src/CommandLine.o dwt/src/Dispatcher.o dwt/src/DWTException.o \
	dwt/src/Events.o dwt/src/Font.o dwt/src/Icon.o dwt/src/ImageList.o dwt/src/LibraryLoader.o \
	dwt/src/Message.o dwt/src/Pen.o dwt/src/Point.o dwt/src/Rectangle.o dwt/src/Region.o \
	dwt/src/Taskbar.o dwt/src/Texts.o dwt/src/Theme.o dwt/src/Widget.o \
	dwt/src/util/DateTimeUtil.o dwt/src/util/GDI.o dwt/src/util/RegKey.o \
	dwt/src/util/StringUtils.o dwt/src/util/TimeSpan.o \
	dwt/src/util/win32/ApiHelpers.o dwt/src/util/win32/Version.o \
	dwt/src/widgets/Button.o dwt/src/widgets/CheckBox.o dwt/src/widgets/ColorDialog.o \
	dwt/src/widgets/ComboBox.o dwt/src/widgets/Control.o dwt/src/widgets/DateTime.o \
	dwt/src/widgets/FolderDialog.o dwt/src/widgets/FontDialog.o dwt/src/widgets/Grid.o \
	dwt/src/widgets/GroupBox.o dwt/src/widgets/Header.o dwt/src/widgets/Label.o \
	dwt/src/widgets/Link.o dwt/src/widgets/LoadDialog.o dwt/src/widgets/MDIChild.o \
	dwt/src/widgets/MDIFrame.o dwt/src/widgets/MDIParent.o dwt/src/widgets/Menu.o \
	dwt/src/widgets/MessageBox.o dwt/src/widgets/ModalDialog.o dwt/src/widgets/ModelessDialog.o \
	dwt/src/widgets/Notification.o dwt/src/widgets/ProgressBar.o dwt/src/widgets/RadioButton.o \
	dwt/src/widgets/Rebar.o dwt/src/widgets/RichTextBox.o dwt/src/widgets/SaveDialog.o \
	dwt/src/widgets/ScrolledContainer.o dwt/src/widgets/Slider.o dwt/src/widgets/Spinner.o \
	dwt/src/widgets/Splitter.o dwt/src/widgets/SplitterContainer.o dwt/src/widgets/StatusBar.o \
	dwt/src/widgets/Table.o dwt/src/widgets/TableTree.o dwt/src/widgets/TabView.o \
	dwt/src/widgets/TextBox.o dwt/src/widgets/ToolBar.o dwt/src/widgets/ToolTip.o \
	dwt/src/widgets/Tree.o dwt/src/widgets/VirtualTree.o dwt/src/widgets/Window.o

ifeq ($(findstring mingw, $(COMPILER_SPEC)),)
	PDB =
else
	PDB := $(OUTPUT_DIR)/$(TARGET).pdb
endif

TARGET := $(OUTPUT_DIR)/$(TARGET)$(LIBEXT)
OBJS := $(addprefix $(OUTPUT_DIR)/, $(OBJS))

PACKAGER = $(OUTPUT_DIR)/packaging/packager$(BINEXT)
PACK_INFO = ../../packaging/info_generated.xml

all: $(TARGET) $(PDB) $(PACK_INFO)

$(TARGET): $(OBJS)
	$(CXX) $^ $(LINKFLAGS) $(LINKXXFLAGS) $(OUTPUT_OPTION)

ifneq ($(findstring mingw, $(COMPILER_SPEC)),)
$(PDB): $(TARGET)
	strip --only-keep-debug $< $(OUTPUT_OPTION) && strip $<
endif

$(PACKAGER): $(OUTPUT_DIR)/packaging/packager/packager.o
	$(CXX) $^ $(subst -shared,,$(LINKFLAGS) $(LINKXXFLAGS)) $(OUTPUT_OPTION)

$(PACK_INFO): $(PACKAGER)
	$(PACKAGER) $(PACK_INFO)

$(OUTPUT_DIR)/%.o: %.c
	$(COMPILE.c) $< $(OUTPUT_OPTION)

$(OUTPUT_DIR)/%.o: %.cpp
	$(COMPILE.cpp) $< $(OUTPUT_OPTION)

$(OUTPUT_DIR)/%.o: %.rc
	windres $< $(OUTPUT_OPTION)

$(OBJS): | $(OUTPUT_DIR)

$(PACKAGER): | $(OUTPUT_DIR)

$(OUTPUT_DIR):
	$(call MKDIR,$(OUTPUT_DIR)/packaging/packager)
	$(call MKDIR,$(OUTPUT_DIR)/pluginsdk)
	$(call MKDIR,$(OUTPUT_DIR)/src)
	$(call MKDIR,$(OUTPUT_DIR)/boost/libs/atomic/src)
	$(call MKDIR,$(OUTPUT_DIR)/boost/libs/regex/src)
	$(call MKDIR,$(OUTPUT_DIR)/dwt/src/util/win32)
	$(call MKDIR,$(OUTPUT_DIR)/dwt/src/widgets)

clean:
	$(call RMDIR,$(OUTPUT_DIR))
