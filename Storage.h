#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <complex>
#include <functional>
#include <mutex>

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

	bool headerSaved = true;

	vector<uint8_t> divergenceTable;
	int divUsage = 0;
	bool divDirty = false;
	vector<PixelData> data;
	int dataUsage = 0;
	bool dataDirty = false;

	mutex mtx;

	void loadHeader();
	void loadDivergenceTable();
	void loadData();
	void loadPauseData(vector<PixelData> &dat, uint64_t &stripe);

	void saveHeader();
	void saveDivergenceTable();
	void saveData();
	void savePauseData(vector<PixelData> &dat, uint64_t stripe);

	void aquireDivergenceTable();
	void aquireData();

	void releaseDivergenceTable();
	void releaseData();

	void deletePauseData();
};

struct Storage
{
	vector<StorageElement*> saves;
	int uidC;

	~Storage();

	void load();
	void save();
};

