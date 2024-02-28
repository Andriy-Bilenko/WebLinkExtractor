######################################
# target
######################################
TARGET = projectA

######################################
# building variables
######################################
# debug build?
DEBUG = 1

#######################################
# paths
#######################################
# Build path
BUILD_DIR = build

######################################
# source
######################################
# C++ sources
CPP_SOURCES =  \
    main.cpp \
	src/parser.cpp \
	src/dataStructs.cpp \
	src/threadpool.cpp 

######################################
# compiler
######################################
CXX = g++

#######################################
# CXXFLAGS
#######################################
# C++ includes
PYTHON_LIB = -I /usr/include/python3.11 -lpython3.11
SOCI_LIB = -lsoci_core -lsoci_postgresql

CXX_INCLUDES = -Iinc $(PYTHON_LIB) $(SOCI_LIB)

LDFLAGS = $(PYTHON_LIB) $(SOCI_LIB) -lpq -lssl -lcrypto -pthread -lcurl

# compile CXX flags
CXXFLAGS = $(CXX_INCLUDES) -Wall -Werror -pedantic -std=c++20

ifeq ($(DEBUG), 1)
CXXFLAGS += -g -gdwarf-2
endif

# Generate dependency information
CXXFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"

#######################################
# build the application
#######################################
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(CPP_SOURCES:.cpp=.o)))
vpath %.cpp $(sort $(dir $(CPP_SOURCES)))

$(BUILD_DIR)/%.o: %.cpp Makefile | $(BUILD_DIR)
	$(CXX) -c $(CXXFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET): $(OBJECTS) Makefile
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

# create build directory
$(BUILD_DIR):
	mkdir $@

#######################################
# clean up
#######################################
clean:
	-rm -fR $(BUILD_DIR)

#######################################
# run the application
#######################################
run: $(BUILD_DIR)/$(TARGET)
	./$(BUILD_DIR)/$(TARGET) $(ARGS)

#######################################
# dependencies
#######################################
-include $(wildcard $(BUILD_DIR)/*.d)
