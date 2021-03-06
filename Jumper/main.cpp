#include "pch.h"
#include <iostream>
#include <SDL.h>

#define THREADING
//#define PIXELATED
#define LOG(x) std::cout << x << std::endl

struct Particle
{
	float x;
	float y;
	float vx;
	float vy;
	float lifetime = 0;
};

struct ThreadParticleChunk
{
	int startValue;
	int length;
};

const int particlesCount = 8192;
static Particle particles[particlesCount];

const int threadsCount = 4;
const int particlesPerThread = particlesCount / threadsCount;

static SDL_Thread* threads[threadsCount];

const float RESTITUTION = 0.5f;
const float FRICTION = 0.98f;
const float DRAG = 0.002f;
const float GRAVITY = 3;

static int screen_w;
static int screen_h;
static float dt;

SDL_Rect platforms[] =
{
	{50, 200, 100, 60},
	{200, 200, 100, 60},
	{200, 300, 100, 60},
	{330, 100, 20, 300}
};

static int TestThread(void* data)
{
	int ct;

	for (ct = 0; ct < 10; ct++)
	{
		printf("Thread count");
	}

	return (int)data;
}

void Increment(int* i)
{
	(*i) = 1;
}

void SimulateParticle(Particle& p, float dt)
{
	if (p.lifetime > 0)
	{
		p.x += p.vx * dt;
		p.y += p.vy * dt;
		p.vy += GRAVITY * dt;
		p.lifetime -= dt;

		// Collide with screen bounds
		if (p.y > screen_h)
		{
			p.vy = -p.vy * 0.5f;
			p.y = screen_h;
		}

		// Collide with platforms
		for (SDL_Rect platformRect : platforms)
		{
			int x = p.x;
			int y = p.y;
			int x2 = x + p.vx * dt;
			int y2 = y + p.vy * dt;

			if (SDL_IntersectRectAndLine(&platformRect, &x, &y, &x2, &y2))
			{
				// which side was hit?
				if (y == platformRect.y || y2 == platformRect.y) // top
				{
					p.y = platformRect.y;
					p.vy *= -RESTITUTION;
				}
				else if (y == platformRect.y + platformRect.h - 1
					|| y2 == platformRect.y + platformRect.h - 1) // bottom
				{
					p.y = platformRect.y + platformRect.h;
					p.vy *= -RESTITUTION;
				}
				else if (x == platformRect.x || x2 == platformRect.x) // left
				{
					p.x = platformRect.x;
					p.vx *= -RESTITUTION;
				}
				else if (x == platformRect.x + platformRect.w - 1
					|| x2 == platformRect.x + platformRect.w - 1) // right
				{
					p.x = platformRect.x + platformRect.w;
					p.vx *= -RESTITUTION;
				}
			}
		}
	}
}

static int ParticleThread(void* data)
{
	ThreadParticleChunk* tpc = (ThreadParticleChunk*)data;
	//((ThreadParticleChunk*)data)->length; // this works too?

	int l = tpc->length;
	int s = tpc->startValue;

	for (size_t i = s; i < l; i++)
	{
		//auto particle = &particles[i];

		SimulateParticle(particles[i], dt);

		//SimulateParticle(particle, dt);
	}

	return 0;
}


int main(int argc, char *argv[])
{
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
	{
		std::cout << "SDL could not initialize " << SDL_GetError() << std::endl;
	}

	SDL_DisplayMode dmode;
	SDL_GetCurrentDisplayMode(0, &dmode);

	const bool fullscreen = false;

	int res_w = fullscreen ? dmode.w : 640;
	int res_h = fullscreen ? dmode.h : 480;

#ifdef PIXELATED
	screen_w = res_w / 2;
	screen_h = res_h / 2;
#else
	screen_w = res_w;
	screen_h = res_h;
#endif

	SDL_Window *window = SDL_CreateWindow("FOOF FOOF FOOOOOF",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		res_w, res_h, 0);

	if (fullscreen)
		SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);

	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);

	//SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, 0);
#ifdef PIXELATED
	SDL_RenderSetLogicalSize(renderer, screen_w, screen_h);
