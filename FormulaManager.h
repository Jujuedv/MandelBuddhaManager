#pragma once

#include <map>
#include <string>
#include <functional>

#include "Storage.h"

using namespace std;

struct FormulaManager
{
	static map<string, function<void(double, double, double, ThreadData&, const StorageElement&)>> formulas;
	static map<string, function<bool(double, double, const StorageElement&)>> diverges;
	static void init();
};
