//This file is modified from official package from libpng.
//The file is named pngtest.c
//I simplified it, and then save to https://github.com/YagaoDirac/eazy-libpng-tutorial
//For more info, http://libpng.org/

#define _CRT_SECURE_NO_WARNINGS//I added this line to surpress the warning to fopen( the correct way is to replace fopen with fopen_s)

#define _POSIX_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "png.h"
#  include "zlib.h"


static int tIME_chunk_present = 0;
static char tIME_string[29] = "tIME chunk is not present";



static int verbose = 0;
static int strict = 0;
static int relaxed = 0;
static int xfail = 0;
static int unsupported_chunks = 0; /* chunk unsupported by libpng in input */
static int error_count = 0; /* count calls to png_error */
static int warning_count = 0; /* count calls to png_warning */


/* Example of using row callbacks to make a simple progress meter */
static int status_pass = 1;
static int status_dots_requested = 0;
static int status_dots = 1;

static void PNGCBAPI
read_row_callback(png_structp png_ptr, png_uint_32 row_number, int pass)
{
   if (png_ptr == NULL || row_number > PNG_UINT_31_MAX)
      return;

   if (status_pass != pass)
   {
      status_pass = pass;
      status_dots = 31;
   }

   status_dots--;

   if (status_dots == 0)
   {
      status_dots=30;
   }
}

#ifdef PNG_WRITE_SUPPORTED
static void PNGCBAPI
write_row_callback(png_structp png_ptr, png_uint_32 row_number, int pass)
{
   if (png_ptr == NULL || row_number > PNG_UINT_31_MAX || pass > 7)
      return;

   fprintf(stdout, "w");
}
#endif



/* This function is called when there is a warning, but the library thinks
 * it can continue anyway.  Replacement functions don't have to do anything
 * here if you don't want to.  In the default configuration, png_ptr is
 * not used, but it is passed in case it may be useful.
 */
typedef struct
{
   const char *file_name;
}  pngtest_error_parameters;

static void PNGCBAPI
pngtest_warning(png_structp png_ptr, png_const_charp message)
{
   const char *name = "UNKNOWN (ERROR!)";
   pngtest_error_parameters *test =
      (pngtest_error_parameters*)png_get_error_ptr(png_ptr);

   ++warning_count;

   if (test != NULL && test->file_name != NULL)
      name = test->file_name;
}

/* This is the default error handling function.  Note that replacements for
 * this function MUST NOT RETURN, or the program will likely crash.  This
 * function is used by default, or if the program supplies NULL for the
 * error function pointer in png_set_error_fn().
 */
static void PNGCBAPI
pngtest_error(png_structp png_ptr, png_const_charp message)
{
   ++error_count;

   pngtest_warning(png_ptr, message);
   /* We can return because png_error calls the default handler, which is
    * actually OK in this case.
    */
}

/* END of code to validate stdio-free compilation */



/* Demonstration of user chunk support of the sTER and vpAg chunks */

/* (sTER is a public chunk not yet known by libpng.  vpAg is a private
chunk used in ImageMagick to store "virtual page" size).  */

static struct user_chunk_data
{
   png_const_infop info_ptr;
   png_uint_32     vpAg_width, vpAg_height;
   png_byte        vpAg_units;
   png_byte        sTER_mode;
   int             location[2];
}
user_chunk_data;

/* Used for location and order; zero means nothing. */
#define have_sTER   0x01
#define have_vpAg   0x02
#define before_PLTE 0x10
#define before_IDAT 0x20
#define after_IDAT  0x40

static void
init_callback_info(png_const_infop info_ptr)
{
	((void)memset(&user_chunk_data, 0, sizeof user_chunk_data));
   
   user_chunk_data.info_ptr = info_ptr;
}

static int
set_location(png_structp png_ptr, struct user_chunk_data *data, int what)
{
   int location;

   if ((data->location[0] & what) != 0 || (data->location[1] & what) != 0)
      return 0; /* already have one of these */

   /* Find where we are (the code below zeroes info_ptr to indicate that the
    * chunks before the first IDAT have been read.)
    */
   if (data->info_ptr == NULL) /* after IDAT */
      location = what | after_IDAT;

   else if (png_get_valid(png_ptr, data->info_ptr, PNG_INFO_PLTE) != 0)
      location = what | before_IDAT;

   else
      location = what | before_PLTE;

   if (data->location[0] == 0)
      data->location[0] = location;

   else
      data->location[1] = location;

   return 1; /* handled */
}

