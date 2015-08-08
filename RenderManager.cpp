#include "RenderManager.h"

ViewWindow::ViewWindow(StorageElement*elem, string t)
{
	storage = elem;
	type = t;
}

ViewWindow::~ViewWindow()
{
	if(texture)
	{
		SDL_DestroyTexture(texture);
		texture = 0;
	}
	if(renderer)
	{
		SDL_DestroyRenderer(renderer);
		renderer = 0;
	}	
	if(window)
	{
		SDL_DestroyWindow(window);
		window = 0;
	}
	if(pixels)
	{
		delete[] pixels;
		pixels = 0;
	}
}

void ViewWindow::create()
{
	SDL_CreateWindowAndRenderer(storage->width, storage->height, SDL_WINDOW_SHOWN, &window, &renderer);

	char buffer[512];
	sprintf(buffer, "%s | %dx%d | s: %d | d: %d | cw: %lf | ch: %lf", storage->formula.c_str(), storage->width, storage->height, storage->steps, storage->divergenceThreshold, storage->complexWidth, storage->complexHeight);
	SDL_SetWindowTitle(window, buffer);

	pixels = new Uint32[storage->width * storage->height];
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, storage->width, storage->height);

	renderPrepare();
}

void ViewWindow::renderPrepare()
{
	vector<PixelData> data = storage->data;
	memset(pixels, 0, sizeof(Uint32) * storage->width * storage->height);

	//TODO add other modes
	{ //FALLBACK: hits
		uint64_t total = 0;
		vector<uint64_t> vals;
		for(int i = 0; i < storage->width * storage->height; ++i)
		{
			total += data[i].hits;
			vals.push_back(data[i].hits);
		}
		sort(vals.begin(), vals.end());
		printf("Total hits: %ld\n", total);
		for(int x = 0; x < storage->width; ++x)
		{
			for(int y = 0; y < storage->height; ++y)
			{
				uint64_t h = data[x + y * storage->width].hits;
				int i = distance(vals.begin(), lower_bound(vals.begin(), vals.end(), h));
				double rel = i / (double)vals.size();
				rel = pow(rel, 10);

				pixels[x + y * storage->width] = 0xFF000000 | ((int)(rel * 256) << 8);
			}
		}
	}

	SDL_UpdateTexture(texture, 0, pixels, storage->width * sizeof(Uint32));
}

void ViewWindow::render()
{
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, 0, 0);
	SDL_RenderPresent(renderer);
}

RenderManager::RenderManager()
{
	SDL_Init(SDL_INIT_EVERYTHING);
}

RenderManager::~RenderManager()
{
	SDL_Quit();
}

void RenderManager::addWindow(ViewWindow* w)
{
	winMutex.lock();
	if(!renderThreadAlive)
	{
		renderThreadAlive = true;
		thread(&RenderManager::renderThread, this).detach();
	}
	newWindows.insert(w);
	winMutex.unlock();
}

void RenderManager::removeWindow(ViewWindow* w)
{
	winMutex.lock();
	newWindows.erase(w);
	windows.erase(w);
	winMutex.unlock();
}

void RenderManager::renderThread()
{
	winMutex.lock();
	while(windows.size() || newWindows.size())
	{
		for(auto w : newWindows)
		{
			w->create();
			windows.insert(w);
		}
		newWindows.clear();
		winMutex.unlock();

		vector<ViewWindow*> rem;

		SDL_Event event;
		while(SDL_PollEvent(&event))
		{
			switch(event.type)
			{
			case SDL_WINDOWEVENT:
				for(auto w : windows)
				{
					if(SDL_GetWindowID(w->window) == event.window.windowID)
					{
						if(event.window.event == SDL_WINDOWEVENT_CLOSE)
							rem.push_back(w);
					}
				}
				break;
			}
		}
		for(auto w : rem)
		{
			removeWindow(w);
			delete w;
		}
		for(auto w : windows)
			w->render();

		SDL_Delay(10);

		winMutex.lock();
	}
	renderThreadAlive = false;
	winMutex.unlock();
}
