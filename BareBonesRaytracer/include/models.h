#ifndef MODELS_H
#define MODELS_H
#include<cmath>

enum TYPEOFLIGHT
{
	POINT = 0,
	DIRECTIONAL = 1,
	AMBIENT = 2
};

struct Color
{
	float r, g, b;
	Color(float r, float g, float b) : r(r), g(g), b(b) {}

	Color operator*(float f) const
	{
		return { r * f , g * f , b * f };
	}

	Color operator+(const Color& color) const
	{
		return { r + color.r , g + color.g , b + color.b };
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
	explicit Vec3D(const Point3D& p) : x(p.x), y(p.y), z(p.z) {}

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

	Vec3D reverse() const
	{
		return Vec3D(-x, -y, -z);
	}
};

inline Vec3D operator*(float f, Vec3D v)
{
	return Vec3D(v.x * f, v.y * f, v.z * f);
}

inline Vec3D operator*(Vec3D v, float f)
{
	return Vec3D(v.x * f, v.y * f, v.z * f);
}

inline Point3D operator+(Point3D p, Vec3D v)
{
	return Point3D(p.x + v.x, p.y + v.y, p.z + v.z);
}

inline Vec3D operator-(Point3D p, Point3D other)
{
	return Vec3D(p.x - other.x, p.y - other.y, p.z - other.z);
}

inline Vec3D operator-(Vec3D v, Point3D p)
{
	return Vec3D(v.x - p.x, v.y - p.y, v.z - p.z);
}

struct Sphere
{
	Point3D center;
	float radius;
	Color color;
	float specular;
	float reflective;

	Sphere(Point3D center, float radius, Color color, float specular, float reflective) : center(center), radius(radius), color(color), specular(specular), reflective(reflective) {}
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

	Light(TYPEOFLIGHT type, float intensity, Vec3D direction) : type(type), intensity(intensity), direction(direction) {}
	Light(TYPEOFLIGHT type, float intensity) : type(type), intensity(intensity), direction(Vec3D()) {}
};

struct Mat3D
{
	Vec3D right; //X axis
	Vec3D up; //Y Axis
	Vec3D forward; //Z axis

	Mat3D(Vec3D right, Vec3D up, Vec3D forward) : right(right), up(up), forward(forward) {}

	Vec3D operator*(const Vec3D& v) const
	{
		return Vec3D(
			right.dot(v),
			up.dot(v),
			forward.dot(v)
		);
	}

	Mat3D operator*(const Mat3D& other) const
	{
		// each row of result = transform each basis vector of 'other' by 'this'
		return Mat3D(
			Vec3D(
				right.x * other.right.x + right.y * other.up.x + right.z * other.forward.x,
				right.x * other.right.y + right.y * other.up.y + right.z * other.forward.y,
				right.x * other.right.z + right.y * other.up.z + right.z * other.forward.z
			),
			Vec3D(
				up.x * other.right.x + up.y * other.up.x + up.z * other.forward.x,
				up.x * other.right.y + up.y * other.up.y + up.z * other.forward.y,
				up.x * other.right.z + up.y * other.up.z + up.z * other.forward.z
			),
			Vec3D(
				forward.x * other.right.x + forward.y * other.up.x + forward.z * other.forward.x,
				forward.x * other.right.y + forward.y * other.up.y + forward.z * other.forward.y,
				forward.x * other.right.z + forward.y * other.up.z + forward.z * other.forward.z
			)
		);
	}
};

#endif // !MODELS_H
