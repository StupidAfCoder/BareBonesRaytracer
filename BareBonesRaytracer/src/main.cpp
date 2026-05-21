#include<stdio.h>
#include"imgui.h"
#include"imgui_impl_sdl3.h"
#include"imgui_impl_sdlrenderer3.h"
#include "models.h"
#include<iostream>
#include<fstream>
#include<vector>
#include<cmath>
#include<algorithm>
#include<thread>
#include<atomic>
#include<SDL3/SDL.h>
#include<SDL3/SDL_main.h>
#include "stb_image_write.h"

template <typename T>
inline const T& clamp(const T& value, const T& min_val, const T& max_val)
{
	return std::min(std::max(value, min_val), max_val);
}

const int WIDTH = 1280;
const int HEIGHT = 720;
const float VIEWPORT_WIDTH = 4.0f;
const float VIEWPORT_HEIGHT = 3.0f;
const float Camera_Z = 1.0f;
const int framebuffer_size = WIDTH * HEIGHT * 3;
const int Recursion_Depth = 3;
const float PI = 3.14159f;

int selectedSphere = 0;
Point3D origin(3, 0, 0);
float cameraYaw = 0.0f;
float cameraPitch = 0.0f;

std::atomic<bool> a_renderInProgress(false);
std::atomic<bool> a_renderCanceled(false);
std::atomic<bool> a_needReRender(false);
std::vector<std::thread> threads;

uint8_t framebuffer[framebuffer_size];

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

void ComputeIntersection(float& closest_t, Sphere*& closest_sphere, Point3D origin, Vec3D endpoint, float t_min, float t_max, std::vector<Sphere>& scene)
{
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
}

Vec3D ReturnReflectedRay(Vec3D normal, Vec3D reflectedRay)
{
	return ((2 * normal * normal.dot(reflectedRay)) - reflectedRay);
}

float ComputeLighting(Point3D intersection, Vec3D normal, Vec3D viewVector , float specular , std::vector<Light>& lightScene , std::vector<Sphere>& scene)
{
	float intensity = 0.0;
	Vec3D lightVector;
	float t_max;

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
				t_max = 1;
			}
			else
			{
				//if the light is directional we already know it's direction
				lightVector = light.direction;
				t_max = std::numeric_limits<float>::infinity();
			}
			float shadow_t = std::numeric_limits<float>::infinity();
			Sphere* shadow_sphere = nullptr;

			ComputeIntersection(shadow_t, shadow_sphere, intersection, lightVector, 0.001f, t_max, scene);
			if (shadow_sphere != nullptr)
			{
				continue;
			}

			lightVector = lightVector.normalize();

			//To compute the diffusion reflection
			float N_dot_L = normal.dot(lightVector);

			if (N_dot_L > 0)
			{
				//The main computation for diffusion reflection
				intensity += light.intensity * N_dot_L;
			}

			//To compute the specular reflection
			if (specular != -1)
			{
				Vec3D reflectedRay = (2 * normal * N_dot_L) - lightVector;

				viewVector = viewVector.normalize();
				float R_dot_V = reflectedRay.dot(viewVector);
				if (R_dot_V > 0)
				{
					intensity += light.intensity * pow(R_dot_V , specular);
				}
			}
		}
	}

	return intensity;
}

