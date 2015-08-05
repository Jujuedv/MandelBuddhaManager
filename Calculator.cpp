#include "Calculator.h"


Calculator::Calculator(char *formula, int w, int h, int steps, int div, double cw, double ch, bool *calculating, Storage *store)
{
	if(!FormulaManager::formulas.count(formula))
	{
		fprintf(stderr, "Formula '%s' not available.\nAdd it to Formulas.h and recompile.\n", formula);
		return;
	}

	this->store = store;
	this->storageElem = nullptr;
	this->calculating = calculating;

	for (auto &s : store->saves)
	{
		if (s.formula != formula)
			continue;
		if (s.width != w)
			continue;
		if (s.height != h)
			continue;
		if (s.steps != steps)
			continue;
		if (s.divergenceThreshold != div)
			continue;
		if (abs(s.complexHeight - ch) > 1e-9)
			continue;
		if (abs(s.complexWidth - cw) > 1e-9)
			continue;

		this->storageElem = &s;
		if (!s.loaded)
		{
			s.load();
		}
		startCalculation();
		return;
	}
	StorageElement s;
	s.formula = formula;
	s.uid = store->uidC++;
	s.divergenceThreshold = div;
	s.width = w;
	s.height = h;
	s.steps = steps;
	s.computedSteps = 0;
	s.complexWidth = cw;
	s.complexHeight = ch;
	s.loaded = true;
	s.saved	= false;
	s.headerSaved = false;

	s.divergenceTable.resize(w*h);
	s.data.resize(w*h);

	createDivergencyTable(s);

	store->saves.push_back(s);
	store->save();

	startCalculation();
}

void Calculator::createDivergencyTable(StorageElement &s)
{
	auto diver = FormulaManager::diverges[s.formula];
	for(int x = 0; x < s.width; ++x)
	{
		for(int y = 0; y < s.height; ++y)
		{
			double x1 = x*s.complexWidth/s.width-s.complexWidth/2;
			double x2 = y*s.complexHeight/s.height-s.complexHeight/2;
			s.divergenceTable[x+y*s.width] = diver(x1, x2, s)?1:0;
		}
	}
	printf("divergencyTable generated\n");
}

void Calculator::startCalculation()
{

}
