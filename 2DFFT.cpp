#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <png.h>
#include <complex>

using namespace std;

	int width = 256, height = 256;
	png_byte color_type;
	png_byte bit_depth;
	png_bytep *row_pointers;
	complex<double> original_img[256][256], temp_img[256][256], fft_img[256][256];
	
	//To read a png file via libpng
	void read_png_file(char *filename) {
	
		FILE *fp = fopen(filename, "rb");

		png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		if(!png) abort();

		png_infop info = png_create_info_struct(png);
  		if(!info) abort();

  		if(setjmp(png_jmpbuf(png))) abort();

  		png_init_io(png, fp);

  		png_read_info(png, info);

  		width      = png_get_image_width(png, info);
  		height     = png_get_image_height(png, info);
  		color_type = png_get_color_type(png, info);
  		bit_depth  = png_get_bit_depth(png, info);

  

  		if(bit_depth == 16)
    		png_set_strip_16(png);

  		if(color_type == PNG_COLOR_TYPE_PALETTE)
    		png_set_palette_to_rgb(png);

  		if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    		png_set_expand_gray_1_2_4_to_8(png);

  		if(png_get_valid(png, info, PNG_INFO_tRNS))
    		png_set_tRNS_to_alpha(png);

  		if(color_type == PNG_COLOR_TYPE_RGB ||
     		color_type == PNG_COLOR_TYPE_GRAY ||
     		color_type == PNG_COLOR_TYPE_PALETTE)
    		png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

  		if(color_type == PNG_COLOR_TYPE_GRAY ||
     		color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    		png_set_gray_to_rgb(png);

  		png_read_update_info(png, info);

  		row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
  		for(int y = 0; y < height; y++) {
    		row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png,info));
  		}

  		png_read_image(png, row_pointers);

  		fclose(fp);
	}

	//To write a png file via libpng
	void write_png_file(char *filename) {

  		FILE *fp = fopen(filename, "wb");
  		if(!fp) abort();

  		png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  		if (!png) abort();

  		png_infop info = png_create_info_struct(png);
  		if (!info) abort();

  		if (setjmp(png_jmpbuf(png))) abort();

  		png_init_io(png, fp);

  		png_set_IHDR(
    		png,
    		info,
    		width, height,
    		8,
    		PNG_COLOR_TYPE_RGBA,
    		PNG_INTERLACE_NONE,
    		PNG_COMPRESSION_TYPE_DEFAULT,
    		PNG_FILTER_TYPE_DEFAULT
  		);
  		png_write_info(png, info);

  		png_write_image(png, row_pointers);
  		png_write_end(png, NULL);

  		for(int y = 0; y < height; y++) {
    		free(row_pointers[y]);
  		}
  		free(row_pointers);

  		fclose(fp);
	}
	
void separate (complex<double>* a, int n) {
    complex<double>* b = new complex<double>[n/2];  // get temp heap storage
    for(int i=0; i<n/2; i++)    // copy all odd elements to heap storage
        b[i] = a[i*2+1];
    for(int i=0; i<n/2; i++)    // copy all even elements to lower-half of a[]
        a[i] = a[i*2];
    for(int i=0; i<n/2; i++)    // copy all odd (from heap) to upper-half of a[]
        a[i+n/2] = b[i];
    delete[] b;                 // delete heap storage
}


void fft (complex<double>* X, int N) {
    if(N < 2) {
        // bottom of recursion.
        // Do nothing here, because already X[0] = x[0]
    } else {
        separate(X,N);      // all evens to lower half, all odds to upper half
        fft(X,     N/2);   // recurse even items
        fft(X+N/2, N/2);   // recurse odd  items
        // combine results of two half recursions
        for(int k=0; k<N/2; k++) {
            complex<double> e = X[k    ];   // even
            complex<double> o = X[k+N/2];   // odd
                         // w is the "twiddle-factor"
            complex<double> w = exp( complex<double>(0,-2.*M_PI*k/N) );
            X[k    ] = e + w * o;
            X[k+N/2] = e - w * o;
        }
    }
}

int main (int argc, char *argv[]) {

	read_png_file(argv[1]);
		   
    //build original image table
    for(int i=0;i<width;i++){
 			for(int j=0;j<height;j++){
 				png_bytep px = &(row_pointers[j][i * 4]);
       			temp_img[j][i]=original_img[j][i]=px[0];
 			}
 	}
 	
 	//start fft process
 	for(int j=0;j<height;j++){
 		fft(temp_img[j],256);
 	}
    for(int i=0;i<width;i++){
 			for(int j=0;j<height;j++){
       			fft_img[i][j]=temp_img[j][i];
 			}
 	}
 	for(int j=0;j<height;j++){
 		fft(fft_img[j],256);
 	}
	
	//build fft image table
    for(int i=0;i<width;i++){
 			for(int j=0;j<height;j++){
 				png_bytep px = &(row_pointers[((j+128)%256)][((i+128)%256) * 4]);
 				
       			px[0]=px[1]=px[2]=212*log10(abs(fft_img[i][j])/65536+1);
       			//printf("%.3f\n",212*log10(abs(fft_img[i][j])/65536+1));
 			}
 	}
 	
 	
 	write_png_file(argv[2]);
 	
 	
 	//apply low pass filter
 	int filter_factor = 140;
 	for(int i=0;i<width;i++){
 			for(int j=0;j<height;j++){
 				int u = (i+128)%256, v = (j+128)%256;
 				complex<double> attenuate_factor = exp(-(u*u+v*v)/(2*filter_factor*filter_factor));
 				printf("%.6f\n",abs(attenuate_factor));
 				fft_img[i][j]=fft_img[i][j]*attenuate_factor;
 			}
 	}
 	
 	//start ifft process
 	
 	for(int i=0;i<width;i++){
 			for(int j=0;j<height;j++){
 				fft_img[i][j]=conj(fft_img[i][j]);
 			}
 	}
 	
 	for(int j=0;j<height;j++){
 		fft(fft_img[j],256);
 	}
    for(int i=0;i<width;i++){
 			for(int j=0;j<height;j++){
       			temp_img[i][j]=fft_img[j][i];
 			}
 	}
 	for(int j=0;j<height;j++){
 		fft(temp_img[j],256);
 	}
	for(int i=0;i<width;i++){
 			for(int j=0;j<height;j++){
 				temp_img[i][j]=conj(temp_img[i][j]);
 			}
 	}
	//build ifft image table
    for(int i=0;i<width;i++){
 			for(int j=0;j<height;j++){
 				png_bytep px = &(row_pointers[j][i * 4]);
       			px[0]=px[1]=px[2]=(temp_img[j][i].real()/65536);
       			//printf("%.3f\n",(temp_img[j][i].real()/65536));
 			}
 	}

    write_png_file(argv[3]);
}