#include "FormulaManager.h"

#include <complex>

using namespace std;

map<string, function<void(double, double, double, ThreadData&, const StorageElement&, volatile bool*)>> FormulaManager::formulas;
map<string, function<bool(double, double, const StorageElement&)>> FormulaManager::diverges;

#define INDEX(x, y) ((xi = int(x)) + settings.width * (yi = int(y)))
#define INDEXCOMP(x, y) INDEX((x + halfCompWidth) * compScaleHori, (y + halfCompHeight) * compScaleVert) 

void FormulaManager::init()
{
#define FORMULA(f) formulas[#f] = [](double xc, double ystart, double ystep, ThreadData& data, const StorageElement& settings, volatile bool *stop){ \
	double halfCompWidth = settings.complexWidth / 2; \
	double halfCompHeight = settings.complexHeight / 2; \
	double compScaleHori = settings.width / settings.complexWidth; \
	double compScaleVert = settings.height / settings.complexHeight; \
	double thres = settings.divergenceThreshold * (double)settings.divergenceThreshold;\
	int xi, yi;\
	\
	for (double yc = ystart; yc + ystep/2 < halfCompHeight && !*stop; yc += ystep) \
	{ \
		complex<double> c(xc, yc), x; \
		\
		int startindex = INDEXCOMP(xc, yc); \
		\
		if (!settings.divergenceTable[startindex]) \
			continue; \
		\
		int kDiv = 0; \
		if(data.next + settings.steps >= data.cache.size()) \
			data.saveCallBack();\
		for (; kDiv < settings.steps; ++kDiv) \
		{ \
			f; \
			data.cache[kDiv+data.next] = make_tuple(x,c,kDiv); \
			if (x.imag() * x.imag() + x.real() * x.real() > thres) \
				break; \
		} \
		\
		if (kDiv != settings.steps) \
			data.next += kDiv;\
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
