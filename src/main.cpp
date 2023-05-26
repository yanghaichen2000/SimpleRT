#include "renderer.h"
#include <thread>
#include <algorithm>
#include <ctime>
#include "bvh_node.h"

#pragma warning(disable : 4996)


using std::vector;
using std::fstream;
using std::thread;
using std::cout;


// 特性列表
// 1.  图像纹理						完成
// 2.  法线贴图（预计算切线）			完成	
// 3.  ggx透明材质
// 4.  环境光照
// 5.  meshtriangle类				完成
// 6.  基础obj加载					完成
// 7.  景深
// 8.  bloom
// 9.  ggx多次弹射补偿
// 10. vec3替换为eigen::Vector3d
// 11. bssrdf
// 12. 透明材质						完成
// 13. 体积
// 14. alpha						完成（正确性需要确认）
// 15. 降噪
// 16. ggx非金属材质					完成
// 17. 混合材质
// 18. 置换贴图						完成
// 19. 圆锥							完成
// 20. 圆柱							完成

// 置换贴图法线重新计算


#define do_render

int main()
{
#ifdef do_render
	// 初始化
	const double aspect_ratio = 16.0 / 16.0; // 16.0 / 9.0
	const int image_width = 1280; //800
	const int image_height = static_cast<int>(image_width / aspect_ratio);
	const int samples_per_pixel = 2;
	const int max_depth = 4;

	// 设置object
	hittable_list world;

	// 简单material
	shared_ptr<material> light_low = make_shared<phong_material>(vec3(1.0, 1.0, 1.0), vec3(3, 3, 3));
	shared_ptr<material> light_super = make_shared<phong_material>(vec3(1.0, 1.0, 1.0), vec3(10, 10, 10));
	shared_ptr<material> light_false = make_shared<phong_material>(vec3(0, 0, 0), vec3(1, 1, 1));
	shared_ptr<material> red = make_shared<phong_material>(vec3(0.8, 0, 0), vec3(0, 0, 0));
	shared_ptr<material> blue = make_shared<phong_material>(vec3(0, 0, 1.0), vec3(0, 0, 0));
	shared_ptr<material> green = make_shared<phong_material>(vec3(0, 0.8, 0), vec3(0, 0, 0));
	shared_ptr<material> phong_2 = make_shared<phong_material>(vec3(1.0, 1.0, 1.0), vec3(0, 0, 0));
	shared_ptr<material> white = make_shared<phong_material>(vec3(1, 1, 1), vec3(0, 0, 0));
	shared_ptr<material> stone = make_shared<ggx_metal_material>(0.8, vec3(0.04, 0.04, 0.04));
	shared_ptr<material> al = make_shared<ggx_metal_material>(0.02, vec3(0.9, 0.9, 0.9));
	shared_ptr<material> au = make_shared<ggx_metal_material>(0.1, vec3(1.0, 0.782, 0.344));
	shared_ptr<material> ag = make_shared<ggx_metal_material>(0.1, vec3(0.972, 0.960, 0.915));

	// 场景顶点
	double tmp_value = 1.0; // 16.0 / 9.0
	vec3 vertex_1(-2 * tmp_value, -2, -8);
	vec3 vertex_2(2 * tmp_value, -2, -8);
	vec3 vertex_3(2 * tmp_value, -2, -4);
	vec3 vertex_4(-2 * tmp_value, -2, -4);
	vec3 vertex_5(-2 * tmp_value, 2, -8);
	vec3 vertex_6(2 * tmp_value, 2, -8);
	vec3 vertex_7(2 * tmp_value, 2, -4);
	vec3 vertex_8(-2 * tmp_value, 2, -4);
	//vec3 light_vertex_1 = vec3(-1, 1.999, -6.25);
	//vec3 light_vertex_2 = vec3(1, 1.999, -6.25);
	//vec3 light_vertex_3 = vec3(1, 1.999, -5.75);
	//vec3 light_vertex_4 = vec3(-1, 1.999, -5.75);
	vec3 light_vertex_1 = vec3(-0.5, 1.999, -6.5);
	vec3 light_vertex_2 = vec3(0.5, 1.999, -6.5);
	vec3 light_vertex_3 = vec3(0.5, 1.999, -5.5);
	vec3 light_vertex_4 = vec3(-0.5, 1.999, -5.5);
	vec3 light_vertex_5 = vec3(1.9999, -0.5, -7.8);
	vec3 light_vertex_6 = vec3(1.9999, -0.5, -7.4);
	vec3 light_vertex_7 = vec3(1.9999, -0.9, -7.4);
	vec3 light_vertex_8 = vec3(1.9999, -0.9, -7.8);

	// 三角形实例
	auto box_material = white;
	shared_ptr<texture> tex_wall_left = make_shared<simple_color_texture>(0, 150, 0);
	shared_ptr<material> mat_wall_left = make_shared<ggx_nonmetal_material>(0.15, 0.03, tex_wall_left);

	shared_ptr<texture> tex_wall_right = make_shared<simple_color_texture>(150, 0, 0);
	shared_ptr<material> mat_wall_right = make_shared<ggx_nonmetal_material>(0.15, 0.03, tex_wall_right);
	
	shared_ptr<texture> tex_wall = make_shared<color_map>("obj/wall_d.jpg", 1);
	shared_ptr<texture> tex_wall_n = make_shared<normal_map>("obj/wall_n.jpg", 1);
	shared_ptr<material> mat_wall = make_shared<ggx_nonmetal_material>(0.05, 0.03, tex_wall, vec3(0, 0, 0), tex_wall_n);

	shared_ptr<texture> tex_floor = make_shared<color_map>("obj/floor_d.jpg", 1);
	shared_ptr<texture> tex_floor_n = make_shared<normal_map>("obj/floor_n.jpg", 1);
	shared_ptr<material> mat_floor = make_shared<ggx_nonmetal_material>(0.15, 0.03, tex_floor, vec3(0, 0, 0), tex_floor_n);

	shared_ptr<texture> tex_ceiling = make_shared<simple_color_texture>(250, 250, 250);
	shared_ptr<texture> tex_ceiling_n = make_shared<normal_map>("obj/ceiling_n.jpg", 1);
	shared_ptr<material> mat_ceiling = make_shared<phong_material>(2, tex_ceiling, vec3(0, 0, 0), tex_ceiling_n);

	triangle tri1(vertex_3, vertex_2, vertex_1, mat_floor, vec3(2, 0, 0), vec3(2, 1, 0), vec3(0, 1, 0), vec3(0, 1, 0), vec3(0, 1, 0), vec3(0, 1, 0));
	triangle tri2(vertex_1, vertex_4, vertex_3, mat_floor, vec3(0, 1, 0), vec3(0, 0, 0), vec3(2, 0, 0), vec3(0, 1, 0), vec3(0, 1, 0), vec3(0, 1, 0));
	triangle tri3(vertex_5, vertex_2, vertex_6, mat_wall, vec3(0, 1, 0), vec3(2, 0, 0), vec3(2, 1, 0), vec3(0, 0, 1), vec3(0, 0, 1), vec3(0, 0, 1));
	triangle tri4(vertex_1, vertex_2, vertex_5, mat_wall, vec3(0, 0, 0), vec3(2, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1), vec3(0, 0, 1), vec3(0, 0, 1));
	triangle tri5(vertex_1, vertex_8, vertex_4, mat_wall, vec3(2, 0, 0), vec3(0, 1, 0), vec3(0, 0, 0), vec3(1, 0, 0), vec3(1, 0, 0), vec3(1, 0, 0)); // green
	triangle tri6(vertex_1, vertex_5, vertex_8, mat_wall, vec3(2, 0, 0), vec3(2, 1, 0), vec3(0, 1, 0), vec3(1, 0, 0), vec3(1, 0, 0), vec3(1, 0, 0)); // green
	triangle tri7(vertex_2, vertex_3, vertex_7, mat_wall, vec3(0, 0, 0), vec3(2, 0, 0), vec3(2, 1, 0), vec3(-1, 0, 0), vec3(-1, 0, 0), vec3(-1, 0, 0)); // red 
	triangle tri8(vertex_2, vertex_6, vertex_7, mat_wall, vec3(0, 0, 0), vec3(0, 1, 0), vec3(2, 1, 0), vec3(-1, 0, 0), vec3(-1, 0, 0), vec3(-1, 0, 0)); // red
	triangle tri9(vertex_5, vertex_6, vertex_8, white);
	triangle tri10(vertex_6, vertex_7, vertex_8, white);
	triangle tri11(vertex_8, vertex_7, vertex_4, white);
	triangle tri12(vertex_7, vertex_3, vertex_4, white);
	triangle tri13(light_vertex_1, light_vertex_2, light_vertex_3, light_false);
	triangle tri14(light_vertex_1, light_vertex_3, light_vertex_4, light_false);
	triangle tri15(light_vertex_7, light_vertex_6, light_vertex_5, light_false);
	triangle tri16(light_vertex_8, light_vertex_7, light_vertex_5, light_false);


	// 设置world

	// sphere
	shared_ptr<medium> med_glass = make_shared<medium>(1.5);
	shared_ptr<texture> tex_sphere = make_shared<simple_color_texture>(vec3(1, 1, 0.1));
	//shared_ptr<material> mat_sphere = make_shared<ggx_translucent_material>(0.02, 0.08, tex_sphere, vec3(0, 0, 0), nullptr, nullptr, med_glass);
	//shared_ptr<material> mat_sphere = make_shared<ggx_translucent_material>(0.02, 0.08, tex_sphere, vec3(0, 0, 0), nullptr, nullptr, nullptr);
	shared_ptr<material> mat_sphere = make_shared<translucent_material>(tex_sphere, vec3(0, 0, 0), nullptr, nullptr, med_glass);
	//world.add(make_shared<sphere>(point3(-0.6, -1, -5.6), 1, mat_sphere));

	//shared_ptr<texture> tex_sss = make_shared<simple_color_texture>(0, 180, 0);
	//shared_ptr<material> mat_sss = make_shared<sss_material>(0.058, tex_sss);
	//shared_ptr<material> mat_sss = make_shared<ggx_nonmetal_material>(0.05, 0.058, tex_sss);

	//world.add(make_shared<sphere>(point3(0, -1, -6.5), 1, mat_sss));
	//world.add(make_shared<sphere>(point3(0, 0, -6), 1.5, mat_sphere));
	

	// 大作业要求物体 球
	shared_ptr<sphere> sphere_1 = make_shared<sphere>(point3(-0.85, -1.7, -6.0), 0.3, al);
	world.add(sphere_1);

	// 大作业要求物体 圆柱
	shared_ptr<texture> tex_cylinder = make_shared<color_map>("obj/white.png", 1.0);
	shared_ptr<material> mat_cylinder = make_shared<ggx_nonmetal_material>(0.02, 0.04, tex_cylinder);
	shared_ptr<cylinder> cylinder_1 = make_shared<cylinder>(point3(-0.9, -0.57, -6.1), 0.3, 0.12, mat_cylinder);
	world.add(cylinder_1);

	// 大作业要求物体 圆锥
	shared_ptr<cone> cone_1 = make_shared<cone>(point3(1.61, -1.2, -6.2), -0.8, 0.45, au);
	world.add(cone_1);

	// cornell box
	world.add(make_shared<triangle>(tri1));
	world.add(make_shared<triangle>(tri2));
	world.add(make_shared<triangle>(tri3));
	world.add(make_shared<triangle>(tri4));
	world.add(make_shared<triangle>(tri5));
	world.add(make_shared<triangle>(tri6));
	world.add(make_shared<triangle>(tri7));
	world.add(make_shared<triangle>(tri8));
	world.add(make_shared<triangle>(tri9));
	world.add(make_shared<triangle>(tri10));
	//world.add(make_shared<triangle>(tri11));
	//world.add(make_shared<triangle>(tri12));
	world.add(make_shared<triangle>(tri13));
	world.add(make_shared<triangle>(tri14));
	world.add(make_shared<triangle>(tri15));
	world.add(make_shared<triangle>(tri16));

	// obj
	shared_ptr<texture> tex_cow = make_shared<color_map>("obj/spot_texture.png");
	shared_ptr<material> mat_cow = make_shared<ggx_nonmetal_material>(0.05, 0.058, tex_cow);

	shared_ptr<texture> tex_cube = make_shared<color_map>("obj/honey_d.jpg");
	shared_ptr<texture> tex_cube_n = make_shared<normal_map>("obj/honey_n.jpg");
	shared_ptr<material> mat_cube = make_shared<ggx_metal_material>(0.1, tex_cube, vec3(0, 0, 0), tex_cube_n);
	
	shared_ptr<texture> tex_bunny = make_shared<simple_color_texture>(255, 255, 255);
	//shared_ptr<texture> tex_bunny = make_shared<simple_color_texture>(vec3(1, 0.782, 0.344));
	//shared_ptr<material> mat_bunny = make_shared<sss_material>(0.058, tex_bunny);
	shared_ptr<material> mat_bunny = make_shared<ggx_nonmetal_material>(0.05, 0, tex_bunny);
	
	shared_ptr<texture> tex_plant = make_shared<color_map>("obj/plant_d.jpg");
	shared_ptr<texture> tex_plant_n = make_shared<normal_map>("obj/plant_n_png.png");
	shared_ptr<material> mat_plant_leaves = make_shared<ggx_nonmetal_material>(0.05, 0.05, tex_plant, vec3(0, 0, 0), tex_plant_n);
	shared_ptr<material> mat_plant_pot_outside = make_shared<ggx_nonmetal_material>(0.1, 0.1, tex_plant, vec3(0, 0, 0), tex_plant_n);

	shared_ptr<texture> tex_wood = make_shared<color_map>("obj/wood_d.jpg");
	shared_ptr<texture> tex_wood_n = make_shared<normal_map>("obj/wood_n.jpg");
	shared_ptr<material> mat_wood = make_shared<ggx_nonmetal_material>(0.12, 0.08, tex_wood, vec3(0, 0, 0), tex_wood_n);

	shared_ptr<texture> tex_fabric = make_shared<color_map>("obj/fabric_d.jpg");
	shared_ptr<texture> tex_fabric_n = make_shared<normal_map>("obj/fabric_n.jpg");
	shared_ptr<texture> tex_fabric_displacement = make_shared<displacement_map>("obj/fabric_displacement.jpg", 1, 0.016);
	shared_ptr<material> mat_fabric = make_shared<ggx_nonmetal_material>(0.1, 0.03, tex_fabric, vec3(0, 0, 0), tex_fabric_n, nullptr, nullptr, tex_fabric_displacement);
	
	disney_brdf_property property;
	property.roughness = 0.01;
	property.specular = 1;
	property.metallic = 1;
	shared_ptr<material> mat_disney_test = make_shared<disney_material>(property, tex_cow);

	shared_ptr<material> mat_default = make_shared<phong_material>(vec3(1, 1, 1));

	unordered_map<string, shared_ptr<material>> material_dict;
	material_dict[string("mat_cow")] = mat_cow;
	material_dict[string("mat_cube")] = mat_cube;
	material_dict[string("mat_bunny")] = mat_sphere;
	material_dict[string("mat_horse")] = mat_sphere;
	material_dict[string("mat_plant_ground")] = mat_plant_leaves;
	material_dict[string("mat_plant_pot_outside")] = mat_plant_pot_outside;
	material_dict[string("mat_plant_pot_inside")] = mat_plant_leaves;
	material_dict[string("mat_plant_leaves")] = mat_plant_leaves;
	material_dict[string("mat_chair_high")] = mat_wood;
	material_dict[string("mat_chair_medium")] = mat_fabric;
	material_dict[string("mat_chair_low")] = mat_wood;

	shared_ptr<mesh_triangle> mesh = make_shared<dict_material_obj_mesh>("obj/test_chair.obj", material_dict, mat_default);
	//mesh->set_translate(vec3(-0.3, 0, -1.5));
	world.add(mesh);


	// 获取world的bvh根节点
	std::cout << "generating bvh...\n";
	shared_ptr<hittable> bvh_root_ptr = generate_bvh(world);
	std::cout << "bvh is ready\n";
	std::cout << bvh_node::node_num << " bvh nodes in total\n";


	// 设置light

	// 圆形光源
	vec3 light_center = vec3(0, 1.99, -6);
	vec3 light_normal = unit_vector(vec3(0, -1, 0));
	vec3 light_radiance_sphere = vec3(22, 22, 22) * vec3(255.0 / 255.0, 228.0 / 255.0, 200.0 / 255.0);
	circle_light light_1(light_center, light_normal, 0.5, light_radiance_sphere);
	shared_ptr<light> light_ptr_1 = make_shared<circle_light>(light_1); 

	// 三角形光源（天花板）
	vec3 light_radiance_ceiling = vec3(22, 22, 22) * vec3(255.0 / 255.0, 228.0 / 255.0, 200.0 / 255.0);
	triangle_light light_2(light_vertex_1, light_vertex_2, light_vertex_3, light_radiance_ceiling);
	triangle_light light_3(light_vertex_1, light_vertex_3, light_vertex_4, light_radiance_ceiling);
	shared_ptr<light> light_ptr_2 = make_shared<triangle_light>(light_2); // 三角形光源
	shared_ptr<light> light_ptr_3 = make_shared<triangle_light>(light_3); // 三角形光源

	// 三角形光源（侧面）
	vec3 light_radiance_wall = vec3(6, 6, 6);
	triangle_light light_4(light_vertex_7, light_vertex_6, light_vertex_5, light_radiance_wall);
	triangle_light light_5(light_vertex_8, light_vertex_7, light_vertex_5, light_radiance_wall);
	shared_ptr<light> light_ptr_4 = make_shared<triangle_light>(light_4); // 三角形光源
	shared_ptr<light> light_ptr_5 = make_shared<triangle_light>(light_5); // 三角形光源

	vector<shared_ptr<light>> light_ptr_list;
	//light_ptr_list.push_back(light_ptr_1);
	light_ptr_list.push_back(light_ptr_2);
	light_ptr_list.push_back(light_ptr_3);
	light_ptr_list.push_back(light_ptr_4);
	light_ptr_list.push_back(light_ptr_5);


	// 设置camera
	camera cam(aspect_ratio);


	// 初始化framebuffer
	vector<vec3> framebuffer;
	framebuffer.assign(image_height * image_width, vec3(0, 0, 0));


	// 开始渲染
	constexpr bool multi_thread = true;
	if (multi_thread) {
		thread t1(render_bvh, image_height, image_width, samples_per_pixel, max_depth, bvh_root_ptr, cam, &framebuffer, light_ptr_list, 8, 0);
		thread t2(render_bvh, image_height, image_width, samples_per_pixel, max_depth, bvh_root_ptr, cam, &framebuffer, light_ptr_list, 8, 1);
		thread t3(render_bvh, image_height, image_width, samples_per_pixel, max_depth, bvh_root_ptr, cam, &framebuffer, light_ptr_list, 8, 2);
		thread t4(render_bvh, image_height, image_width, samples_per_pixel, max_depth, bvh_root_ptr, cam, &framebuffer, light_ptr_list, 8, 3);
		thread t5(render_bvh, image_height, image_width, samples_per_pixel, max_depth, bvh_root_ptr, cam, &framebuffer, light_ptr_list, 8, 4);
		thread t6(render_bvh, image_height, image_width, samples_per_pixel, max_depth, bvh_root_ptr, cam, &framebuffer, light_ptr_list, 8, 5);
		thread t7(render_bvh, image_height, image_width, samples_per_pixel, max_depth, bvh_root_ptr, cam, &framebuffer, light_ptr_list, 8, 6);
		thread t8(render_bvh, image_height, image_width, samples_per_pixel, max_depth, bvh_root_ptr, cam, &framebuffer, light_ptr_list, 8, 7);
		t1.join();
		t2.join();
		t3.join();
		t4.join();
		t5.join();
		t6.join();
		t7.join();
		t8.join();
	}
	else {
		thread t1(render_bvh, image_height, image_width, samples_per_pixel, max_depth, bvh_root_ptr, cam, &framebuffer, light_ptr_list, 1, 0);
		t1.join();
	}
	

	// 输出png
	
	// 创建一个OpenCV图像对象
	cv::Mat image(image_height, image_width, CV_8UC3);

	// 将像素值从framebuffer复制到OpenCV图像对象
	for (int y = 0; y < image_height; ++y) {

		std::cerr << "\r.png Pixels remaining: " << (image_width - y) * image_height << ' ' << std::flush;
		for (int x = 0; x < image_width; ++x) {
			// framebuffer[y][x]是您渲染器计算得到的像素值
			// 这里假设每个像素值存储为3个通道（BGR格式）
			cv::Vec3b& pixel = image.at<cv::Vec3b>(y, x);
			pixel[0] = static_cast<int>(256 * clamp(framebuffer[y * image_width + x].z(), 0.0, 0.999));  // 蓝色通道
			pixel[1] = static_cast<int>(256 * clamp(framebuffer[y * image_width + x].y(), 0.0, 0.999));  // 绿色通道
			pixel[2] = static_cast<int>(256 * clamp(framebuffer[y * image_width + x].x(), 0.0, 0.999));  // 红色通道
		}
	}

	// 输出
	std::time_t currentTime = std::time(nullptr);
	std::tm* localTime = std::localtime(&currentTime);
	char buffer[80];
	std::strftime(buffer, sizeof(buffer), "%Y%m%d%H%M%S", localTime);
	std::string currentTime_str = std::string(buffer);
	std::string filename = "out/" + currentTime_str + ".png";
	cv::imwrite(filename, image);

	std::cout << "\nwrite finish" << std::endl;
	
#endif
}