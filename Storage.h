#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <complex>
#include <functional>

using namespace std;

struct PixelData
{
	uint64_t hits = 0;
	double realOrig = 0, imagOrig = 0;
	double realLast = 0, imagLast = 0;
	uint64_t steps = 0, reachedStep = 0;
	uint64_t startHits = 0, startSteps = 0;

	void load(FILE* file);
	void save(FILE* file);
};

struct ThreadData
{
	vector<tuple<complex<double>, complex<double>, int>> cache;
	volatile int next;
	function<void(void)> saveCallBack;
};

struct StorageElement
{
	int uid;

	string formula;
	int divergenceThreshold;
	int width, height;
	int steps, computedSteps, skipPoints;
	double complexWidth, complexHeight;

	bool loaded = false, saved = true, headerSaved = true;

	vector<char> divergenceTable;
	vector<PixelData> data;

	void loadHeader();
	void load();
	void saveHeader();
	void save();
};

struct Storage
{
	vector<StorageElement*> saves;
	int uidC;

	~Storage();

	void load();
	void save();
};

