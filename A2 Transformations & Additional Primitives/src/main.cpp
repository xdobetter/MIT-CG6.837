#include "scene_parser.h"
#include "camera.h"
#include "object3d.h"
#include "image.h"
#include "light.h"
#include <cstring>
#include <iostream>

char* input_file = nullptr;
int width = 100;
int height = 100;
char* output_file = nullptr;
float depth_min = 0;
float depth_max = 1;
char* depth_file = nullptr;
char* normal_file = nullptr;
bool shade_back = false;

void argParser(int argc, char** argv);//解析输入的命令
void render(SceneParser& scene, Camera* camera, Object3D* group, Image& image, Image& normalImage, Image& depthImage, Vec3f& ambientColor);//渲染
int main(int argc,char **argv) {
	//sample command line:
	//-input scene2_01_diffuse.txt -size 200 200 -output output2_01.tga
	argParser(argc, argv);
	SceneParser scene(input_file);
	Camera* camera = scene.getCamera();//获得相机参数
	Object3D* group = scene.getGroup();//获得组参数
	Vec3f ambientColor = scene.getAmbientLight();//获得环境光参数
	
	Image image(width, height);
	image.SetAllPixels(scene.getBackgroundColor());//场景的背景色设置为图片的像素
	Image depthImage(width, height);
	depthImage.SetAllPixels(Vec3f(0.0, 0.0, 0.0));//背景色黑色
	Image normalImage(width, height);
	normalImage.SetAllPixels(Vec3f(0.0, 0.0, 0.0));//背景色黑色
	render(scene, camera, group, image, normalImage, depthImage, ambientColor);//渲染
	//文件存储判断
	if (output_file != nullptr) image.SaveTGA(output_file);
	if (depth_file != nullptr) depthImage.SaveTGA(depth_file);
	if (normal_file != nullptr) normalImage.SaveTGA(normal_file);
	return 0;
}

void render(SceneParser &scene,Camera *camera,Object3D *group,Image &image,Image &normalImage, Image &depthImage,Vec3f &ambientColor) {
	//渲染
	for (int i = 0; i < width; ++i) {
		for (int j = 0; j < height; ++j) {
			float x = float(i) / float(width);
			float y = float(j) / float_t(height);
			Ray ray = camera->generateRay(Vec2f(x, y));
			Hit hit(INFINITY, nullptr, Vec3f(0.0, 0.0, 0.0));
			bool flag = group->intersect(ray, hit, camera->getTMin());
			if (flag) {
				//shadow_back
				Vec3f normal = hit.getNormal();
				Vec3f rd = ray.getDirection();
				if (normal.Dot3(rd) > 0) {
					if (shade_back) {
						normal = -1 * normal;
					}
					else {
						//for scene2_07 black triangle
						image.SetPixel(i, j, Vec3f(0, 0, 0));
						continue;
					}
				}
				//shader
				Vec3f color(0.0, 0.0, 0.0);
				Vec3f diffuseColor = hit.getMaterial()->getDiffuseColor();
				Vec3f ambient = diffuseColor * ambientColor;
				color += ambient;
				for (int k = 0; k < scene.getNumLights(); ++k) {
					Light* light = scene.getLight(k);
					Vec3f pos = hit.getIntersectionPoint();
					Vec3f l, lightColor;//光向、光的颜色
					light->getIllumination(pos, l, lightColor);//获取光的方向（调用该方法是原光的相反方向）
					float d = l.Dot3(normal);
					if (d < 0) d = 0;
					Vec3f diffuse = d * diffuseColor * lightColor;
					color += diffuse;
				}
				image.SetPixel(i, j, color);//设置颜色

				//法向图
				normalImage.SetPixel(i, j, Vec3f(fabs(normal.x()), fabs(normal.y()), fabs(normal.z())));
				//深度图
				float t = hit.getT();
				if (t > depth_max) t = depth_max;
				if (t < depth_min) t = depth_min;
				t = (depth_max - t) / (depth_max - depth_min);
				depthImage.SetPixel(i, j, Vec3f(t, t, t));
			}

		}
	}
}

void argParser(int argc, char** argv) {
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-input")) {
			i++;
			assert(i < argc);
			input_file = argv[i];
		}
		else if (!strcmp(argv[i], "-size")) {
			i++;
			assert(i < argc);
			width = atoi(argv[i]);
			i++;
			assert(i < argc);
			height = atoi(argv[i]);
		}
		else if (!strcmp(argv[i], "-output")) {
			i++;
			assert(i < argc);
			output_file = argv[i];
		}
		else if (!strcmp(argv[i], "-depth")) {
			i++;
			assert(i < argc);
			depth_min = atof(argv[i]);
			i++;
			assert(i < argc);
			depth_max = atof(argv[i]);
			i++;
			assert(i < argc);
			depth_file = argv[i];
		}
		else if (!strcmp(argv[i], "-normals")) {
			i++;
			assert(i < argc);
			normal_file = argv[i];
		}
		else if (!strcmp(argv[i], "-shade_back")) {
			shade_back = true;
		}
		else {
			printf("whoops error with command line argument %d: '%s'\n", i, argv[i]);
			assert(0);
		}
	}
}