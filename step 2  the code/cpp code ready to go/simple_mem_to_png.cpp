#include "simple_mem_to_png.h"

//tutorial
void simple_mem_to_png::how_to()
{
	//step 1
	//to get mem from os
	alloc(200, 100);
	//now you have 100*100*4 bytes for the uncompressed color info.

	//step 2
	//let's modify the mem(draw something)
	for (size_t y = 0; y < 100; ++y)
		for (size_t x = 0; x < 200; ++x)
		{	
			//I prepared a function called get_offset to help you 
			size_t offset = get_offset(x, y);

			img.pixels[offset].red = unsigned char(x);
			img.pixels[offset].green = unsigned char(y);
			img.pixels[offset].blue = rand() & 0xff;//equivalent to %256
			if (x > y)
				img.pixels[offset].alpha = 0xff;
			else
				img.pixels[offset].alpha = 0xaf;
			//if you don't know the detail of this part, it's ok. It's ez to find it out in the result .png file.
		}

	//step 3
	//save the info to a file.
	save_to_file("how to.png");

	//step 4
	//to give the mem back to os, all you need to do is destruct this object. The dtor does it automatically for you.

	//by the way, saving doesn'g destroy the mem, which means after a saving, the mem is the same. 
	//So, it's very convenient to save a series of .png files.
	//All you need to do is repeating the step 2 and 3.

	//All the members(data and functions) are defined in public, means it's ez to modify all of them at any time.
}



	
	
//ctor
simple_mem_to_png::simple_mem_to_png()
{
	img.pixels = nullptr;
}
simple_mem_to_png::~simple_mem_to_png()
{
	if (img.pixels)
		delete[] img.pixels;
}

//util
size_t simple_mem_to_png::get_offset(int x, int y)
{
	return (y*img.width + x);// pixil_size == 4
}

	

simple_mem_to_png::error_type simple_mem_to_png::alloc(int width, int height)
{
	if (width <= 0 || height <= 0)
		return error_type::Bad_param;
		
	img.height = height;
	img.width = width;

	if (img.pixels)
		delete[]img.pixels;
	img.pixels = new pixel_struct[img.height*img.width];

	return error_type::OK;
}



simple_mem_to_png::error_type simple_mem_to_png::save_to_file(const char* filename)
{
	FILE * fp;
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	//size_t x, y;
	png_byte ** row_pointers = NULL;
	int pixel_size(4);
	int depth(8);

	//set up
	auto result = fopen_s(&fp, filename, "wb");
	if (!fp) {
		return error_type::Failed_to_open_file;
	}

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL) {
		fclose(fp);
		return error_type::Failed_to_create_PNG_write_struct;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		png_destroy_write_struct(&png_ptr, &info_ptr);
		fclose(fp);
		return error_type::Failed_to_create_PNG_info_struct;
	}

	if (setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_write_struct(&png_ptr, &info_ptr);
		fclose(fp);
		return error_type::Failed_to_set_jmp_for_PNG_lib;
	}

	png_set_IHDR(png_ptr,info_ptr,img.width, img.height,depth,
		PNG_COLOR_TYPE_RGBA,PNG_INTERLACE_NONE,PNG_COMPRESSION_TYPE_DEFAULT,PNG_FILTER_TYPE_DEFAULT);

	png_init_io(png_ptr, fp);

	//prepare the row_pointers
	row_pointers = new png_byte*[img.height];
	for (size_t i = 0; i < img.height; ++i)
	{
		row_pointers[i] = reinterpret_cast<png_byte*>(img.pixels + i * img.width);
	}
	png_set_rows(png_ptr, info_ptr, row_pointers);
	//write to file.
	png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

	//clear
	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(fp);

	//return
	return error_type::OK;

}//function def save_to_file


int main()
{
	simple_mem_to_png aaa;

	aaa.how_to();
	
	return 0;
}