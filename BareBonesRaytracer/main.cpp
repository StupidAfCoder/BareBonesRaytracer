#include<stdio.h>
#include<iostream>
#include<fstream>
#include<vector>
#include<cmath>
#include<algorithm>
#include "stb_image_write.h"

template <typename T>
inline const T& clamp(const T& value, const T& min_val, const T& max_val)
{
	return std::min(std::max(value, min_val), max_val);
}

enum TYPEOFLIGHT {
	POINT = 0,
	DIRECTIONAL = 1,
	AMBIENT = 2
};

const int WIDTH = 800;
const int HEIGHT = 600;
const float VIEWPORT_WIDTH = 1.0f;
const float VIEWPORT_HEIGHT = 1.0f;
const float Camera_Z = 1.0f;
const int framebuffer_size = WIDTH * HEIGHT * 3;

uint8_t framebuffer[framebuffer_size];

struct Color
{
	float r, g, b;
	Color(float r, float g, float b) : r(r), g(g), b(b) {}
	
	Color operator*(float f)
	{
		return { r * f , g * f , b * f };
	}
};

struct Point3D
{
	float x, y, z;
	Point3D(float x, float y, float z) : x(x), y(y), z(z) {}

	Point3D operator+(Point3D other) const
	{
		return { x + other.x , y + other.y , z + other.z };
	}
	
};

inline Point3D operator*(float other, Point3D point)
{
	float x = other * point.x;
	float y = other * point.y;
	float z = other * point.z;
	return Point3D(x, y, z);
}

struct Vec3D
{
	float x, y, z;
	Vec3D(float x, float y, float z) : x(x), y(y), z(z) {}
	Vec3D() : x(0), y(0), z(0) {}
	explicit Vec3D(const Point3D& p) : x(p.x) , y(p.y) , z(p.z) {}

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

	float length() const
	{
		return sqrt((x * x) + (y * y) + (z * z));
	}

	Vec3D normalize() const
	{

		float len = length();

		if (len > 1e-8f)
		{
			float invLen = 1.0f / len;
			return Vec3D(x * invLen, y * invLen, z * invLen);
		}

		return Vec3D(0.0f, 0.0f, 0.0f);
	}
};

inline Vec3D operator*(float f, Vec3D v)
{
	return Vec3D(v.x * f, v.y * f, v.z * f);
}

inline Vec3D operator+(Point3D p, Vec3D v)
{
	return Vec3D(p.x + v.x, p.y + v.y, p.z + v.z);
}

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

struct Light
{
	TYPEOFLIGHT type;
	float intensity;
	Vec3D direction;

	Light(TYPEOFLIGHT type, float intensity, Vec3D direction) : type(type), intensity(intensity) , direction(direction) {}
	Light(TYPEOFLIGHT type, float intensity) : type(type) , intensity(intensity) , direction(Vec3D()) {}
};

//This function basically uses the reinhard tone map formula to clamp the values of the float from 0.0f to 1.0f (although not the best but it will work) 
Color reinhardToneMap(Color color)
{
	color.r = (color.r / (color.r + 1.0f));
	color.g = (color.g / (color.g + 1.0f));
	color.b = (color.b / (color.b + 1.0f));

	return color;
}

void PutPixelOnScreen(int x, int y, Color color)
{
	int screenX = WIDTH / 2 + x;
	int screenY = HEIGHT / 2 - y;

	if (screenX < 0 || screenX >= WIDTH) return;
	if (screenY < 0 || screenY >= HEIGHT) return;

	int index = (screenY * WIDTH + screenX) * 3;
	//Implementing other things so reinhard will be kept until I use it :)
	//color = reinhardToneMap(color);
	framebuffer[index + 0] = static_cast<uint8_t>(clamp(color.r * 255.0f, 0.0f, 255.0f));
	framebuffer[index + 1] = static_cast<uint8_t>(clamp(color.g * 255.0f, 0.0f, 255.0f));
	framebuffer[index + 2] = static_cast<uint8_t>(clamp(color.b * 255.0f, 0.0f, 255.0f));
}

