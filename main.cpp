#include <iostream>
#include <functional>
#include <algorithm>

#include <SDL2/SDL.h>

#include <libtecla.h>

#include "FormulaManager.h"
#include "Calculator.h"
#include "RenderManager.h"

using namespace std;

#define ISCMD(str, cmd) (str.substr(0, strlen(cmd)) == cmd)

GetLine *gl = 0;

#define AUTO_CPL_SELECT(name, prev, source, source2)\
		left = left.substr((prev).size());\
		left.erase(left.begin(), find_if(left.begin(), left.end(), not1(ptr_fun<int,int>(isspace))));\
		string name = left.substr(0, left.find_first_of(" \n\t"));\
		if(left == name)\
		{\
			for(auto x : (source))\
			{\
				if((source2).substr(0, name.size()) == name)\
				{\
					cpl_add_completion(cpl, line, l.size()-left.size(), word_end, (source2).substr(left.size()).c_str(), 0, 0);\
				}\
			}\
			return 0;\
		}


CPL_MATCH_FN(autocomp)
{
	Storage *store = (Storage*)data;

	string l(line, line + word_end);
	string cmd = l.substr(0, l.find_first_of(" \n\t"));
	if(l == cmd)
	{
		static vector<string> cmds = {"calc", "pause", "save", "select", "stop", "view"};
		for(auto c : cmds)
		{
			if(c.substr(0, cmd.size()) == cmd)
			{
				cpl_add_completion(cpl, line, 0, word_end, c.substr(l.size()).c_str(), 0, 0);
			}
		}
	}
	else if(cmd == "calc" || cmd == "select")
	{
		string left = l;
		AUTO_CPL_SELECT(formula, cmd, FormulaManager::formulas, x.first);

		vector<pair<bool, StorageElement*>> elems;
		for(auto e : store->saves)
			elems.push_back({e->formula == formula, e});

		set<string> possDims;
		for(auto e : elems)
			if(e.first)
				possDims.insert(to_string(e.second->width) + "x" + to_string(e.second->height));

		AUTO_CPL_SELECT(dims, formula, possDims, x);

		int w, h;
		sscanf(dims.c_str(), "%dx%d", &w, &h);
		for(auto &e : elems)
			if(!e.first || e.second->width != w || e.second->height != h)
				e.first = false;

		set<string> possSteps;
		for(auto e : elems)
			if(e.first)
				possSteps.insert(to_string(e.second->steps));

		AUTO_CPL_SELECT(steps, dims, possSteps, x);
		
		int s;
		sscanf(steps.c_str(), "%d", &s);
		for(auto &e : elems)
			if(!e.first || e.second->steps != s)
				e.first = false;

		set<string> possDiv;
		for(auto e : elems)
			if(e.first)
				possDiv.insert(to_string(e.second->divergenceThreshold));

		AUTO_CPL_SELECT(div, steps, possDiv, x);
		
		int d;
		sscanf(div.c_str(), "%d", &d);
		for(auto &e : elems)
			if(!e.first || e.second->divergenceThreshold != d)
				e.first = false;

		set<string> possSkip;
		for(auto e : elems)
			if(e.first)
				possSkip.insert(to_string(e.second->skipPoints));

		AUTO_CPL_SELECT(skip, div, possSkip, x);
		
		int sk;
		sscanf(skip.c_str(), "%d", &sk);
		for(auto &e : elems)
			if(!e.first || e.second->skipPoints != sk)
				e.first = false;

		set<string> possCw;
		for(auto e : elems)
			if(e.first)
				possCw.insert(to_string(e.second->complexWidth));

		AUTO_CPL_SELECT(cw, skip, possCw, x);
		
		double complW;
		sscanf(cw.c_str(), "%lf", &complW);
		for(auto &e : elems)
			if(!e.first || abs(e.second->complexWidth - complW) > 1e-9)
				e.first = false;
		
		set<string> possCh;
		for(auto e : elems)
			if(e.first)
				possCh.insert(to_string(e.second->complexHeight));

		AUTO_CPL_SELECT(ch, cw, possCh, x);
		
		double complH;
		sscanf(ch.c_str(), "%lf", &complH);
		for(auto &e : elems)
			if(!e.first || abs(e.second->complexHeight - complH) > 1e-9)
				e.first = false;
	}
	return 0;
}