Color TraceRay(Point3D origin, Vec3D endpoint, float t_min , float t_max, std::vector<Sphere>& scene , std::vector<Light>& lightScene , int recursion_depth)
{
	//We define that the currently the closest intersection with us at infinity 
	float closest_t = std::numeric_limits<float>::infinity();
	Sphere* closest_sphere = nullptr;
	ComputeIntersection(closest_t, closest_sphere, origin, endpoint, t_min, t_max, scene);
	if (closest_sphere == nullptr)
	{
		return Color(0.0f, 0.0f, 0.0f);
	}

	Point3D intersection = origin + (closest_t * endpoint);
	Vec3D normal = intersection - closest_sphere->center;
	//We then normalize our normal vector
	normal = normal.normalize();

	Color local_color =  closest_sphere->color * ComputeLighting(intersection , normal , endpoint.reverse(),  closest_sphere->specular , lightScene , scene);
	float r = closest_sphere->reflective;
	if (recursion_depth <= 0 || r <= 0)
	{
		return local_color;
	}

	Vec3D reflectedRay = ReturnReflectedRay(normal, endpoint.reverse());

	//This line has been added due to reflection acne (search the concept)
	Point3D reflected_origin = intersection + (normal * 0.01f);
	Color reflected_color = TraceRay(reflected_origin, reflectedRay, 0.01f, std::numeric_limits<float>::infinity(), scene, lightScene, recursion_depth - 1);

	return (local_color * (1 - r)) + (reflected_color * r);
}

void RenderRows(int StartY, int EndY, Point3D origin, std::vector<Sphere> scene, std::vector<Light> lightsScene , Mat3D rotation_camera)
{
	//The loop runs according the coordiante space designed by us it get's converted to coordinate that the ppm actually loads in by putPixel
	for (int y = StartY; y < EndY; y++)
	{
		for (int x = -WIDTH / 2; x < WIDTH / 2; x++)
		{
			if (a_renderCanceled) return;
			Vec3D endpoint = rotation_camera * CanvasToViewPort(x, y);
			endpoint = endpoint.normalize();
			Color color = TraceRay(origin, endpoint, 1, std::numeric_limits<float>::infinity(), scene, lightsScene, Recursion_Depth);
			PutPixelOnScreen(x, y, color);
		}
	}
}

void CancelandWait()
{
	a_renderCanceled = true;

	for (auto& t : threads)
	{
		if (t.joinable()) t.join();
	}
	threads.clear();
	a_renderCanceled = false;
}

void LaunchRender(Point3D origin, std::vector<Sphere> scene, std::vector<Light> lightsScene, Mat3D rotation_camera)
{
	CancelandWait();
	memset(framebuffer, 0, sizeof(framebuffer));
	
	int numOfThreads = std::thread::hardware_concurrency();

	int rowsPerThread = HEIGHT / numOfThreads;

	a_renderInProgress = true;
	
	for (int i = 0; i < numOfThreads; i++)
	{
		int startY = (-HEIGHT / 2) + i * rowsPerThread;
		int endY = (i == numOfThreads - 1) ? HEIGHT / 2 : startY + rowsPerThread;
		threads.emplace_back(std::thread(RenderRows, startY, endY, origin, scene, lightsScene, rotation_camera));
	}
}

Mat3D BuildCameraMatrix(float yawDeg, float pitchDeg)
{
	float yawRad = yawDeg * (PI / 180.0f);
	float pitchRad = pitchDeg * (PI / 180.0f);

	Mat3D yawMat(
		Vec3D(cosf(yawRad), 0.0f, sinf(yawRad)),
		Vec3D(0.0f, 1.0f, 0.0f),
		Vec3D(-sinf(yawRad), 0.0f, cosf(yawRad))
	);
	
	Mat3D pitchMat(
		Vec3D(1.0f , 0.0f , 0.0f),
		Vec3D(0.0f ,cosf(pitchRad) , -sinf(pitchRad)),
		Vec3D(0.0f , sinf(pitchRad) , cosf(pitchRad))
	);

	return yawMat * pitchMat;
}

