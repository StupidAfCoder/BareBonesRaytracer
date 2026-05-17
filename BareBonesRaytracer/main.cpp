#include<stdio.h>
#include<iostream>
#include<fstream>
#include<vector>
#include "stb_image_write.h"

const int WIDTH = 800;
const int HEIGHT = 600;
const float VIEWPORT_WIDTH = 1.0f;
const float VIEWPORT_HEIGHT = 1.0f;
const float DISTANCE_FOV = 1.0f;
const int framebuffer_size = WIDTH * HEIGHT * 3;

uint8_t framebuffer[framebuffer_size];

struct Color
{
	uint8_t r, g, b;
	Color(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b) {}
};

struct Point3D
{
	float x, y, z;
	Point3D(float x, float y, float z) : x(x), y(y), z(z) {}
};

struct Vec3D
{
	float x, y, z;
	Vec3D(float x, float y, float z) : x(x), y(y), z(z) {}

	Vec3D operator-(const Vec3D& other) const
	{
		return { x - other.x , y - other.y , z - other.z };
	}

	Vec3D operator+(const Vec3D& other) const
	{
		return { x + other.x , y + other.y , z + other.z };
	}

	float dot(const Vec3D& other) const
	{
		return x * other.x + y * other.y + z * other.z;
	}
};

struct Sphere
{
	Point3D center;
	float radius;
	Color color;

	Sphere(Point3D center, float radius, Color color) : center(center), radius(radius), color(color) {}
}; 

struct IntersectData
{
	float t1, t2;
	IntersectData(float t1, float t2) : t1(t1), t2(t2) {}
};

void PutPixelOnScreen(int x, int y, Color color)
{
	int screenX = WIDTH / 2 + x;
	int screenY = HEIGHT / 2 - y;

	if (screenX < 0 || screenX >= WIDTH) return;
	if (screenY < 0 || screenY >= HEIGHT) return;

	int index = (screenY * WIDTH + screenX) * 3;
	framebuffer[index + 0] = color.r;
	framebuffer[index + 1] = color.g;
	framebuffer[index + 2] = color.b;
}

Point3D CanvasToViewPort(int X, int Y)
{
	Point3D point(X * (VIEWPORT_WIDTH / WIDTH), Y * (VIEWPORT_HEIGHT / HEIGHT), DISTANCE_FOV);
	return point;
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

IntersectData IntersectRaySphere(Point3D origin, Point3D endpoint, Sphere sphere)
{
	float radius = sphere.radius;
	Vec3D originToCenter = Vec3D(origin.x, origin.y, origin.z) - Vec3D(sphere.center.x, sphere.center.y, sphere.center.z);

	Vec3D Endpoint(endpoint.x, endpoint.y, endpoint.z);
	Vec3D Origin(origin.x, origin.y, origin.z);
	//These variables are named this way due to the utilization of the quadractic formula (remember for future reference)
	float a = Endpoint.dot(Endpoint);
	float b = 2 * (originToCenter.dot(Endpoint));
	float c = originToCenter.dot(originToCenter) - radius*radius;

	float discriminant = b * b - 4 * a * c;
	if (discriminant < 0)
	{
		return IntersectData(std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity());
	}

	float t1 = (-b + sqrt(discriminant)) / (2 * a);
	float t2 = (-b - sqrt(discriminant)) / (2 * a);

	return IntersectData(t1, t2);
}

Color TraceRay(Point3D origin, Point3D endpoint, float t_min , float t_max, std::vector<Sphere> scene)
{
	float closest_t = std::numeric_limits<float>::infinity();
	Sphere* closest_sphere = nullptr;
	for (auto& sphere : scene)
	{
		IntersectData data = IntersectRaySphere(origin, endpoint, sphere);
		if (t_min <= data.t1 && data.t1 <= t_max && data.t1 < closest_t)
		{
			closest_t = data.t1;
			closest_sphere = &sphere;
		}
		if (t_min <= data.t2 && data.t2 <= t_max && data.t2 < closest_t)
		{
			closest_t = data.t2;
			closest_sphere = &sphere;
		}
	}
	if (closest_sphere == nullptr)
	{
		return Color(255, 255, 255);
	}

	return closest_sphere->color;
}

int main()
{
	memset(framebuffer, 0, sizeof(framebuffer));
	
	const float RADIUS = 1.0f;
	Point3D origin(0, 0, 0);

	Point3D center1(0, 1, 3);
	Point3D center2(1,2,4);
	Point3D center3(-1, 0, 4);

	Color red(255, 0, 0);
	Color blue(0, 0, 255);
	Color green(0, 255, 0);

	Sphere s1(center1, RADIUS, red);
	Sphere s2(center2, RADIUS, green);
	Sphere s3(center3, RADIUS, blue);
	
	std::vector<Sphere> scene = { s1 , s2 , s3 };
	for (int x = -WIDTH/2; x < WIDTH/2; x++)
	{
		for (int y = -HEIGHT/2; y < HEIGHT/2; y++)
		{
			Point3D endpoint = CanvasToViewPort(x, y);
			Color color = TraceRay(origin, endpoint, 1, std::numeric_limits<float>::infinity(), scene);
			PutPixelOnScreen(x, y, color);
		}
	}

	SavePPM("trial.ppm");
	std::cout << "File has been saved!" << std::endl;
}