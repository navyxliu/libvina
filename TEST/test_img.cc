#include "imgsupport.hpp"
//char * FILE_NAME = "lena_gray.png";
char * FILE_NAME = "heckert_gnu.small.png";
using namespace vina;

int main(int argc, char * argv[])
{
  PngImage lena;
  char * fn;

  printf("libpng version: %s\n", 
	 PNG_LIBPNG_VER_STRING);

  if ( argc <= 1) {
    printf("usage:\ntest_img: [filename]\n");
    fn = FILE_NAME;
  }
  else {
    fn = argv[1];
  }
  
  if ( lena.loadFromFile(fn) ) 
    {
      printf("loaded file %s\n", fn);
      printf("file dimention: width=%4u, height=%4u\n", 
	     lena.getWidth(), lena.getHeight());
      printf("color_type=%2d\n", lena.getColorType());
      printf("gamma %d\n", lena.getGamma());
      printf("channel=%d\n", lena.getChannel());
      //xoprintf("DPI=%d\n", lena.getDpi());
    }
  
  unsigned char * raw = lena.getData();
  int W, H;
  W = lena.getWidth();
  H = lena.getHeight();
  printf("raw data= %p\n", raw);
  for(int i=0; i<1; ++i, putchar('\n'))
    for(int j=0; j<64; ++j) {
      printf("%02x", (unsigned char)raw[i * W +j]);
      if ( ((1+j) % 8) == 0 ) printf("  ");
      if ( ((1+j) % 32) == 0 ) putchar('\n');
    }
 
  PngImage writer;
  char * fn_to_write = "lena.png";
  if (writer.storeToFile(fn_to_write, 256, 256, 8, PNG_COLOR_TYPE_GRAY) ) {
    printf("going to write file %s\n", fn_to_write);
  }
  FILE * f_raw = fopen("lena.raw", "rb");
  if ( f_raw == NULL ) {
    printf("fopen raw file failed");
    exit(1);
  }
  unsigned char * data = (unsigned char *)malloc(256 * 256);
  if ( fread(data, 256, 256, f_raw)
       != 256 ) {
    printf("fread failed\n");
    exit(1);
  }
  
  writer.setData(data);
  printf("finish to write file lena.png\n");
  
}
