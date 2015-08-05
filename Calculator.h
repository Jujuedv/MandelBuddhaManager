#pragma once

#include "Storage.h"
#include "FormulaManager.h"

struct Calculator
{
	StorageElement *storageElem;
	Storage* store;
	bool* calculating;

	Calculator(char *formula, int w, int h, int steps, int div, double cw, double ch, bool *calculating, Storage *store);
	void createDivergencyTable(StorageElement &s);
	void startCalculation();
};

