CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I include -g
LEX      = flex
BISON    = bison

SRC_DIR  = src
OBJ_DIR  = obj

# Generated sources
LEX_OUT  = $(SRC_DIR)/lex.yy.c
PAR_OUT  = $(SRC_DIR)/parser.tab.c
PAR_HDR  = $(SRC_DIR)/parser.tab.h

# Core C++ sources (exclude generated files)
SRCS = $(SRC_DIR)/main.cpp \
       $(SRC_DIR)/semantic.cpp \
       $(SRC_DIR)/tac_gen.cpp \
       $(SRC_DIR)/optimizer.cpp

OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS)) \
       $(OBJ_DIR)/lex.yy.o \
       $(OBJ_DIR)/parser.tab.o

TARGET = minicc

# ---- Module targets -----
POSTFIX_BIN    = parser/postfix
PREFIX_BIN     = parser/prefix
INFIX_BIN      = parser/calc_infix
FF_BIN         = first_follow/ff

.PHONY: all clean demos ff llvm-ir test check

all: $(OBJ_DIR) $(TARGET)

# ---- Main compiler -------------------------------------------------------

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(PAR_OUT) $(PAR_HDR): $(SRC_DIR)/parser.y
	$(BISON) -d -o $(PAR_OUT) $<

$(LEX_OUT): $(SRC_DIR)/lexer.l $(PAR_HDR)
	$(LEX) -o $(LEX_OUT) $<

$(OBJ_DIR)/parser.tab.o: $(PAR_OUT)
	$(CXX) $(CXXFLAGS) -I src -c -o $@ $<

$(OBJ_DIR)/lex.yy.o: $(LEX_OUT)
	$(CXX) $(CXXFLAGS) -I src -Wno-unused-function -c -o $@ $<

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(PAR_HDR)
	$(CXX) $(CXXFLAGS) -I src -c -o $@ $<

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# ---- Module 2/3: calculator demos ----------------------------------------

demos: $(POSTFIX_BIN) $(PREFIX_BIN) $(INFIX_BIN)

$(POSTFIX_BIN): parser/postfix.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

$(PREFIX_BIN): parser/prefix.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

$(INFIX_BIN): parser/infix_lex.cpp parser/infix.y
	$(BISON) -d -o parser/infix.tab.c parser/infix.y
	$(CXX) $(CXXFLAGS) -I parser -o $@ parser/infix.tab.c parser/infix_lex.cpp -lm

# ---- Module 4: First/Follow / LL(1) table --------------------------------

ff: $(FF_BIN)

$(FF_BIN): first_follow/first_follow.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

# ---- Module 8: LLVM IR generation ----------------------------------------

llvm-ir:
	@command -v clang >/dev/null 2>&1 || { echo "clang not found. Install with: sudo apt-get install clang"; exit 1; }
	clang llvm/test1.c -S -emit-llvm -o llvm/test1.ll
	clang llvm/test1.c -S -emit-llvm -O3 -o llvm/test1_opt.ll
	clang llvm/test2.c -S -emit-llvm -o llvm/test2.ll
	clang llvm/test2.c -S -emit-llvm -O3 -o llvm/test2_opt.ll
	@echo "LLVM IR files written to llvm/"

# ---- Smoke test -----------------------------------------------------------

test: all
	./$(TARGET) tests/test_main.mc
	@echo "--- Unoptimised TAC ---"
	@cat output/test_main.tac
	@echo "--- Optimised TAC ---"
	@cat output/test_main.opt.tac

check: all
	@bash tests/run.sh

clean:
	rm -rf $(OBJ_DIR) $(TARGET) \
	       $(LEX_OUT) $(PAR_OUT) $(PAR_HDR) \
	       parser/infix.tab.c parser/infix.tab.h \
	       $(POSTFIX_BIN) $(PREFIX_BIN) $(INFIX_BIN) $(FF_BIN) \
	       output/*