#endif

	SDL_Event event;
	int quit = 0;

	const int FPS = 120;
	const int frameDelay = 1000 / FPS;
	dt = 1.0f / FPS;

	Uint32 frameStart;
	int frameTime;

	const int playerSizeX = 10;
	const int playerSizeY = 10;

	float playerX = 30;
	float playerY = 30;

	float velocityX = 0;
	float velocityY = 0;

	bool b_left = false;
	bool b_right = false;
	bool b_up = false;
	bool b_down = false;

	const int key_left = SDLK_a;
	const int key_right = SDLK_d;
	const int key_down = SDLK_s;
	const int key_up = SDLK_w;



	while (!quit)
	{
		frameStart = SDL_GetTicks();

		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDL_KEYDOWN:

				switch (event.key.keysym.sym)
				{
				case key_left: b_left = true; break;
				case key_right: b_right = true; break;
				case key_down: b_down = true; break;
				case key_up: b_up = true; break;
				}

				break;
			case SDL_KEYUP:
				switch (event.key.keysym.sym)
				{
				case key_left: b_left = false; break;
				case key_right: b_right = false; break;
				case key_down: b_down = false; break;
				case key_up: b_up = false; break;
				}
				break;

			case SDL_QUIT:
				quit = 1;
				break;

			default:
				break;
			}
		}

		float keyMult = 6 * dt;
		if (b_left) velocityX -= keyMult;
		if (b_right) velocityX += keyMult;
		if (b_down) velocityY += keyMult;
		if (b_up) velocityY -= keyMult;

		if (b_up)
		{
			// spawn particle
			int spawnPerFrame = 32;
			for (Particle &particle : particles)
			{
				if (particle.lifetime <= 0)
				{
					float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
					float r2 = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
					r2 = -1 + r2 * 2;

					particle.x = playerX + playerSizeX * 0.5f;
					particle.y = playerY + playerSizeY;
					particle.vx = velocityX + r2 * 50;
					particle.vy = velocityY + 400 * r;
					particle.lifetime = 1;
					spawnPerFrame--;

					if (spawnPerFrame < 0)
						break;
				}
			}
		}

#ifdef THREADING
		int threadReturnValue;

		for (size_t i = 0; i < threadsCount; i++)
		{
			ThreadParticleChunk chunk;
			chunk.startValue = i * particlesPerThread;
			chunk.length = particlesCount;
			threads[i] = SDL_CreateThread(ParticleThread, "Particle Thread", &chunk);
		}
#endif

		// Simulate player
		velocityX *= 1 - DRAG;
		velocityY *= 1 - DRAG;

		velocityY += GRAVITY * dt;

		playerX += velocityX;
		playerY += velocityY;

		SDL_Rect playerRect = { playerX, playerY, playerSizeX, playerSizeY };

		// Collide player with screen bounds
		if (playerY > screen_h - playerSizeY)
		{
			velocityY = -velocityY * 0.5f;
			playerY = screen_h - playerSizeY;
		}
		if (playerY < 0)
		{
			velocityY = 0;
			playerY = 0;
		}
		if (playerX > screen_w - playerSizeX)
		{
			velocityX = 0;
			playerX = screen_w - playerSizeX;
		}
		if (playerX < 0)
		{
			velocityX = 0;
			playerX = 0;
		}

		// Collide player with platforms
		for (SDL_Rect platformRect : platforms)
		{
			SDL_Rect result;
			SDL_bool b = SDL_IntersectRect(&platformRect, &playerRect, &result);

			if (b)
			{
				if (result.w < result.h)
				{
					// push away so it doesn't lodge:
					playerX -= velocityX * result.w;
					velocityX *= -1 * RESTITUTION;
					velocityY *= FRICTION;
				}
				else
				{
					playerY -= velocityY * result.h;
					velocityY *= -1 * RESTITUTION;
					velocityX *= FRICTION;
				}

				// debug intersection rect:
				//SDL_SetRenderDrawColor(renderer, 240, 240, 240, 188);
				//SDL_RenderFillRect(renderer, &result);
			}
		}

#ifdef THREADING
		// Wait for particle threads to finish
		for (SDL_Thread* thread : threads)
		{
			SDL_WaitThread(thread, &threadReturnValue);
		}
#else
		// simulate particles
		for (Particle &particle : particles)
			SimulateParticle(particle, dt);
#endif

		// DRAWING

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
		SDL_RenderClear(renderer);

		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		SDL_Rect rect = { playerX, playerY, playerSizeX, playerSizeY };
		SDL_RenderFillRect(renderer, &rect);

		// Draw particles
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		for (Particle& particle : particles)
		{
			if (particle.lifetime > 0)
			{
				SDL_RenderDrawLine(renderer,
					particle.x,
					particle.y,
					particle.x + particle.vx * dt * particle.lifetime * 8,
					particle.y + particle.vy * dt * particle.lifetime * 8);
			}
		}

		// Draw platforms
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		for (SDL_Rect platformRect : platforms)
		{
			SDL_RenderFillRect(renderer, &platformRect);
		}

		// End rendering
		SDL_RenderPresent(renderer);

		frameTime = SDL_GetTicks() - frameStart;

		if (frameDelay > frameTime)
		{
			SDL_Delay(frameDelay - frameTime);
		}
		}

	return EXIT_SUCCESS;
	}