Vec3D CanvasToViewPort(int X, int Y)
{
	Vec3D point(X * (VIEWPORT_WIDTH / WIDTH), Y * (VIEWPORT_HEIGHT / HEIGHT), Camera_Z);
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

IntersectData IntersectRaySphere(Point3D origin, Vec3D endpoint, Sphere sphere)
{
	//Remember to calculate the actual intersection we need to find if the ray was tangent to the sphere or actually passed inside the sphere. 
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

float ComputeLighting(Vec3D intersection, Vec3D normal, std::vector<Light>& lightScene)
{
	float intensity = 0.0;
	Vec3D lightVector;

	for (auto& light : lightScene)
	{
		if (light.type == AMBIENT)
		{
			intensity += light.intensity;
		}
		else
		{
			if (light.type == POINT)
			{
				//This code gets the direction of the vector that point light is coming from , naming convention currently is not so good :)
				lightVector = light.direction - intersection;
			}
			else
			{
				//if the light is directional we already know it's direction
				lightVector = light.direction;
			}
			//To compute the diffusion reflection
			float N_dot_L = normal.dot(lightVector);

			if (N_dot_L > 0)
			{
				//The main computation for diffusion reflection
				intensity += light.intensity * (N_dot_L / (normal.length() * lightVector.length()));
			}
		}
	}

	return intensity;
}

Color TraceRay(Point3D origin, Vec3D endpoint, float t_min , float t_max, std::vector<Sphere>& scene , std::vector<Light>& lightScene)
{
	//We define that the currently the closest intersection with us at infinity 
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
		return Color(1.0f, 1.0f, 1.0f);
	}

	Vec3D intersection = origin + (closest_t * endpoint);
	Vec3D normal = intersection - static_cast<Vec3D>(closest_sphere->center);
	//We then normalize our normal vector
	normal = normal.normalize();

	return closest_sphere->color * ComputeLighting(intersection , normal , lightScene);
}

int main()
{
	memset(framebuffer, 0, sizeof(framebuffer));
	
	const float RADIUS = 1.0f;
	const float RADIUS_BIG = 5000.0f;
	Point3D origin(0, 0, 0);

	Point3D center1(0, -1, 3);
	Point3D center2(2,  0, 4);
	Point3D center3(-2, 0, 4);
	Point3D center4(0, -5001, 1);

	Color red(1.0f, 0.0f, 0.0f);
	Color blue(0.0f, 0.0f, 1.0f);
	Color green(0.0f, 1.0f, 0.0f);
	Color yellow(1.0f, 1.0f, 0.0f);

	Sphere s1(center1, RADIUS, red);
	Sphere s2(center2, RADIUS, green);
	Sphere s3(center3, RADIUS, blue);
	Sphere s4(center4, RADIUS_BIG, yellow);

	Light l1(POINT, 0.6, Vec3D(2.0f, 1.0f, 0.0f));
	Light l2(DIRECTIONAL, 0.2, Vec3D(1.0f, 4.0f, 4.0f));
	Light l3(AMBIENT, 0.2);
	
	std::vector<Sphere> scene = { s1 , s2 , s3 , s4};
	std::vector<Light> lightsScene = { l1, l2 , l3 };

	//The loop runs according the coordiante space designed by us it get's converted to coordinate that the ppm actually loads in by putPixel
	for (int x = -WIDTH/2; x < WIDTH/2; x++)
	{
		for (int y = -HEIGHT/2; y < HEIGHT/2; y++)
		{
			Vec3D endpoint = CanvasToViewPort(x, y);
			Color color = TraceRay(origin, endpoint, 1, std::numeric_limits<float>::infinity(), scene, lightsScene);
			PutPixelOnScreen(x, y, color);
		}
	}

	SavePPM("trial.ppm");
	std::cout << "File has been saved!" << std::endl;
}