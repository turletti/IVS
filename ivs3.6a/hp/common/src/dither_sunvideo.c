/* 	@(#)dither.c	1.3 94/01/26  */
/* get all the values necessary to do a yuv_to_rgb ordered_dither.
   This can either be the XIL standard version, or some tweaked ones
   that we like better */

#include <xil/xil.h>
#include <math.h>

int dither_version = 1;
double dither_gamma = 2.0;

void get_std_dither_info(XilSystemState state,
			 float*rescale_values,
			 float* offset_values,
			 XilDitherMask* mask,
			 XilLookup* yuv_to_rgb,
			 XilLookup* colorcube)
{
    rescale_values[0] = 255.0 / (235.0 - 16.0);
    rescale_values[1] = 255.0 / (240.0 - 16.0);
    rescale_values[2] = 255.0 / (240.0 - 16.0);
    offset_values[0] = -16.0 * rescale_values[0];
    offset_values[1] = -16.0 * rescale_values[1];
    offset_values[2] = -16.0 * rescale_values[2];

    /* get a standard dither mask */
    *mask = xil_dithermask_get_by_name(state, "dm443");
    /*
     * Get a standard lookup which is an 8x5x5 colorcube whose values have
     * been colorspace-transformed from YUV to RGB, install it into the X
     * colormap, and adjust the lookup offset based on the starting index
     * of the allocated colors in the X colormap.
     */ 
    *yuv_to_rgb = xil_lookup_get_by_name(state, "yuv_to_rgb");

    /*
     * Get a standard 8x5x5 colorcube to use with xil_ordered_dither,
     * and copy the lookup offset from yuv_to_rgb to this colorcube.
     */
    *colorcube  = xil_lookup_get_by_name(state, "cc855");
    xil_lookup_set_offset(*colorcube, xil_lookup_get_offset(*yuv_to_rgb));

}


			 
void get_dither_info(XilSystemState state,
		     float*rescale_values,
		     float* offset_values,
		     XilDitherMask* mask,
		     XilLookup* yuv_to_rgb,
		     XilLookup* colorcube)
{
    int min[3];
    int max[3];
    int i;

    if (dither_version == 0) {
	get_std_dither_info(state, rescale_values, offset_values, mask,
			    yuv_to_rgb, colorcube);
	return;
    }
    

    /* get a standard dither mask */
    *mask = xil_dithermask_get_by_name(state, "dm443");
    /*
     * Get a standard 8x5x5 colorcube to use with xil_ordered_dither,
     * and copy the lookup offset from yuv_to_rgb to this colorcube.
     */
    *colorcube  = xil_lookup_get_by_name(state, "cc855");

    /* experimentally derived numbers that ignore highest saturation values
     *  that are unlikely to occur in order to get a higher density of displayable
     *  values in the range that is more likely to occur
     */
    min[0] = 27;    	max[0] = 200;
    min[1] = 72;	max[1] = 175;
    min[2] = 78;    	max[2] = 168;

    for (i=0; i < 3; i++) {
	rescale_values[i] = 255.0 / (float)(max[i] - min[i]);
	offset_values[i] = -min[i] * rescale_values[i];
    }

    /*
     * colorconvert the colorcube into a yuv_to_rgb lookup that can be
     * installed as the colormap and used to do the yuv_to_rgb conversion.
     */
    {
    XilColorspace src_cspace = xil_colorspace_get_by_name(state, 
					"ycc601");
    XilColorspace dst_cspace = xil_colorspace_get_by_name(state, 
					"rgb709");
    unsigned int count, buf_size, i;
    unsigned int table_offset;
    byte* buffer;
    XilImage image_src;
    XilImage image_dst;
    byte* data_ptr;
    XilMemoryStorage storage;
    float scale[3];
    float offset[3];
    double gamma;
    
    *yuv_to_rgb = xil_lookup_create_copy (*colorcube);

    for (i=0; i < 3; i++) {
	scale[i] = (float)(max[i] - min[i])/255.0;
	offset[i] = (float)min[i];
    }

    count = xil_lookup_get_num_entries(*colorcube);
    buf_size = 3 * count * sizeof(Xil_unsigned8);
 
    /* allocate the values buffer */
    buffer = (byte *) malloc (buf_size);
 
    /* Copy the current values from the look-up table into the buffer*/
    table_offset =  xil_lookup_get_offset ( *colorcube );
    xil_lookup_get_values (*colorcube, table_offset, count, buffer);
 
    /*
     * Now create 2 images with
     *    width == number of table entries
     *    height == 1
     *    datatype == Xil_unsigned8
     *    nbands == 3
     * Set the colorspace attributes of the images.
     */
    image_src = xil_create ( state,  count, (unsigned int) 1,
			     (unsigned int) 3, XIL_BYTE);
    image_dst = xil_create ( state,  count, (unsigned int) 1,
			     (unsigned int) 3, XIL_BYTE);
    xil_set_colorspace (image_src, src_cspace);
    xil_set_colorspace (image_dst, dst_cspace);
 

    /*
     * copy the lookup table data into the image.
     */
    xil_export(image_src);                
    xil_get_memory_storage(image_src, &storage);
    data_ptr = (byte*)storage.byte.data;

    /* 
     * gamma correct the y values
     */
    gamma = 1.0/dither_gamma;
    for (i = 0; i < buf_size; i+= 3) {
	double tmp = ((double) buffer[i]) / 255.0;
	tmp = pow(tmp,gamma) * 255.0;
	if (tmp > 255)
	    buffer[i] = 255;
	else
	    buffer[i] = (byte)tmp;
    }
              
    for (i=0; i < buf_size; i+=3) {
	data_ptr[0] = buffer[i];
	data_ptr[1] = buffer[i+1];
	data_ptr[2] = buffer[i+2];
	data_ptr += storage.byte.pixel_stride;
    }
    xil_import(image_src,TRUE);

    xil_rescale(image_src, image_src, scale, offset);
    xil_color_convert(image_src, image_dst);

    xil_export(image_dst);
    xil_get_memory_storage(image_dst, &storage);
    data_ptr = (byte*)storage.byte.data;
    for (i=0; i < buf_size; i+=3) {
	buffer[i] =  data_ptr[0];
	buffer[i+1] = data_ptr[1];
	buffer[i+2] = data_ptr[2];
	data_ptr += storage.byte.pixel_stride;
    }
    xil_lookup_set_values (*yuv_to_rgb, table_offset, count, buffer);
              
    xil_destroy(image_src);
    xil_destroy(image_dst);
    free ( buffer );
              
    }
}
