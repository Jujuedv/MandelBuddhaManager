#include "Storage.h"
#include <sys/stat.h>

void PixelData::load(FILE *file)
{
	fscanf(file, "%lu %lf %lf %lf %lf %lu %lu %lu %lu\n", 
		&hits,
		&realOrig, 
		&imagOrig,
		&realLast, 
		&imagLast, 
		&steps, 
		&reachedStep, 
		&startHits,
		&startSteps);
}

void PixelData::save(FILE *file)
{
	fprintf(file, "%lu %lf %lf %lf %lf %lu %lu %lu %lu\n",
		hits,
		realOrig,
		imagOrig,
		realLast,
		imagLast,
		steps,
		reachedStep,
		startHits,
		startSteps);
}

void StorageElement::loadHeader()
{
	char filename[128];
	sprintf(filename, "storage/storage_%d.header", uid);

	auto file = fopen(filename, "r");

	char buffer[256];
	fscanf(file, "%s\n", buffer);
	formula=buffer;
	fscanf(file, "%d %d\n", &width, &height);
	fscanf(file, "%d %d\n", &steps, &divergenceThreshold);
	fscanf(file, "%lf %lf\n", &complexWidth, &complexHeight);
	fscanf(file, "%d\n", &computedSteps);
	fscanf(file, "%d\n", &skipPoints);

	fclose(file);
}

void StorageElement::load()
{
	char filename[128];
	sprintf(filename, "storage/storage_%d.data", uid);

	auto file = fopen(filename, "r");

	divergenceTable.resize(width * height);
	data.resize(width * height);

	for (int i = 0; i < width * height; ++i)
		fscanf(file, "%hhd\n", &(divergenceTable[i]));
	for (int i = 0; i < width * height; ++i)
		data[i].load(file);

	fclose(file);

	loaded = true;
}



void StorageElement::saveHeader()
{
	char filename[128];
	sprintf(filename, "storage/storage_%d.header", uid);

	auto file = fopen(filename, "w");

	fprintf(file, "%s\n", formula.c_str());
	fprintf(file, "%d %d\n", width, height);
	fprintf(file, "%d %d\n", steps, divergenceThreshold);
	fprintf(file, "%lf %lf\n", complexWidth, complexHeight);
	fprintf(file, "%d\n", computedSteps);
	fprintf(file, "%d\n", skipPoints);

	fclose(file);

	headerSaved = true;
}

void StorageElement::save()
{
	char filename[128];
	sprintf(filename, "storage/storage_%d.data", uid);

	auto file = fopen(filename, "w");

	for (int i = 0; i < width * height; ++i)
		fprintf(file, "%d ", divergenceTable[i]);
	fprintf(file, "\n");
	for (int i = 0; i < width * height; ++i)
		data[i].save(file);

	fclose(file);

	saved = true;
}

Storage::~Storage()
{
	save();
	for(auto s : saves)
		delete s;
	saves.clear();
}

void Storage::load()
{
	auto file = fopen("storage/storage.index", "r");
	if(!file)
	{
		uidC = 0;
		printf("no storage exists... will be created on calculations...\n");
		return;
	}
	fscanf(file, "%d\n", &uidC);

	int N;
	fscanf(file, "%d\n", &N);
	saves.resize(N);
	for (auto &s : saves)
	{
		s = new StorageElement();
		fscanf(file, "%d\n", &s->uid);
		s->loadHeader();
	}
	fclose(file);
}

void Storage::save()
{
	mkdir("storage", 0777);
	auto file = fopen("storage/storage.index", "w");
	fprintf(file, "%d\n", uidC);

	fprintf(file, "%d\n", (int)saves.size());
	for (auto &s : saves)
	{
		fprintf(file, "%d\n", s->uid);
		if (!s->headerSaved)
			s->saveHeader();
		if (!s->saved)
			s->save();
	}
	fclose(file);
}