int main(int argc, char** argv)
{
	SDL_Init(SDL_INIT_EVERYTHING);
	atexit(SDL_Quit);

	FormulaManager::init();

	string line;
	char *lineBuf;

	Storage store;
	store.load();

	RenderManager renderMan;

	gl = new_GetLine(1024, 1024*1024);
	gl_customize_completion(gl, (void*)&store, autocomp);
	
	printf("mbmanager initialized.\nUsage:\ncalc <formula> <w>x<h> <steps> <div> <skip> <cw> <ch>\n");

	Calculator* calc = nullptr;
	StorageElement* active = nullptr;

	while ((lineBuf = gl_get_line(gl, "", 0, -1)))
	{
		line = lineBuf;
		if (ISCMD(line, "calc"))
		{
			char formula[100] = "x=x*x+c";
			int w = 800, h = 600, steps = 1000, div = 50, skip = 0;
			double cw = 4, ch = 3;
			sscanf(line.c_str(), "calc %s %dx%d %d %d %d %lf %lf", formula, &w, &h, &steps, &div, &skip, &cw, &ch);

			if (calc) 
				fprintf(stderr, "already calculating something.. aborting..\n");
			else
			{
				printf("--> calc %s %dx%d %d %d %d %lf %lf\n", formula, w, h, steps, div, skip, cw, ch);

				bool ok = true;
				calc = new Calculator(formula, w, h, steps, div, skip, cw, ch, &ok, &store);
				if(!ok)
				{
					delete calc;
					calc = nullptr;
					continue;
				}
				calc->startCalculation();
				active = calc->storageElem;
			}
		}
		else if(ISCMD(line, "select"))
		{
			char formula[100] = "x=x*x+c";
			int w = 800, h = 600, steps = 1000, div = 50, skip = 0;
			double cw = 4, ch = 3;
			sscanf(line.c_str(), "select %s %dx%d %d %d %d %lf %lf", formula, &w, &h, &steps, &div, &skip, &cw, &ch);
			printf("--> select %s %dx%d %d %d %d %lf %lf\n", formula, w, h, steps, div, skip, cw, ch);

			bool found = false;
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
				if (s->skipPoints != skip)
					continue;
				if (abs(s->complexHeight - ch) > 1e-9)
					continue;
				if (abs(s->complexWidth - cw) > 1e-9)
					continue;

				active = s;
				found = true;
				break;
			}
			if(found)
				printf("selected.\n");
			else
				fprintf(stderr, "not found. create with 'calc'\n");
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
		else if(ISCMD(line, "pause"))
		{
			if(!calc)
				fprintf(stderr, "no calculation running.. pausing nothing..\n");
			else
			{
				printf("pausing... \n");
				calc->pauseCalculation();
				delete calc;
				calc = 0;
				printf("pausing... done.\n");
			}
		}
		else if(ISCMD(line, "view"))
		{
			char renderType[512] = "hits";
			sscanf(line.c_str(), "view %s", renderType);
			
			if(!active)
			{
				fprintf(stderr, "no active data set... aborting\nSelect one using 'select' or create one using 'calc'\n");
				continue;
			}
			renderMan.addWindow(new ViewWindow(active, renderType));
		}
		else if(ISCMD(line, "save"))
		{
			char renderType[512] = "hits", filename[512] = "out.png";
			sscanf(line.c_str(), "save %s %[^\n]", renderType, filename);
			
			if(!active)
			{
				fprintf(stderr, "no active data set... aborting\nSelect one using 'select' or create one using 'calc'\n");
				continue;
			}
			auto vw = new ViewWindow(active, renderType);
			vw->createToFile(filename);
			delete vw;
		}

		fflush(stdout);
	}

	gl = del_GetLine(gl);

	return 0;
}
