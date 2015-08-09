#include <iostream>

#include <SDL2/SDL.h>

#include <libtecla.h>

#include "FormulaManager.h"
#include "Calculator.h"
#include "RenderManager.h"

using namespace std;

#define ISCMD(str, cmd) (str.substr(0, strlen(cmd)) == cmd)

int main(int argc, char** argv)
{
	SDL_Init(SDL_INIT_EVERYTHING);
	atexit(SDL_Quit);

	FormulaManager::init();

	string line;

	Storage store;
	store.load();

	RenderManager renderMan;
	
	printf("mbmanager initialized.\nUsage:\ncalc <formula> <w>x<h> <steps> <div> <cw> <ch>\n");

	Calculator* calc = nullptr;

	while (getline(cin, line))
	{
		if (ISCMD(line, "calc"))
		{
			char formula[100] = "x=x*x+c";
			int w = 800, h = 600, steps = 1000, div = 50;
			double cw = 4, ch = 3;
			sscanf(line.c_str(), "calc %s %dx%d %d %d %lf %lf", formula, &w, &h, &steps, &div, &cw, &ch);

			if (calc) 
				fprintf(stderr, "already calculating something.. aborting..\n");
			else
			{
				printf("--> calc %s %dx%d %d %d %lf %lf\n", formula, w, h, steps, div, cw, ch);

				bool ok = true;
				calc = new Calculator(formula, w, h, steps, div, cw, ch, &ok, &store);
				if(!ok)
				{
					delete calc;
					calc = nullptr;
					continue;
				}
				calc->startCalculation();
			}
		}
		else if(ISCMD(line, "stop"))
		{
			if(!calc)
				fprintf(stderr, "no calculation running.. stopping nothing..\n");
			else
			{
				printf("stopping... \n");
				calc->stopCalculation();
				delete calc;
				calc = 0;
				printf("stopping... done.\n");
			}
		}
		else if(ISCMD(line, "view"))
		{
			char renderType[512] = "hits", renderSource[512] = "current";
			sscanf(line.c_str(), "view %s %s", renderType, renderSource);
			StorageElement *source = 0;
			
			if(renderSource == "current"s)
			{
				if(calc)
					source = calc->storageElem;
				else
				{
					fprintf(stderr, "No calculation is running... aborting\n");
					continue;
				}
			}
			else
			{
				char formula[100] = "x=x*x+c";
				int w = 800, h = 600, steps = 1000, div = 50;
				double cw = 4, ch = 3;
				sscanf(line.c_str(), "view %*s %s %dx%d %d %d %lf %lf", formula, &w, &h, &steps, &div, &cw, &ch);
				printf("--> view %s %s %dx%d %d %d %lf %lf\n", renderType, formula, w, h, steps, div, cw, ch);

				for (auto s : store.saves)
				{
					if (s->formula != formula)
						continue;
					if (s->width != w)
						continue;
					if (s->height != h)
						continue;
					if (s->steps != steps)
						continue;
					if (s->divergenceThreshold != div)
						continue;
					if (abs(s->complexHeight - ch) > 1e-9)
						continue;
					if (abs(s->complexWidth - cw) > 1e-9)
						continue;

					source = s;

					if (!s->loaded)
						s->load();
					break;
				}
			}

			if(!source)
			{
				fprintf(stderr, "no source found... aborting...\n");
				continue;
			}

			renderMan.addWindow(new ViewWindow(source, renderType));
		}
	}

	return 0;
}
