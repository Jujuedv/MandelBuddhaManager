#include "Storage.h"
#include <sys/stat.h>
#include <SDL2/SDL_endian.h>

template<typename T>
void write(FILE *file, const T &t);

template<>
void write(FILE *file, const uint64_t &t)
{
	uint64_t val = SDL_SwapBE64(t);
	fwrite(&val, sizeof(uint64_t), 1, file);
}

template<>
void write(FILE *file, const uint8_t &t)
{
	fwrite(&t, sizeof(uint8_t), 1, file);
}

template<>
void write(FILE *file, const double &t)
{
	write(file, *reinterpret_cast<const uint64_t *>(&t));
}

template<typename T>
void read(FILE *file, T &t);

template<>
void read(FILE *file, uint64_t &t)
{
	uint64_t val; 
	fread(&val, sizeof(uint64_t), 1, file);
	t = SDL_SwapBE64(val);
}

template<>
void read(FILE *file, uint8_t &t)
{
	fread(&t, sizeof(uint8_t), 1, file);
}

template<>
void read(FILE *file, double &t)
{
	read(file, *reinterpret_cast<uint64_t *>(&t));
}

void PixelData::load(FILE *file)
{
	read(file, hits);
	read(file, realOrig);
	read(file, imagOrig);
	read(file, realLast);
	read(file, imagLast);
	read(file, steps);
	read(file, reachedStep);
	read(file, startHits);
	read(file, startSteps);
}

void PixelData::save(FILE *file)
{
	write(file, hits);
	write(file, realOrig);
	write(file, imagOrig);
	write(file, realLast);
	write(file, imagLast);
	write(file, steps);
	write(file, reachedStep);
	write(file, startHits);
	write(file, startSteps);
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

	auto file = fopen(filename, "rb");

	divergenceTable.resize(width * height);
	data.resize(width * height);

	for (int i = 0; i < width * height; ++i)
		read(file, divergenceTable[i]);
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

	auto file = fopen(filename, "wb");

	for (int i = 0; i < width * height; ++i)
		write(file, divergenceTable[i]);
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
