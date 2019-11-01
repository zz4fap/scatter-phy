CXXFLAGS = -Wall -Wextra -g -fstack-protector-all -std=c++11 $(shell pkg-config --cflags protobuf \
) $(shell pkg-config --cflags libzmq)
LDFLAGS = $(shell pkg-config --libs libzmq) $(shell pkg-config --libs protobuf)

EXECUTABLES := CPPExample layer_template
#SOURCES    = $(basename $(shell find ./ -maxdepth 1 -name '*.cc' -or -name '*.cpp'))
SOURCES    = ./utils ./Message ./CommManager ./interf.pb


.PHONY: all
all: $(EXECUTABLES)

CPPExample: $(addsuffix .o,$(SOURCES)) CPPExample.o
	 $(CXX) $^ -o $@ $(LDFLAGS)

layer_template: $(addsuffix .o,$(SOURCES)) layer_template.o
	 $(CXX) $^ -o $@ $(LDFLAGS)
	 
%.d: %.$(EXTENSION)
	$(CXX)  -MM $< -o $@ $(CXXFLAGS)
	echo sed -i 's/:/ $@:/' $@
	printf '\t$$(CC)  -c $$< -o $$@\n' >>$@ $$(CXXFLAGS)

-include $(addsuffix .d,$(SOURCES))

.PHONY: clean
clean:
	find . -name '*.o' -delete
	find . -name '*.d' -delete
	find . -name '*~'  -delete
	rm $(EXECUTABLES)
	
