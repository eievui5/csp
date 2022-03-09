CC := g++
BIN := bin/csp
OBJS := $(patsubst src/%.cpp, obj/%.o, $(shell find src/ -name '*.cpp'))

CFLAGS := -Isrc/include -Isrc/ -std=c++17 -Wall -Wimplicit-fallthrough -Wno-unused-result -Wno-parentheses -MD
RELEASEFLAGS := -Os -s -flto
DEBUGFLAGS := -Og -g

CFLAGS += $(DEBUGFLAGS)

all:
	$(MAKE) $(BIN)

clean:
	rm -rf bin/ obj/

rebuild:
	$(MAKE) clean
	$(MAKE) all

test: all
	./$(BIN) -i test/test.csp -o bin/test.html -d obj/

memcheck: all
	valgrind --leak-check=full ./$(BIN) $(TESTFLAGS)

# Compile each source file.
obj/%.o: src/%.cpp
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

# Link the output binary.
$(BIN): $(OBJS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ $^
