#include<stdio.h>
#include<iostream>
#include<fstream>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

const int WIDTH = 800;
const int HEIGHT = 600;
const int framebuffer_size = WIDTH * HEIGHT * 3;

uint8_t framebuffer[framebuffer_size];

void PutPixelOnScreen(int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
	int screenX = WIDTH / 2 + x;
	int screenY = HEIGHT / 2 - y;

	if (screenX < 0 || screenX >= WIDTH) return;
	if (screenY < 0 || screenY >= HEIGHT) return;

	int index = (screenY * WIDTH + screenX) * 3;
	framebuffer[index + 0] = r;
	framebuffer[index + 1] = g;
	framebuffer[index + 2] = b;
}

void SavePPM(const char* filename)
{
	std::ofstream file(filename);

	file << "P3\n";
	file << WIDTH << " " << HEIGHT << "\n";
	file << "255\n";

	for (int i = 0; i < framebuffer_size; i += 3)
	{
		file << static_cast<int>(framebuffer[i + 0]) << " " << static_cast<int>(framebuffer[i + 1]) << " " << static_cast<int>(framebuffer[i + 2]) << "\n";
	}
}

void SavePNG(const char* filename)
{
	stbi_write_png(filename, WIDTH, HEIGHT, 3, framebuffer, WIDTH * 3);
}

int main()
{
	memset(framebuffer, 0, sizeof(framebuffer));
	
	const int RADIUS = 100;

	for (int x = -WIDTH/2; x < WIDTH/2; x++)
	{
		for (int y = -HEIGHT/2; y < HEIGHT/2; y++)
		{
			if (x * x + y * y <= RADIUS * RADIUS)
			{
				PutPixelOnScreen(x, y, 255, 0, 0);
			}
			else
			{
				PutPixelOnScreen(x, y, 0, 255, 0);
			}
		}
	}

	SavePPM("trial.ppm");
	std::cout << "File has been saved!" << std::endl;
}