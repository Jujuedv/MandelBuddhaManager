#pragma once

#include <set>
#include <mutex>
#include <thread>
#include <algorithm>

#include <SDL2/SDL.h>

#include "Storage.h"

using namespace std;

struct ViewWindow
{
	StorageElement *storage = 0;
	SDL_Renderer *renderer = 0;
	SDL_Window *window = 0;
	Uint32 *pixels = 0;
	SDL_Texture *texture = 0;

	string type;
	bool update;
	int lastRendered;

	ViewWindow(StorageElement*, string, bool);
	~ViewWindow();

	void create();
	void renderPrepare();
	void render();
};

struct RenderManager
{
	volatile bool renderThreadAlive = false;
	set<ViewWindow*> windows, newWindows;
	mutex winMutex;
	RenderManager();
	~RenderManager();

	void addWindow(ViewWindow*);
	void removeWindow(ViewWindow*);	
	void renderThread();
};