static int PNGCBAPI
read_user_chunk_callback(png_struct *png_ptr, png_unknown_chunkp chunk)
{
   struct user_chunk_data *my_user_chunk_data =
      (struct user_chunk_data*)png_get_user_chunk_ptr(png_ptr);

   if (my_user_chunk_data == NULL)
      png_error(png_ptr, "lost user chunk pointer");

   /* Return one of the following:
    *    return (-n);  chunk had an error
    *    return (0);  did not recognize
    *    return (n);  success
    *
    * The unknown chunk structure contains the chunk data:
    * png_byte name[5];
    * png_byte *data;
    * size_t size;
    *
    * Note that libpng has already taken care of the CRC handling.
    */

   if (chunk->name[0] == 115 && chunk->name[1] ==  84 &&     /* s  T */
       chunk->name[2] ==  69 && chunk->name[3] ==  82)       /* E  R */
      {
         /* Found sTER chunk */
         if (chunk->size != 1)
            return (-1); /* Error return */

         if (chunk->data[0] != 0 && chunk->data[0] != 1)
            return (-1);  /* Invalid mode */

         if (set_location(png_ptr, my_user_chunk_data, have_sTER) != 0)
         {
            my_user_chunk_data->sTER_mode=chunk->data[0];
            return (1);
         }

         else
            return (0); /* duplicate sTER - give it to libpng */
      }

   if (chunk->name[0] != 118 || chunk->name[1] != 112 ||    /* v  p */
       chunk->name[2] !=  65 || chunk->name[3] != 103)      /* A  g */
      return (0); /* Did not recognize */

   /* Found ImageMagick vpAg chunk */

   if (chunk->size != 9)
      return (-1); /* Error return */

   if (set_location(png_ptr, my_user_chunk_data, have_vpAg) == 0)
      return (0);  /* duplicate vpAg */

   my_user_chunk_data->vpAg_width = png_get_uint_31(png_ptr, chunk->data);
   my_user_chunk_data->vpAg_height = png_get_uint_31(png_ptr, chunk->data + 4);
   my_user_chunk_data->vpAg_units = chunk->data[8];

   return (1);
}


static void
write_sTER_chunk(png_structp write_ptr)
{
   png_byte sTER[5] = {115,  84,  69,  82, '\0'};

   png_write_chunk(write_ptr, sTER, &user_chunk_data.sTER_mode, 1);
}

static void
write_vpAg_chunk(png_structp write_ptr)
{
   png_byte vpAg[5] = {118, 112,  65, 103, '\0'};

   png_byte vpag_chunk_data[9];

   png_save_uint_32(vpag_chunk_data, user_chunk_data.vpAg_width);
   png_save_uint_32(vpag_chunk_data + 4, user_chunk_data.vpAg_height);
   vpag_chunk_data[8] = user_chunk_data.vpAg_units;
   png_write_chunk(write_ptr, vpAg, vpag_chunk_data, 9);
}

static void
write_chunks(png_structp write_ptr, int location)
{
   int i;

   /* Notice that this preserves the original chunk order, however chunks
    * intercepted by the callback will be written *after* chunks passed to
    * libpng.  This will actually reverse a pair of sTER chunks or a pair of
    * vpAg chunks, resulting in an error later.  This is not worth worrying
    * about - the chunks should not be duplicated!
    */
   for (i=0; i<2; ++i)
   {
      if (user_chunk_data.location[i] == (location | have_sTER))
         write_sTER_chunk(write_ptr);

      else if (user_chunk_data.location[i] == (location | have_vpAg))
         write_vpAg_chunk(write_ptr);
   }
}


/* END of code to demonstrate user chunk support */

/* START of code to check that libpng has the required text support; this only
 * checks for the write support because if read support is missing the chunk
 * will simply not be reported back to pngtest.
 */
