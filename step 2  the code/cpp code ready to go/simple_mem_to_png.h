//#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <png.h>

class simple_mem_to_png
{
public:
	//tutorial
	void how_to();

	//basic
	struct pixel_struct {
		uint8_t red;
		uint8_t green;
		uint8_t blue;
		uint8_t alpha;
	};
	struct bitmap_struct {
		pixel_struct* pixels;
		size_t width;
		size_t height;
	};
	bitmap_struct img;
	
	//ctor
	simple_mem_to_png();
	~simple_mem_to_png();

	//util
	size_t get_offset(int x, int y);

	//debug info
	enum class error_type
	{
		OK,//means no error occurs

		Failed_to_open_file,
		Failed_to_create_PNG_write_struct,
		Failed_to_create_PNG_info_struct,
		Failed_to_set_jmp_for_PNG_lib,

		Bad_param
	};

	error_type alloc(int width, int height);

	error_type save_to_file(const char* filename);
	

};

