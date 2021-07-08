
SRC_DIR         ?= ./src
BUILD_DIR       ?= ./build

DOCS_CFG        ?= .doxygen

LEX_SRC         ?= $(SRC_DIR)/VCDScanner.l
LEX_OUT         ?= $(BUILD_DIR)/VCDScanner.cpp
LEX_HEADER      ?= $(BUILD_DIR)/VCDScanner.hpp
LEX_OBJ         ?= $(BUILD_DIR)/VCDScanner.o

YAC_SRC         ?= $(SRC_DIR)/VCDParser.ypp
YAC_OUT         ?= $(BUILD_DIR)/VCDParser.cpp
YAC_HEADER      ?= $(BUILD_DIR)/VCDParser.hpp
YAC_OBJ         ?= $(BUILD_DIR)/VCDParser.o

CXXFLAGS        += -I$(BUILD_DIR) -I$(SRC_DIR) -g
LD_FLAGS		?=

VCD_SRC         ?= $(SRC_DIR)/VCDFile.cpp \
                   $(SRC_DIR)/VCDValue.cpp \
                   $(SRC_DIR)/VCDFileParser.cpp

VCD_OBJ_FILES   = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(VCD_SRC)) $(YAC_OBJ) $(LEX_OBJ)

TEST_APP        ?= $(BUILD_DIR)/vcd-parse


all : vcd-parser $(BUILD_DIR)/libverilog-vcd-parser.a

debug: CXXFLAGS += -g
debug: all

wasm: CXX = emcc
wasm: CXXFLAGS += --bind
wasm: LD_FLAGS += -s WASM=1 -s EXPORTED_FUNCTIONS=["_free","_malloc","_VCDToJSON"] -s EXPORTED_RUNTIME_METHODS=["UTF8ToString","stringToUTF8","lengthBytesUTF8"]
wasm: vcd-parser  $(BUILD_DIR)/libvcdparse

docs: $(BUILD_DIR)/docs

vcd-parser: $(TEST_APP)

parser-srcs: $(YAC_OUT) $(LEX_OUT)

$(BUILD_DIR)/docs: $(DOCS_CFG) $(VCD_SRC) $(TEST_FILE)
	@mkdir -p $(@D)
	BUILD_DIR=$(BUILD_DIR) doxygen $(DOCS_CFG)

$(BUILD_DIR)/libverilog-vcd-parser.a: $(VCD_OBJ_FILES)
	@mkdir -p $(@D)
	ar rcs $@ $^
	cp $(SRC_DIR)/*.hpp $(BUILD_DIR)/

$(BUILD_DIR)/libvcdparse: $(VCD_OBJ_FILES)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(LD_FLAGS) -o $@ $^

$(BUILD_DIR)/%.o : $(SRC_DIR)/%.cpp $(LEX_OBJ) $(YAC_OBJ)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(YAC_OUT) : $(YAC_SRC)
	@mkdir -p $(@D)
	bison -v --defines=$(YAC_HEADER) $(YAC_SRC) -o $(YAC_OUT)

$(LEX_OUT) : $(LEX_SRC) $(YAC_OUT)
	@mkdir -p $(@D)
	flex  -P VCDParser --header-file=$(LEX_HEADER) -o $(LEX_OUT) $(LEX_SRC)

$(TEST_APP) : $(TEST_FILE) $(SRC_DIR)/VCDStandalone.cpp $(VCD_OBJ_FILES)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(LD_FLAGS) -o $@ $^

clean:
	rm -rf $(BUILD_DIR)