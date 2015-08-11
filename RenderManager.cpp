#include "RenderManager.h"

const double PI = acos(-1);

template<typename T>
T clamp(const T &val, const T &mi, const T &ma)
{
	return max(mi, min(ma, val));
} 

int createHSLColor(double h, double s, double l) { 
	double c = (1-abs(2*l-1))*s; 
	double hs = h * 6;
	double x = c * (1-abs(fmod(hs, 2)-1));

	double r = 0, g = 0, b = 0;
	switch((int)hs)
	{
		case 0: tie(r,g,b) = make_tuple(c,x,0); break;
		case 1: tie(r,g,b) = make_tuple(x,c,0); break;
		case 2: tie(r,g,b) = make_tuple(0,c,x); break;
		case 3: tie(r,g,b) = make_tuple(0,x,c); break;
		case 4: tie(r,g,b) = make_tuple(x,0,c); break;
		case 5: tie(r,g,b) = make_tuple(c,0,x); break;
	}

	double m = l - 0.5 * c;
	r = clamp(r+m, 0., 1.);
	g = clamp(g+m, 0., 1.);
	b = clamp(b+m, 0., 1.);

	return 0xFF000000 | ((int)(r * 255) << 16) | ((int)(g * 255) << 8) | ((int)(b * 255));
}

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
	if(type == "origin")
	{
		vector<double> rV, gV, bV;
		for(int i = 0; i < storage->width * storage->height; ++i)
		{
			rV.push_back(data[i].realOrig + storage->complexWidth * data[i].hits / 2);
			gV.push_back(data[i].imagOrig + storage->complexHeight * data[i].hits / 2);
			bV.push_back((-data[i].realOrig) + storage->complexWidth * data[i].hits / 2);
		}
		sort(rV.begin(), rV.end());
		sort(gV.begin(), gV.end());
		sort(bV.begin(), bV.end());
		for(int x = 0; x < storage->width; ++x)
		{
			for(int y = 0; y < storage->height; ++y)
			{
				auto d = data[x + y * storage->width];
				double r = distance(rV.begin(), lower_bound(rV.begin(), rV.end(), d.realOrig + storage->complexWidth * d.hits / 2)) / (double)rV.size();
				double g = distance(gV.begin(), lower_bound(gV.begin(), gV.end(), d.imagOrig + storage->complexHeight * d.hits / 2)) / (double)gV.size();
				double b = distance(bV.begin(), lower_bound(bV.begin(), bV.end(), (-d.realOrig) + storage->complexWidth * d.hits / 2)) / (double)bV.size();
				r = pow(r, 10);
				g = pow(g, 10);
				b = pow(b, 10);

				pixels[x + y * storage->width] = 0xFF000000 | ((int)(r * 255) << 16) | ((int)(g * 255) << 8) | ((int)(b * 255));
			}
		}

	}
	else if(type == "direction")
	{
		vector<uint64_t> vals;
		for(int i = 0; i < storage->width * storage->height; ++i)
			vals.push_back(data[i].hits);
		sort(vals.begin(), vals.end());
		for(int x = 0; x < storage->width; ++x)
		{
			for(int y = 0; y < storage->height; ++y)
			{
				int index = x + y * storage->width;
				auto d = data[index];
				uint64_t h = d.hits;
				int i = distance(vals.begin(), lower_bound(vals.begin(), vals.end(), h));
				double rel = i / (double)vals.size();
				rel = pow(rel, 10);

				complex<double> c(x*storage->complexWidth/storage->width-storage->complexWidth/2, y*storage->complexHeight/storage->height-storage->complexHeight/2);
				complex<double> orig(d.realOrig / h, d.imagOrig / h);
				complex<double> last(d.realLast / h, d.imagLast / h);

				complex<double> dir = c - orig;

				
				pixels[index] = createHSLColor((arg(dir)+PI)/(2*PI), 1, rel);
			}
		}
	}
	else
	{ //FALLBACK: hits
		uint64_t total = 0;
		vector<uint64_t> vals;
		for(int i = 0; i < storage->width * storage->height; ++i)
		{
			total += data[i].hits;
			vals.push_back(data[i].hits);
		}
		sort(vals.begin(), vals.end());
		for(int x = 0; x < storage->width; ++x)
		{
			for(int y = 0; y < storage->height; ++y)
			{
				uint64_t h = data[x + y * storage->width].hits;
				int i = distance(vals.begin(), lower_bound(vals.begin(), vals.end(), h));
				double rel = i / (double)vals.size();
				rel = pow(rel, 10);

				pixels[x + y * storage->width] = 0xFF000000 | ((int)(rel * 255) << 8);
			}
		}
	}

	SDL_UpdateTexture(texture, 0, pixels, storage->width * sizeof(Uint32));

	lastRendered = storage->computedSteps;
}

void ViewWindow::render()
{
	if(lastRendered != storage->computedSteps)
		renderPrepare();
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
