#include "Calculator.h"
#include <chrono>

using namespace std;
using namespace std::literals;

Calculator::Calculator(char *formula, int w, int h, int steps, int div, double cw, double ch, bool *ok, Storage *store)
{
	if(!FormulaManager::formulas.count(formula))
	{
		fprintf(stderr, "Formula '%s' not available.\nAdd it to Formulas.h and recompile.\n", formula);
		*ok = false;
		return;
	}

	this->store = store;
	this->storageElem = nullptr;

	for (auto s : store->saves)
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

		this->storageElem = s;

		if (!s->loaded)
			s->load();
		return;
	}

	store->saves.push_back(new StorageElement());
	StorageElement *s = store->saves.back();
	this->storageElem = s;

	s->formula = formula;
	s->uid = store->uidC++;
	s->divergenceThreshold = div;
	s->width = w;
	s->height = h;
	s->steps = steps;
	s->computedSteps = 0;
	s->complexWidth = cw;
	s->complexHeight = ch;
	s->loaded = true;
	s->saved = false;
	s->headerSaved = false;

	s->divergenceTable.resize(w*h);
	s->data.resize(w*h);

	createDivergencyTable(*s);

	store->save();
}

void Calculator::createDivergencyTable(StorageElement &s)
{
	printf("generating divergency table... ");
	fflush(stdout);
	auto diver = FormulaManager::diverges[s.formula];
	for(int x = 0; x < s.width; ++x)
	{
		for(int y = 0; y < s.height; ++y)
		{
			double x1 = x*s.complexWidth/s.width-s.complexWidth/2;
			double x2 = y*s.complexHeight/s.height-s.complexHeight/2;
			int val = diver(x1, x2, s)?1:0;
			for(int dx = max(-10, -x); x + dx < s.width && dx <= 10; ++dx)
				for(int dy = max(-10, -y); y + dy < s.height && dy <= 10; ++dy)
					s.divergenceTable[(x+dx)+(y+dy)*s.width] |= val;
		}
	}
	
	printf("done\n");
}

void Calculator::startCalculation()
{
	stop = false;
	xstep = storageElem->complexWidth*pow(0.5, storageElem->computedSteps+1);
	x = -storageElem->complexWidth/2 + xstep;
	ystep = storageElem->complexHeight*pow(0.5, storageElem->computedSteps+1);
	y = -storageElem->complexHeight/2 + ystep;
	stripe = 0;

	threadData.resize(4);
	calculating = 4;
	waiting = merging = 0;
	for(int i = 0; i < 4; ++i)
	{
		threadData[i].data.resize(storageElem->width * storageElem->height);
		threadData[i].cache.resize(storageElem->steps);
		threads.emplace_back(&Calculator::worker, this, i);
	}
}

void Calculator::stopCalculation()
{
	stop = true;
	for(auto &t : threads)
		t.join();
	threads.clear();
	threadData.clear();
}

void Calculator::worker(int threadNum)
{
	auto form = FormulaManager::formulas[storageElem->formula];
	while(!stop)
	{
		while(true)
		{
			calc.lock();
			double myX = x + xstep * stripe;
			double myY = y;
			double myYstep = stripe % 2 ? ystep * 2 : ystep;
			
			stripe++;

			calc.unlock();

			if(myX + xstep/2 > storageElem->complexWidth/2)
				break;

			if(stop)
				return;

			form(myX, myY, myYstep, threadData[threadNum], *storageElem);
		}

		sync.lock();
		calculating--;
		while(merging)
		{
			sync.unlock();
			this_thread::sleep_for(1ms);
			sync.lock();
		}
		waiting++;
		do 
		{
			sync.unlock();

			this_thread::sleep_for(1ms);

			sync.lock();
			if(stop && calculating)
			{
				calculating++;
				sync.unlock();
				return;
			}
		}
		while(calculating);
		waiting--;
		merging++;
		xstep = storageElem->complexWidth*pow(0.5, storageElem->computedSteps+2);
		x = -storageElem->complexWidth/2 + xstep;
		ystep = storageElem->complexHeight*pow(0.5, storageElem->computedSteps+2);
		y = -storageElem->complexHeight/2 + ystep;
		stripe = 0;
		sync.unlock();

		merge.lock();
		for(int i = 0; i < storageElem->width; ++i)
		{
			for(int j = 0; j < storageElem->height; ++j)
			{
				int index=i+j*storageElem->width;

				auto &d = storageElem->data[index];
				auto &td = threadData[threadNum].data[index];

				d.hits += td.hits; td.hits = 0;
				d.realOrig += td.realOrig; td.realOrig = 0;
				d.imagOrig += td.imagOrig; td.imagOrig = 0;
				d.realLast += td.realLast; td.realLast = 0;
				d.imagLast += td.imagLast; td.imagLast = 0;
				d.steps += td.steps; td.steps = 0;
				d.reachedStep += td.reachedStep; td.reachedStep = 0;
				d.startHits += td.startHits; td.startHits = 0;
				d.startSteps += td.startSteps; td.startSteps = 0;
			}
		}

		merge.unlock();

		sync.lock();
		while(waiting)
		{
			sync.unlock();
			this_thread::sleep_for(1ms);
			sync.lock();
		}
		merging--;
		calculating++;
		if(!merging)
		{
			printf("Saving Step %d... ", ++storageElem->computedSteps);
			storageElem->headerSaved = false;
			storageElem->saved = false;
			store->save();
			printf("done\n");
		}
		sync.unlock();
	}
}
