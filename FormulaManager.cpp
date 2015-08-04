#include "FormulaManager.h"

#include <complex>

using namespace std;

map<string, function<void(double, double, double, ThreadData&, const StorageElement&)>> FormulaManager::formulas;
map<string, function<bool(double, double, const StorageElement&)>> FormulaManager::diverges;

#define INDEX(x, y) (int(x) + settings.width * int(y))
#define INDEXCOMP(x, y) INDEX((x + halfCompWidth) * compScaleHori, (y + halfCompHeight) * compScaleVert) 

void FormulaManager::init()
{
#define FORMULA(f) formulas[#f] = [](double xc, double ystart, double ystep, ThreadData& data, const StorageElement& settings){ \
	double halfCompWidth = settings.complexWidth / 2; \
	double halfCompHeight = settings.complexHeight / 2; \
	double compScaleHori = settings.width / settings.complexWidth; \
	double compScaleVert = settings.height / settings.complexHeight; \
	\
	for (double yc = ystart; yc < halfCompHeight; yc += ystep) \
	{ \
		complex<double> c(xc, yc), x; \
		\
		int startindex = INDEXCOMP(xc, yc); \
		\
		auto startData = data.data[startindex]; \
		startData.startHits++; \
		\
		if (!settings.divergenceTable[startindex]) \
		continue; \
		\
		int kDiv = 0; \
		for (; kDiv < settings.steps; ++kDiv) \
		{ \
			f; \
			data.cache[kDiv] = x; \
			if (abs(x) > settings.divergenceThreshold) \
			break; \
		} \
		\
		startData.startSteps += kDiv; \
		\
		if (kDiv != settings.steps) \
		{ \
			for (int k = 0; k < kDiv; ++k) \
			{ \
				x = data.cache[k]; \
				auto fieldIndex = INDEXCOMP(x.real(), x.imag()); \
				auto fieldData = data.data[fieldIndex]; \
				\
				fieldData.hits++; \
				fieldData.realOrig += xc; \
				fieldData.imagOrig += yc; \
				fieldData.steps += kDiv; \
				fieldData.reachedStep += k; \
			} \
		} \
	} \
};\
diverges[#f] = [](double x1, double x2, const StorageElement& settings){\
	complex<double> x, c(x1,x2);\
	for(int i = 0; i < 100; ++i)\
	{\
		f;\
		if(abs(f)>settings.divergenceThreshold)\
			return true;\
	}\
	return false;\
};
#include "Formulas.h"
#undef FORMULA
}
