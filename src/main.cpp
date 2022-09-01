#include "renderer.h"
#include <thread>
#include <algorithm>
#include "bvh_node.h"

#define PPM_FILE_NAME "out/out.ppm"

using std::vector;
using std::fstream;
using std::thread;
using std::cout;


// 特性列表
// 1.  图像纹理						完成
// 2.  法线贴图（预计算切线）			完成	
// 3.  视差贴图					
// 4.  hdri
// 5.  meshtriangle类				完成
// 6.  基础obj加载					完成
// 7.  景深
// 8.  bloom
// 9.  ggx多次弹射补偿
// 10. vec3替换为eigen::Vector3d
// 11. bssrdf
// 12. btdf
// 13. 体积


#define do_render

int main()
{
#ifdef do_render
	// 初始化
	const double aspect_ratio = 1; // 16.0 / 9.0
	const int image_width = 1024; //800
	const int image_height = static_cast<int>(image_width / aspect_ratio);
	const int samples_per_pixel = 8;
	const int max_depth = 5;

	// 打开图像文件
	std::fstream file;
	file.open(PPM_FILE_NAME, std::ios::out);

	// 确认是否打开成功
	if (file.is_open()) std::cout << "file open: " << PPM_FILE_NAME << std::endl;

	// 写入图像格式
	file << "P3\n" << image_width << ' ' << image_height << "\n255\n";

	const int threads = 6;

	// 设置object
	hittable_list world;

	// 简单material
	shared_ptr<material> light_low = make_shared<phong_material>(vec3(1.0, 1.0, 1.0), vec3(3, 3, 3));
	shared_ptr<material> light_super = make_shared<phong_material>(vec3(1.0, 1.0, 1.0), vec3(10, 10, 10));
	shared_ptr<material> light_false = make_shared<phong_material>(vec3(0, 0, 0), vec3(1, 1, 1));
	shared_ptr<material> red = make_shared<phong_material>(vec3(0.5, 0.05, 0.05), vec3(0, 0, 0));
	shared_ptr<material> blue = make_shared<phong_material>(vec3(0, 0, 1.0), vec3(0, 0, 0));
	shared_ptr<material> green = make_shared<phong_material>(vec3(0.05, 0.5, 0.05), vec3(0, 0, 0));
	shared_ptr<material> phong_2 = make_shared<phong_material>(vec3(1.0, 1.0, 1.0), vec3(0, 0, 0));
	shared_ptr<material> white = make_shared<phong_material>(vec3(1, 1, 1), vec3(0, 0, 0));
	shared_ptr<material> gold = make_shared<ggx_material>(0.2);
	shared_ptr<material> stone = make_shared<ggx_material>(0.8, vec3(0.04, 0.04, 0.04));
	shared_ptr<material> aluminum = make_shared<ggx_material>(0.003, vec3(0.939, 0.914, 0.822));
	shared_ptr<material> copper = make_shared<ggx_material>(0.6, vec3(0.904, 0.372, 0.256));

	// 场景顶点
	vec3 vertex_1(-2, -2, -8);
	vec3 vertex_2(2, -2, -8);
	vec3 vertex_3(2, -2, -4);
	vec3 vertex_4(-2, -2, -4);
	vec3 vertex_5(-2, 2, -8);
	vec3 vertex_6(2, 2, -8);
	vec3 vertex_7(2, 2, -4);
	vec3 vertex_8(-2, 2, -4);
	vec3 light_vertex_1 = vec3(-0.5, 1.999, -2.5);
	vec3 light_vertex_2 = vec3(0.5, 1.999, -2.5);
	vec3 light_vertex_3 = vec3(0.5, 1.999, -1.5);
	vec3 light_vertex_4 = vec3(-0.5, 1.999, -1.5);

	// 三角形实例
	auto box_material = white;
	triangle tri1(vertex_3, vertex_2, vertex_1, box_material);
	triangle tri2(vertex_1, vertex_4, vertex_3, box_material);
	triangle tri3(vertex_5, vertex_2, vertex_6, box_material);
	triangle tri4(vertex_1, vertex_2, vertex_5, box_material);
	triangle tri5(vertex_1, vertex_8, vertex_4, green);
	triangle tri6(vertex_1, vertex_5, vertex_8, green);
	triangle tri7(vertex_2, vertex_3, vertex_7, red);
	triangle tri8(vertex_2, vertex_6, vertex_7, red);
	triangle tri9(vertex_5, vertex_6, vertex_8, box_material);
	triangle tri10(vertex_6, vertex_7, vertex_8, box_material);
	triangle tri11(vertex_8, vertex_7, vertex_4, box_material);
	triangle tri12(vertex_7, vertex_3, vertex_4, box_material);
	triangle tri13(light_vertex_1, light_vertex_2, light_vertex_3, light_false);
	triangle tri14(light_vertex_1, light_vertex_3, light_vertex_4, light_false);

	// 设置world

	// 球
	//world.add(make_shared<sphere>(point3(0.8, -1.2, -5.2), 0.79, aluminum));
	//world.add(make_shared<sphere>(point3(-1.3, 1.3, -7), 0.6, aluminum));
	
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
	//world.add(make_shared<triangle>(tri13));
	//world.add(make_shared<triangle>(tri14));

	// obj
	shared_ptr<texture> tex_haku_000 = make_shared<color_map>("obj/final/haku_tex/face_Haku.png");
	shared_ptr<texture> tex_haku_00B = make_shared<color_map>("obj/final/haku_tex/4tex.png");
	shared_ptr<texture> tex_haku_00C = make_shared<color_map>("obj/final/haku_tex/hair_MikuAp.png");
	shared_ptr<texture> tex_haku_00R = make_shared<color_map>("obj/final/haku_tex/jiao.png");
	shared_ptr<texture> tex_haku_00S = make_shared<color_map>("obj/final/haku_tex/jinshi_1.png");
	shared_ptr<texture> tex_haku_00S_normal = make_shared<normal_map>("obj/final/haku_tex/jinshi_normal.png");
	shared_ptr<texture> tex_haku_00V = make_shared<color_map>("obj/final/haku_tex/jinshi.png");
	shared_ptr<texture> tex_haku_01C = make_shared<color_map>("obj/final/haku_tex/jiaoshi_1.png");
	shared_ptr<texture> tex_haku_01E = make_shared<color_map>("obj/final/haku_tex/jingshi_1.png");
	shared_ptr<texture> tex_haku_01L = make_shared<color_map>("obj/final/haku_tex/qunzi_1_2.png");
	shared_ptr<texture> tex_haku_002 = make_shared<color_map>("obj/final/haku_tex/face_Haku_R.png");
	shared_ptr<texture> tex_haku_004 = make_shared<color_map>("obj/final/haku_tex/face_HakuOp_bc.png");
	shared_ptr<texture> tex_haku_009 = make_shared<color_map>("obj/final/haku_tex/body00_MikuAp_b_1.png");
	shared_ptr<texture> tex_haku_012 = make_shared<color_map>("obj/final/haku_tex/xiezi_01_2.png");
	shared_ptr<texture> tex_haku_013 = make_shared<color_map>("obj/final/haku_tex/qunzi_1_1.png");
	shared_ptr<texture> tex_haku_017 = make_shared<color_map>("obj/final/haku_tex/erzhui.png");
	
	shared_ptr<texture> tex_cow = make_shared<color_map>("obj/final/spot_texture.png");
	shared_ptr<texture> tex_cube_d = make_shared<color_map>("obj/final/honey_d.jpg");
	shared_ptr<texture> tex_cube_n = make_shared<normal_map>("obj/final/honey_n.jpg");

	shared_ptr<material> mat_haku_000 = make_shared<phong_material>(tex_haku_000);
	shared_ptr<material> mat_haku_00B = make_shared<phong_material>(tex_haku_00B);
	shared_ptr<material> mat_haku_00C = make_shared<phong_material>(tex_haku_00C);
	shared_ptr<material> mat_haku_00R = make_shared<phong_material>(tex_haku_00R);
	shared_ptr<material> mat_haku_00S = make_shared<phong_material>(tex_haku_00S, vec3(0, 0, 0), tex_haku_00S_normal);
	shared_ptr<material> mat_haku_00V = make_shared<phong_material>(tex_haku_00V);
	shared_ptr<material> mat_haku_01C = make_shared<phong_material>(tex_haku_01C);
	shared_ptr<material> mat_haku_01E = make_shared<phong_material>(tex_haku_01E);
	shared_ptr<material> mat_haku_01L = make_shared<phong_material>(tex_haku_01L);
	shared_ptr<material> mat_haku_002 = make_shared<phong_material>(tex_haku_002);
	shared_ptr<material> mat_haku_004 = make_shared<phong_material>(tex_haku_004);
	shared_ptr<material> mat_haku_009 = make_shared<phong_material>(tex_haku_009);
	shared_ptr<material> mat_haku_012 = make_shared<phong_material>(tex_haku_012);
	shared_ptr<material> mat_haku_013 = make_shared<phong_material>(tex_haku_013);
	shared_ptr<material> mat_haku_017 = make_shared<phong_material>(tex_haku_017);
	
	shared_ptr<material> mat_cow = make_shared<ggx_material>(0.7, tex_cow);
	shared_ptr<material> mat_cube = make_shared<ggx_material>(0.1, tex_cube_d, vec3(0, 0, 0), tex_cube_n);
	shared_ptr<material> mat_bunny = white;

	shared_ptr<material> mat_default = make_shared<phong_material>(vec3(1, 1, 1));

	unordered_map<string, shared_ptr<material>> material_dict;
	material_dict[string("mat_cow")] = mat_cow;
	material_dict[string("mat_cube")] = mat_cube;
	material_dict[string("mat_bunny")] = mat_bunny;
	material_dict[string("mat_haku_000")] = material_dict[string("mat_haku_000_2")] = mat_haku_000;
	material_dict[string("mat_haku_002")] = material_dict[string("mat_haku_002_2")] = mat_haku_002;
	material_dict[string("mat_haku_004")] = material_dict[string("mat_haku_004_2")] = mat_haku_004;
	material_dict[string("mat_haku_009")] = material_dict[string("mat_haku_009_2")] = mat_haku_009;
	material_dict[string("mat_haku_00B")] = material_dict[string("mat_haku_00B_2")] = mat_haku_00B;
	material_dict[string("mat_haku_00C")] = material_dict[string("mat_haku_00C_2")] = mat_haku_00C;
	material_dict[string("mat_haku_00R")] = material_dict[string("mat_haku_00R_2")] = mat_haku_00R;
	material_dict[string("mat_haku_00S")] = material_dict[string("mat_haku_00S_2")] = mat_haku_00S;
	material_dict[string("mat_haku_00V")] = material_dict[string("mat_haku_00V_2")] = mat_haku_00V;
	material_dict[string("mat_haku_012")] = material_dict[string("mat_haku_012_2")] = mat_haku_012;
	material_dict[string("mat_haku_013")] = material_dict[string("mat_haku_013_2")] = mat_haku_013;
	material_dict[string("mat_haku_017")] = material_dict[string("mat_haku_017_2")] = mat_haku_017;
	material_dict[string("mat_haku_01C")] = material_dict[string("mat_haku_01C_2")] = mat_haku_01C;
	material_dict[string("mat_haku_01E")] = material_dict[string("mat_haku_01E_2")] = mat_haku_01E;
	material_dict[string("mat_haku_01L")] = material_dict[string("mat_haku_01L_2")] = mat_haku_01L;


	shared_ptr<mesh_triangle> mesh = make_shared<dict_material_obj_mesh>("obj/final/final.obj", material_dict, mat_default);
	world.add(mesh);



	// 获取world的bvh根节点
	std::cout << "generating bvh...\n";
	shared_ptr<hittable> bvh_root_ptr = generate_bvh(world);
	std::cout << "bvh is ready\n";

	// 设置light
	vec3 light_center = vec3(0, 1.5, -2);
	vec3 light_normal = unit_vector(vec3(0, -1, -4));
	vec3 light_radiance = vec3(26, 26, 26);
	circle_light light_1(light_center, light_normal, 0.5, light_radiance);
	shared_ptr<light> light_ptr_1 = make_shared<circle_light>(light_1); // 圆形光源

	triangle_light light_2(light_vertex_1, light_vertex_2, light_vertex_3, light_radiance);
	triangle_light light_3(light_vertex_1, light_vertex_3, light_vertex_4, light_radiance);
	shared_ptr<light> light_ptr_2 = make_shared<triangle_light>(light_2); // 三角形光源
	shared_ptr<light> light_ptr_3 = make_shared<triangle_light>(light_3); // 三角形光源

	vector<shared_ptr<light>> light_ptr_list;
	light_ptr_list.push_back(light_ptr_1);
	//light_ptr_list.push_back(light_ptr_2);
	//light_ptr_list.push_back(light_ptr_3);

	// 设置camera
	camera cam(aspect_ratio);

	// 初始化framebuffer
	vector<vec3> framebuffer;
	framebuffer.assign(image_height * image_width, vec3(0, 0, 0));

	// 开始渲染
	// 由于triangle类的hit()函数有问题，这里world还是会复制给每个线程
	
	/*
	thread t1(render_raw, image_height, image_width, samples_per_pixel, max_depth, world, cam, &framebuffer, light_ptr_list, 6, 0);
	thread t2(render_raw, image_height, image_width, samples_per_pixel, max_depth, world, cam, &framebuffer, light_ptr_list, 6, 1);
	thread t3(render_raw, image_height, image_width, samples_per_pixel, max_depth, world, cam, &framebuffer, light_ptr_list, 6, 2);
	thread t4(render_raw, image_height, image_width, samples_per_pixel, max_depth, world, cam, &framebuffer, light_ptr_list, 6, 3);
	thread t5(render_raw, image_height, image_width, samples_per_pixel, max_depth, world, cam, &framebuffer, light_ptr_list, 6, 4);
	thread t6(render_raw, image_height, image_width, samples_per_pixel, max_depth, world, cam, &framebuffer, light_ptr_list, 6, 5);
	*/
	///*
	thread t1(render_bvh, image_height, image_width, samples_per_pixel, max_depth, bvh_root_ptr, cam, &framebuffer, light_ptr_list, 6, 0);
	thread t2(render_bvh, image_height, image_width, samples_per_pixel, max_depth, bvh_root_ptr, cam, &framebuffer, light_ptr_list, 6, 1);
	thread t3(render_bvh, image_height, image_width, samples_per_pixel, max_depth, bvh_root_ptr, cam, &framebuffer, light_ptr_list, 6, 2);
	thread t4(render_bvh, image_height, image_width, samples_per_pixel, max_depth, bvh_root_ptr, cam, &framebuffer, light_ptr_list, 6, 3);
	thread t5(render_bvh, image_height, image_width, samples_per_pixel, max_depth, bvh_root_ptr, cam, &framebuffer, light_ptr_list, 6, 4);
	thread t6(render_bvh, image_height, image_width, samples_per_pixel, max_depth, bvh_root_ptr, cam, &framebuffer, light_ptr_list, 6, 5);
	//*/
	///*
	t1.join();
	t2.join();
	t3.join();
	t4.join();
	t5.join();
	t6.join();
	//*/

	//render_raw(image_height, image_width, samples_per_pixel, max_depth, world, cam, &framebuffer, light_ptr_list, 1, 0);
	//render_bvh(image_height, image_width, samples_per_pixel, max_depth, bvh_root_ptr, cam, &framebuffer, light_ptr_list, 1, 0);

	// 将颜色从framebuffer写入ppm文件中
	std::cout << "\nrender finish" << std::endl;
	for (int i = 0; i < image_height * image_width; i++)
	{
		if (i % 10000 == 0) {
			std::cerr << "\rPixels remaining: " << image_height * image_width - i << ' ' << std::flush;
		}

		double r = framebuffer[i].x();
		double g = framebuffer[i].y();
		double b = framebuffer[i].z();

		file << static_cast<int>(256 * clamp(r, 0.0, 0.999)) << ' '
		<< static_cast<int>(256 * clamp(g, 0.0, 0.999)) << ' '
		<< static_cast<int>(256 * clamp(b, 0.0, 0.999)) << '\n';
	}

	file.close();

	std::cout << "\nwrite finish" << std::endl;
	
#endif

	char a;
	std::cin >> a;
}