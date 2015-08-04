SRC=main.cpp Calculator.cpp Storage.cpp FormulaManager.cpp
HDR=Calculator.h Storage.h FormulaManager.h Formulas.h
BIN=mbmanager
OBJ=$(SRC:%cpp=%o)
#CXX=/usr/bin/clang++
CXXFLAGS=-std=c++11 -g
LDFLAGS=-lSDL2

all: .depend mbmanager

clean:
	rm -rf $(BIN) *.o .depend

.depend: $(SRC) $(HDR)
	$(CXX) -MM $(CXXFLAGS) $(SRC) > .depend

mbmanager: $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(BIN) $(LDFLAGS) $(OBJ)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

-include .depend
