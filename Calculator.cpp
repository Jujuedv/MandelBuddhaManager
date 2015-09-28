#include "Calculator.h"
#include <chrono>
#include <unistd.h>

using namespace std;
using namespace std::literals;

constexpr int THREADCOUNT = 7;
constexpr int MEMPERTHREAD = 128*1024*1024;
constexpr int CACHEPERTHREAD = MEMPERTHREAD / sizeof(ThreadData::cache[0]);

Calculator::Calculator(char *formula, int w, int h, int steps, int div, int skip, double cw, double ch, bool *ok, Storage *store)
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
		if (s->skipPoints != skip)
			continue;
		if (abs(s->complexHeight - ch) > 1e-9)
			continue;
		if (abs(s->complexWidth - cw) > 1e-9)
			continue;

		this->storageElem = s;
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
	s->skipPoints = skip;
	s->computedSteps = 0;
	s->complexWidth = cw;
	s->complexHeight = ch;
	s->headerSaved = false;

	store->save();

	createDivergencyTable(*s);

	store->save();
}

void Calculator::createDivergencyTable(StorageElement &s)
{
	s.aquireDivergenceTable();
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
	s.divDirty = true;
	s.releaseDivergenceTable();
	
	printf("done\n");
}

void Calculator::startCalculation()
{
	printf("starting calculation...\n");

	stop = false;
	xstep = storageElem->complexWidth*pow(0.5, storageElem->computedSteps+1);
	x = -storageElem->complexWidth/2 + xstep;
	ystep = storageElem->complexHeight*pow(0.5, storageElem->computedSteps+1);
	y = -storageElem->complexHeight/2 + ystep;
	stripe = 0;

	threadData.resize(THREADCOUNT);
	mergeDat.resize(storageElem->width * storageElem->height);
	calculating = THREADCOUNT;
	waiting = merging = 0;
	for(int i = 0; i < THREADCOUNT; ++i)
	{
		threadData[i].next = 0;
		threadData[i].cache.resize(CACHEPERTHREAD);
		threadData[i].saveCallBack = [this, i](){
			merge.lock();
			int next = 0; 
			double halfCompWidth = storageElem->complexWidth / 2;
			double halfCompHeight = storageElem->complexHeight / 2;
			double compScaleHori = storageElem->width / storageElem->complexWidth;
			double compScaleVert = storageElem->height / storageElem->complexHeight;
			while(next < threadData[i].next)
			{
				int kDiv = 0;
				while(kDiv + next < threadData[i].next && get<2>(threadData[i].cache[kDiv + next]) == kDiv)
					++kDiv;
				complex<double> c = get<1>(threadData[i].cache[next]);	
				int cx = (c.real() + halfCompWidth) * compScaleHori;
				int cy = (c.imag() + halfCompHeight) * compScaleVert;
				auto &cdat = mergeDat[cx + storageElem->width * cy];
				cdat.startHits ++;
				cdat.startSteps += kDiv;
				for(int j = storageElem->skipPoints; j < kDiv; ++j)
				{
					complex<double> x;
					int n;
					tie(x,ignore,n) = threadData[i].cache[next+j];
					int xx = (x.real() + halfCompWidth) * compScaleHori;
					int xy = (x.imag() + halfCompHeight) * compScaleVert;

					if(xx >= 0 && xy >= 0 && xx < storageElem->width && xy < storageElem->height)
					{
						auto &xdat = mergeDat[xx + storageElem->width * xy];
						xdat.hits++;
						xdat.realOrig += c.real();
						xdat.imagOrig += c.imag();
						xdat.steps += kDiv;
						xdat.reachedStep += j;
						if(j)
						{
							auto xlast = get<1>(threadData[i].cache[next+j-1]);
							xdat.realLast += xlast.real();
							xdat.imagLast += xlast.imag();
						}
					}
				}
				next += kDiv;
			}
			threadData[i].next = 0;

			merge.unlock();
		};
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
	storageElem->aquireData();
	storageElem->aquireDivergenceTable();
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
			if(sync.try_lock())
			{
				printf("\033]0;%d/%d stripes\007", stripe, (2 << storageElem->computedSteps)-1);
				fflush(stdout);
				sync.unlock();
			}

			calc.unlock();

			if(myX + xstep/2 > storageElem->complexWidth/2)
				break;

			if(stop)
			{
				sync.lock();
				storageElem->releaseDivergenceTable();
				storageElem->releaseData();
				sync.unlock();
				return;
			}

			form(myX, myY, myYstep, threadData[threadNum], *storageElem, &stop);
		}

		threadData[threadNum].saveCallBack();

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
				storageElem->releaseDivergenceTable();
				storageElem->releaseData();
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
			merge.lock();
			for(int i = 0; i < storageElem->width; ++i)
			{
				for(int j = 0; j < storageElem->height; ++j)
				{
					int index=i+j*storageElem->width;

					auto &d = storageElem->data[index];
					auto &td = mergeDat[index];

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

			printf("Saving Step %d... ", ++storageElem->computedSteps);
			storageElem->headerSaved = false;
			storageElem->dataDirty = true;
			store->save();
			printf("done\n");
			fflush(stdout);
		}
		sync.unlock();
	}
	sync.lock();
	storageElem->releaseDivergenceTable();
	storageElem->releaseData();
	sync.unlock();
}
