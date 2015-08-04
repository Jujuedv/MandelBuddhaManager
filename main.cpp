#include <iostream>

#include <SDL2/SDL.h>

#include "FormulaManager.h"
#include "Calculator.h"

using namespace std;

#define ISCMD(str, cmd) (str.substr(0, strlen(cmd)) == cmd)

int main(int argc, char** argv)
{
	SDL_Init(SDL_INIT_EVERYTHING);
	atexit(SDL_Quit);

	FormulaManager::init();

	string line;

	Storage store;
	printf("mbmanager initialized.\nUsage:\ncalc <formula> <w> <h> <steps> <div> <cw> <ch>\n");

	bool calculating = false;
	while (getline(cin, line))
	{
		if (ISCMD(line, "calc"))
		{
			char formula[100] = "x=x*x+c";
			int w = 800, h = 600, steps = 1000, div = 50;
			double cw = 4, ch = 3;
			sscanf(line.c_str(), "calc %s %dx%d %d %d %lf %lf", formula, &w, &h, &steps, &div, &cw, &ch);
			printf("--> calc %s %dx%d %d %d %lf %lf\n", formula, w, h, steps, div, cw, ch);

			if (calculating)
				fprintf(stderr, "already calculating something.. aborting..\n");
			else
				Calculator(formula, w, h, steps, div, cw, ch, &calculating, &store);
		}
	}

	return 0;
}
