#ifndef _RENDERMANAGER_H_
#define _RENDERMANAGER_H_

#include <set>
#include <mutex>
#include <thread>
#include <algorithm>
#include <tuple>

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
	int lastRendered;

	ViewWindow(StorageElement*, string);
	~ViewWindow();

	void create();
	void createToFile(string filename);
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

#endif