bool LightControls(std::vector<Light>& lightsScene)
{
	bool changed = false;
	if (ImGui::CollapsingHeader("Lights"))
	{
		ImGui::Text("Point Light");
		changed |= ImGui::SliderFloat("PL X", &lightsScene[0].direction.x, -20.0f, 20.0f);
		changed |= ImGui::SliderFloat("PL Y", &lightsScene[0].direction.y, -20.0f, 20.0f);
		changed |= ImGui::SliderFloat("PL Z", &lightsScene[0].direction.z, -20.0f, 20.0f);
		changed |= ImGui::SliderFloat("PL Intensity", &lightsScene[0].intensity, 0.0f, 1.0f);

		ImGui::Separator();

		ImGui::Text("Directional Light");
		changed |= ImGui::SliderFloat("DL X", &lightsScene[1].direction.x, -20.0f, 20.0f);
		changed |= ImGui::SliderFloat("DL Y", &lightsScene[1].direction.y, -20.0f, 20.0f);
		changed |= ImGui::SliderFloat("DL Z", &lightsScene[1].direction.z, -20.0f, 20.0f);
		changed |= ImGui::SliderFloat("DL Intensity", &lightsScene[1].intensity, 0.0f, 1.0f);

		ImGui::Separator();

		ImGui::Text("Ambient Light");
		changed |= ImGui::SliderFloat("AL Intensity", &lightsScene[2].intensity, 0.0f, 1.0f);
	}
	return changed;
}

bool CameraControls()
{
	bool changed = false;
	if (ImGui::CollapsingHeader("Camera Controls"))
	{
		ImGui::Text("Camera Position");
		changed |= ImGui::SliderFloat("Camera X", &origin.x, -10.0f, 10.0f);
		changed |= ImGui::SliderFloat("Camera Y", &origin.y, -10.0f, 10.0f);
		changed |= ImGui::SliderFloat("Camera Z", &origin.z, -10.0f, 10.0f);
		
		ImGui::Separator();

		ImGui::Text("Camera Rotation");
		changed |= ImGui::SliderFloat("Camera Yaw", &cameraYaw, -180.0f, 180.0f);
		changed |= ImGui::SliderFloat("Camera Pitch", &cameraPitch, -89.0f, 89.0f);
	}
	return changed;
}

bool SphereControls(std::vector<Sphere>& scene)
{
	bool changed = false;
	if (ImGui::CollapsingHeader("Spheres"))
	{
		for (int i = 0; i < (int)scene.size(); i++)
		{
			if (i > 0) ImGui::SameLine();
			
			bool isSelected = (selectedSphere == i);
			if (isSelected)
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.9f, 1.0f));
			
			char label[16];
			snprintf(label, sizeof(label), "S %d", i + 1);

			if (ImGui::Button(label))
				selectedSphere = i;
			if (isSelected)
				ImGui::PopStyleColor();
		}

		ImGui::Separator();

		if (selectedSphere >= (int)scene.size())
			selectedSphere = (int)scene.size() - 1;

		Sphere& s = scene[selectedSphere];
		ImGui::PushID(selectedSphere);

		changed |= ImGui::SliderFloat("X", &s.center.x, -20.0f, 20.0f);
		changed |= ImGui::SliderFloat("Y", &s.center.y, -20.0f, 20.0f);
		changed |= ImGui::SliderFloat("Z", &s.center.z, 0.1f, 30.0f);
		changed |= ImGui::SliderFloat("Radius", &s.radius, 0.1f, 200.0f);
		changed |= ImGui::SliderFloat("Specular", &s.specular, 0.0f, 1000.0f);
		changed |= ImGui::SliderFloat("Reflective", &s.reflective, 0.0f, 1.0f);

		float col[3] = { s.color.r , s.color.g , s.color.b };
		if (ImGui::ColorEdit3("Color", col))
		{
			s.color.r = col[0];
			s.color.g = col[1];
			s.color.b = col[2];
			changed = true;
		}

		ImGui::PopID();

		ImGui::Separator();

		if (ImGui::Button(" + Add Sphere") && scene.size() < 20)
		{
			scene.emplace_back(
				Point3D(0.0f , 0.0f , 0.5f),
				1.0f,
				Color(1.0f , 1.0f , 1.0f),
				100.0f,
				0.0f
			);
			selectedSphere = (int)scene.size() - 1;
			changed = true;
		}

		ImGui::SameLine();

		if (ImGui::Button("- Remove Sphere") && scene.size() > 1)
		{
			scene.erase(scene.begin() + selectedSphere);
			if (selectedSphere >= (int)scene.size() - 1)
				selectedSphere = (int)scene.size() - 1;
			changed = true;
		}
	}
	return changed;
}

