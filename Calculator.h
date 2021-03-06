#ifndef _CALCULATOR_H_
#define _CALCULATOR_H_

#include <atomic>
#include <mutex>
#include <thread>

#include "Storage.h"
#include "FormulaManager.h"

using namespace std;

struct Calculator
{

	StorageElement *storageElem;
	Storage* store;

	vector<ThreadData> threadData;
	vector<thread> threads;
	vector<PixelData> mergeDat;
	mutex sync, merge, calc;
	volatile int calculating = 0, waiting = 0, merging = 0;
	volatile bool stop = false, abort = false;
	volatile double x, y, xstep, ystep;
	volatile uint64_t stripe;

	Calculator(char *formula, int w, int h, int steps, int div, int skip, double cw, double ch, bool *ok, Storage *store);
	void createDivergencyTable(StorageElement &s);
	void startCalculation();
	void stopCalculation();
	void pauseCalculation();
	void worker(int threadNum);
};

#endif
