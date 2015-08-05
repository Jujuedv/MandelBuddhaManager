#pragma once

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
	mutex merging;
	volatile bool stop = false;

	Calculator(char *formula, int w, int h, int steps, int div, double cw, double ch, bool *ok, Storage *store);
	void createDivergencyTable(StorageElement &s);
	void startCalculation();
	void stopCalculation();
	void worker(int threadNum);
};