int main(int argc, char* argv[])
{
	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		std::cerr << "Error during initialization of SDL. Check dependencies " << SDL_GetError() << std::endl;
		return -1;
	}

	SDL_Window* window = SDL_CreateWindow("BareBonesRayTracer", WIDTH, HEIGHT, 0);
	SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
	SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
	ImGui_ImplSDLRenderer3_Init(renderer);

	memset(framebuffer, 0, sizeof(framebuffer));

	Mat3D rotation_camera = BuildCameraMatrix(cameraYaw, cameraPitch);
	
	const float RADIUS = 1.0f;
	const float RADIUS_BIG = 5000.0f;

	Point3D center1(0, -1, 3);
	Point3D center2(2,  0, 4);
	Point3D center3(-2, 0, 4);
	Point3D center4(0, -5001, 1);

	Color red(1.0f, 0.0f, 0.0f);
	Color blue(0.0f, 0.0f, 1.0f);
	Color green(0.0f, 1.0f, 0.0f);
	Color yellow(1.0f, 1.0f, 0.0f);

	Sphere s1(center1, RADIUS, red, 500.0f , 0.2f);
	Sphere s2(center2, RADIUS, green, 10.0f , 0.4f);
	Sphere s3(center3, RADIUS, blue, 500.0f, 0.3f);
	Sphere s4(center4, RADIUS_BIG, yellow, 1000.0f, 0.5f);

	Light l1(POINT, 0.6f, Vec3D(2.0f, 1.0f, 0.0f));
	Light l2(DIRECTIONAL, 0.2f, Vec3D(1.0f, 4.0f, 4.0f));
	Light l3(AMBIENT, 0.2f);
	
	std::vector<Sphere> scene = { s1 , s2 , s3 , s4};
	std::vector<Light> lightsScene = { l1, l2 , l3 };

	LaunchRender(origin, scene, lightsScene, rotation_camera);
	
	bool running = true;
	SDL_Event event;
	while (running)
	{

		while (SDL_PollEvent(&event))
		{
			ImGui_ImplSDL3_ProcessEvent(&event);
			if (event.type == SDL_EVENT_QUIT)
			{
				running = false;
			}

		}

		if (a_needReRender)
		{
			rotation_camera = BuildCameraMatrix(cameraYaw, cameraPitch);
			a_needReRender = false;
			LaunchRender(origin, scene, lightsScene, rotation_camera);
		}

		SDL_UpdateTexture(texture, NULL, framebuffer, WIDTH * 3);
		SDL_RenderClear(renderer);
		SDL_RenderTexture(renderer, texture, NULL, NULL);

		ImGui_ImplSDLRenderer3_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Controls");
		bool changed = false;

		changed |= LightControls(lightsScene);
		changed |= CameraControls();
		changed |= SphereControls(scene);

		if (changed)
			a_needReRender = true;

		ImGui::Separator();
		if (ImGui::Button("Save PNG")){
			const char* basepath = SDL_GetBasePath();
			if (basepath) {
				std::string fullpath = std::string(basepath) + "Output/output.png";
				SavePNG(fullpath.c_str());
			} else {
				SavePNG("output.png");
			}
		}
		if (ImGui::Button("Save PPM")){
			const char* basepath = SDL_GetBasePath();
			if (basepath) {
				std::string fullpath = std::string(basepath) + "Output/output.ppm";
				SavePPM(fullpath.c_str());
			} else {
				SavePPM("output.ppm");
			}
		}
		ImGui::Separator();

		ImGui::End();

		ImGui::Render();
		ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);

		SDL_RenderPresent(renderer);
		SDL_Delay(16);
	}

	CancelandWait();

	ImGui_ImplSDLRenderer3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();

	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}