static void
pngtest_check_text_support(png_structp png_ptr, png_textp text_ptr,
    int num_text)
{
   while (num_text > 0)
   {
      switch (text_ptr[--num_text].compression)
      {
         case PNG_TEXT_COMPRESSION_NONE:
            break;

         case PNG_TEXT_COMPRESSION_zTXt:
#           ifndef PNG_WRITE_zTXt_SUPPORTED
               ++unsupported_chunks;
               /* In libpng 1.7 this now does an app-error, so stop it: */
               text_ptr[num_text].compression = PNG_TEXT_COMPRESSION_NONE;
#           endif
            break;

         case PNG_ITXT_COMPRESSION_NONE:
         case PNG_ITXT_COMPRESSION_zTXt:
#           ifndef PNG_WRITE_iTXt_SUPPORTED
               ++unsupported_chunks;
               text_ptr[num_text].compression = PNG_TEXT_COMPRESSION_NONE;
#           endif
            break;

         default:
            /* This is an error */
            png_error(png_ptr, "invalid text chunk compression field");
            break;
      }
   }
}
/* END of code to check that libpng has the required text support */

/* Test one file */
static int
test_one_file(const char *inname, const char *outname)
{
   static png_FILE_p fpin;
   static png_FILE_p fpout;  /* "static" prevents setjmp corruption */
   pngtest_error_parameters error_parameters;
   png_structp read_ptr;
   png_infop read_info_ptr, end_info_ptr;
   png_structp write_ptr;
   png_infop write_info_ptr;
   png_infop write_end_info_ptr;
   int interlace_preserved = 1;
  
   png_bytep row_buf;
   png_uint_32 y;
   png_uint_32 width, height;
   volatile int num_passes;
   int pass;
   int bit_depth, color_type;

   row_buf = NULL;
   error_parameters.file_name = inname;

   if ((fpin = fopen(inname, "rb")) == NULL)
   {
      return (1);
   }

   if ((fpout = fopen(outname, "wb")) == NULL)
   {
	  fclose(fpin);
      return (1);
   }

   read_ptr =
       png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

   png_set_error_fn(read_ptr, &error_parameters, pngtest_error,
       pngtest_warning);



   write_ptr =
       png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

   png_set_error_fn(write_ptr, &error_parameters, pngtest_error,
       pngtest_warning);



   read_info_ptr = png_create_info_struct(read_ptr);
   end_info_ptr = png_create_info_struct(read_ptr);


   write_info_ptr = png_create_info_struct(write_ptr);
   write_end_info_ptr = png_create_info_struct(write_ptr);




   init_callback_info(read_info_ptr);
   png_set_read_user_chunk_fn(read_ptr, &user_chunk_data,
       read_user_chunk_callback);



   if (setjmp(png_jmpbuf(read_ptr)))
   {
      png_free(read_ptr, row_buf);
      row_buf = NULL;
      png_destroy_read_struct(&read_ptr, &read_info_ptr, &end_info_ptr);
      png_destroy_info_struct(write_ptr, &write_end_info_ptr);
      png_destroy_write_struct(&write_ptr, &write_info_ptr);
      fclose(fpin);
      fclose(fpout);
      return (1);
   }


   if (setjmp(png_jmpbuf(write_ptr)))
   {
      png_free(read_ptr, row_buf);
      row_buf = NULL;
      png_destroy_read_struct(&read_ptr, &read_info_ptr, &end_info_ptr);
      png_destroy_info_struct(write_ptr, &write_end_info_ptr);
      png_destroy_write_struct(&write_ptr, &write_info_ptr);
      fclose(fpin);
      fclose(fpout);
      return (1);
   }


   png_init_io(read_ptr, fpin);
   png_init_io(write_ptr, fpout);

   png_read_info(read_ptr, read_info_ptr);

   /* This is a bit of a hack; there is no obvious way in the callback function
    * to determine that the chunks before the first IDAT have been read, so
    * remove the info_ptr (which is only used to determine position relative to
    * PLTE) here to indicate that we are after the IDAT.
    */
   user_chunk_data.info_ptr = NULL;


   {
      int interlace_type, compression_type, filter_type;

      if (png_get_IHDR(read_ptr, read_info_ptr, &width, &height, &bit_depth,
          &color_type, &interlace_type, &compression_type, &filter_type) != 0)
      {
         png_set_IHDR(write_ptr, write_info_ptr, width, height, bit_depth,
             color_type, interlace_type, compression_type, filter_type);
         /* num_passes may not be available below if interlace support is not
          * provided by libpng for both read and write.
          */
         switch (interlace_type)
         {
            case PNG_INTERLACE_NONE:
               num_passes = 1;
               break;

            case PNG_INTERLACE_ADAM7:
               num_passes = 7;
               break;

            default:
               png_error(read_ptr, "invalid interlace type");
               /*NOT REACHED*/
         }
      }

      else
         png_error(read_ptr, "png_get_IHDR failed");
   }


   {
      png_fixed_point gamma;

      if (png_get_gAMA_fixed(read_ptr, read_info_ptr, &gamma) != 0)
         png_set_gAMA_fixed(write_ptr, write_info_ptr, gamma);
   }


   {
      int intent;

      if (png_get_sRGB(read_ptr, read_info_ptr, &intent) != 0)
         png_set_sRGB(write_ptr, write_info_ptr, intent);
   }

   {
      png_uint_32 res_x, res_y;
      int unit_type;

      if (png_get_pHYs(read_ptr, read_info_ptr, &res_x, &res_y,
          &unit_type) != 0)
         png_set_pHYs(write_ptr, write_info_ptr, res_x, res_y, unit_type);
   }

  
   /* Write the info in two steps so that if we write the 'unknown' chunks here
    * they go to the correct place.
    */

   png_write_info_before_PLTE(write_ptr, write_info_ptr);

   write_chunks(write_ptr, before_PLTE); /* before PLTE */

   png_write_info(write_ptr, write_info_ptr);

   write_chunks(write_ptr, before_IDAT); /* after PLTE */

   png_write_info(write_ptr, write_end_info_ptr);

   write_chunks(write_ptr, after_IDAT); /* after IDAT */



   row_buf = (png_bytep)png_malloc(read_ptr,
       png_get_rowbytes(read_ptr, read_info_ptr));

   for (pass = 0; pass < num_passes; pass++)
   {
      for (y = 0; y < height; y++)
      {
         png_read_rows(read_ptr, (png_bytepp)&row_buf, NULL, 1);

		 row_buf[0] = 111;
		 row_buf[1] = 255;
		 row_buf[2] = 255;

		 row_buf[4] = 0;
		 row_buf[5] = 255;
		 row_buf[6] = 111;

		 row_buf[8] = 255;
		 row_buf[9] = 0;
		 row_buf[10] = 255;
		 row_buf[11] = 200;


         png_write_rows(write_ptr, (png_bytepp)&row_buf, 1);
      }
   }

    png_free_data(read_ptr, read_info_ptr, PNG_FREE_UNKN, -1);
    png_free_data(write_ptr, write_info_ptr, PNG_FREE_UNKN, -1);







   
   png_free(read_ptr, row_buf);
   row_buf = NULL;
   png_destroy_read_struct(&read_ptr, &read_info_ptr, &end_info_ptr);
   
   png_destroy_info_struct(write_ptr, &write_end_info_ptr);
   png_destroy_write_struct(&write_ptr, &write_info_ptr);

   fclose(fpin);
   fclose(fpout);

   /* Summarize any warnings or errors and in 'strict' mode fail the test.
    * Unsupported chunks can result in warnings, in that case ignore the strict
    * setting, otherwise fail the test on warnings as well as errors.
    */
   if (error_count > 0)
   {
      /* We don't really expect to get here because of the setjmp handling
       * above, but this is safe.
       */
      //fprintf(STDERR, "\n  %s: %d libpng errors found (%d warnings)",inname, error_count, warning_count);

      if (strict != 0)
         return (1);
   }
   else
   {   /* If there is no write support nothing was written! */
	   if (unsupported_chunks > 0)
	   {
	   }

	   else 
	   {
		   if (warning_count > 0)
		   {
			   if (strict != 0)
				   return (1);
		   }
	   }
   }


   return (0);




  
}

/* Input and output filenames */
static const char *inname = "pngtest.png";
static const char *outname = "pngout.png";


int
main(int argc, char *argv[])
{

	int result = test_one_file("in.png", "out.png");


	return 0;
}

/* Generate a compiler error if there is an old png.h in the search path. */
//typedef png_libpng_version_1_6_37 Your_png_h_is_not_version_1_6_37;
