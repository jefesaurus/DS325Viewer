CC := g++ # This is the main compiler
# CC := clang --analyze # and comment out the linker last line for sanity

SRCDIR := src
INCLUDEDIR := include
BUILDDIR := build
TARGET := test
 
SRCEXT := cpp
SOURCES := $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))
OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.o))

ifeq ($(RICE),1)
CFLAGS := -std=c++11 -Wall -Ofast -msse -msse2 -msse3 -mmmx -m3dnow -march=native -funroll-all-loops -fforce-addr
else
#CFLAGS := -g -Wall -O3 -std=c++11 -msse -msse2 -msse3 -mmmx -m3dnow -march=native
CFLAGS := -g -Wall -std=c++11
endif

LIB := -lglut -lGL -lGLU -lGLEW -lm -lpthread
INC := -I ./dlib -I $(INCLUDEDIR) -I ./glm 

$(TARGET): $(OBJECTS)
	@echo " Linking..."
	@echo " $(CC) $^ -o $(TARGET) $(LIB)"; $(CC) $^ -o $(TARGET) $(LIB)

$(BUILDDIR)/%.o: $(SRCDIR)/%.$(SRCEXT)
	@mkdir -p $(BUILDDIR)
	@mkdir -p $(shell dirname $@)
	@echo " $(CC) $(CFLAGS) $(INC) -c -o $@ $<"; $(CC) $(CFLAGS) $(INC) -c -o $@ $<

clean:
	@echo " Cleaning..."; 
	@echo " $(RM) -r $(BUILDDIR) $(TARGET)"; $(RM) -r $(BUILDDIR) $(TARGET)

.PHONY: clean
