/*
   Kevin Wilhoit
   CpSc 404 
   9/25/2014

   Assignment #3

   This program takes in two input images, and computes the over operation on them, causing
   the first image to be set on top of the second image. The image is able to be viewed and
   resized, and can be written to disk using a filename that can be passed in alongside the
   first two images. 
*/

#include <OpenImageIO/imageio.h>
#include <cstdlib>
#include <iostream>
#include <string>
#include <iomanip>

#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif

unsigned int WIDTH = 640;
unsigned int HEIGHT = 480;

using namespace std;
OIIO_NAMESPACE_USING

string infilename = "", infilename2 = "", outfilename = "";
int channels, channels2;
unsigned char *pixmap;
unsigned char *pixmap2;
unsigned char *pixmap_y;
unsigned char *pixmap_t;
/*
  Routine to write the current framebuffer to an image file
*/
void writeimage(){
  
  ImageOutput *out = ImageOutput::create(outfilename);
  if(!out){std:cerr << "Could not create: " << geterror() << endl; exit(-1);}

  int w = glutGet(GLUT_WINDOW_WIDTH);
  int h = glutGet(GLUT_WINDOW_HEIGHT);
  pixmap_t = new unsigned char[w*h*channels];

  if(channels == 3)glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixmap_t);
  else if(channels == 4)glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixmap_t);
  ImageSpec image(w, h, channels, TypeDesc::UINT8);

  if(outfilename == ""){
    cout << "Enter output image filename: ";
    cin >> outfilename;
  }
  out->open(outfilename, image);

  int scanlinesize = w * channels * sizeof(pixmap_t[0]);
  out->write_image (TypeDesc::UINT8,
		(char *)pixmap_t+(h-1)*scanlinesize, 		// offset to last
		AutoStride,					// default x stride
		-scanlinesize,					// special y stride
		AutoStride);					// default z stride

  out->close();
  delete out;
}

/*
  Display Callback Routine to write a read image to the display.
*/
void displayimage(){
  
  glRasterPos2i(0, 0);

  if (channels == 3) glDrawPixels(WIDTH, HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, pixmap);
  else glDrawPixels(WIDTH, HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, pixmap);
  
  glutSwapBuffers();
}

/*
  Routine to read a passed in file to the framebuffer
*/
void readimage(){

  //this reads in the background image
  ImageInput *in = ImageInput::open(infilename2);
  if (!in){ std::cerr << "Could not create: " << geterror() << endl; exit(-1);}

  const ImageSpec &image = in->spec();

  WIDTH = image.width;
  HEIGHT = image.height;
  channels = image.nchannels;
  pixmap_y = new unsigned char[WIDTH*HEIGHT*channels];

  int scanlinesize = WIDTH * channels * sizeof(pixmap_y[0]);
  in->read_image (TypeDesc::UINT8,
		(char *)pixmap_y+(HEIGHT-1)*scanlinesize,	// offset to last
		AutoStride,                       		// default x stride
		-scanlinesize,  				// special y stride
		AutoStride);    				// default z stride

  in->close();
  delete in;

  //this reads in the foreground image
  ImageInput *in2 = ImageInput::open(infilename);
  if (!in2){ std::cerr << "Could not create: " << geterror() << endl; exit(-1);}

  const ImageSpec &image2 = in2->spec();

  channels2 = image2.nchannels;
  pixmap2 = new unsigned char[WIDTH*HEIGHT*channels2];

  int scanlinesize2 = WIDTH * channels2 * sizeof(pixmap2[0]);
  in2->read_image (TypeDesc::UINT8,
		(char *)pixmap2+(HEIGHT-1)*scanlinesize2,	// offset to last
		AutoStride,                       		// default x stride
		-scanlinesize2,  				// special y stride
		AutoStride);    				// default z stride

  in2->close();
  delete in2;

  // this checks to see if the background image is in 4 channels, and converts
  // the image if necessary. 
  if(channels == 3) {
    channels++;
    pixmap = new unsigned char[WIDTH*HEIGHT*channels];
    int j = 0;
    for (int i = 0; i < WIDTH*HEIGHT*channels; i = i + 4) {
      pixmap[i]   = pixmap_y[j];
      pixmap[i+1] = pixmap_y[j+1];
      pixmap[i+2] = pixmap_y[j+2];
      pixmap[i+3] = 255;
      j = j + 3;
    }
  }
  else if(channels == 4){
    pixmap = new unsigned char[WIDTH*HEIGHT*channels];
    pixmap = pixmap_y;
  }

  // Compositing Image 1 over Image 2 using CP = aA*CA + (1-aA)*aB*CB
  for (int i = 0; i < WIDTH*HEIGHT*channels; i = i + 4) {

    double a_alpha = (double)pixmap2[i+3]/255.0;
    double b_alpha = (double)pixmap[i+3]/255.0;

    // Color R composite
    pixmap2[i] = (a_alpha * pixmap2[i]) + ((1.0 - a_alpha) * b_alpha * pixmap[i]);
    if(pixmap2[i] > 255) pixmap2[i] = 255; if(pixmap2[i] < 0) pixmap2[i] = 0;

    // Color G composite
    pixmap2[i+1] = (a_alpha * pixmap2[i+1]) + ((1.0 - a_alpha) * b_alpha * pixmap[i+1]);
    if(pixmap2[i+1] > 255) pixmap2[i+1] = 255; if(pixmap2[i+1] < 0) pixmap2[i+1] = 0;

    // Color B composite
    pixmap2[i+2] = (a_alpha * pixmap2[i+2]) + ((1.0 - a_alpha) * b_alpha * pixmap[i+2]);
    if(pixmap2[i+2] > 255) pixmap2[i+2] = 255; if(pixmap2[i+2] < 0) pixmap2[i+2] = 0;

    // Color A composite
    pixmap2[i+3] = (a_alpha + ((1.0 - a_alpha) * b_alpha)) * 255;
    if(pixmap2[i+3] > 255) pixmap2[i+3] = 255; if(pixmap2[i+3] < 0) pixmap2[i+3] = 0;

  }
  
  pixmap = pixmap2;

  glutReshapeWindow(WIDTH, HEIGHT);
  
  displayimage();
  
  return;
}

/*
  Keyboard Callback Routine: 'w' writes the image buffer to a file, 'q' or ESC quits
  This routine is called every time a key is pressed on the keyboard
*/
void handleKey(unsigned char key, int x, int y){
  
  switch(key){

    case 'w':		// w - writes image to disk
    case 'W':
      writeimage();
      break;
      
    case 'q':		// q - quit
    case 'Q':
    case 27:		// esc - quit
      exit(0);
      
    default:		// not a valid key -- just ignore it
      return;
  }
}

/*
 Reshape Callback Routine: sets up the viewport and drawing coordinates
 This routine is called when the window is created and every time the window 
 is resized, by the program or by the user
*/
void handleReshape(int w, int h){

// This handles reshaping the window if the window has a smaller width and height 
// than the displayed image.
  if(w < WIDTH && h < HEIGHT){
    glViewport(0, 0, w, h);
    glPixelZoom(((float)w/WIDTH), ((float)h/HEIGHT));
  }
// This handles reshaping the window if the window has a smaller width than
// the displayed image.
  else if(w < WIDTH){   
    glViewport(0, (h-HEIGHT)/2, w, HEIGHT);
    glPixelZoom(((float)w/WIDTH), 1.0);
  }
// This handles reshaping the window if the window has a smaller height than
// the displayed image.
  else if(h < HEIGHT){
    glViewport((w-WIDTH)/2, 0, WIDTH, h);
    glPixelZoom(1.0, ((float)h/HEIGHT));
  }
// This handles reshaping of the window if the window has a larger width and
// height than the displayed image.
  else {
    glViewport((w-WIDTH)/2, (h-HEIGHT)/2, WIDTH, HEIGHT);
    glPixelZoom(1.0, 1.0);
  }
}

/*
   Main program to draw the square, change colors, and wait for quit
*/
int main(int argc, char* argv[]){
  
  // start up the glut utilities
  glutInit(&argc, argv);

  // create the graphics window, giving width, height, and title text
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
  glutInitWindowSize(WIDTH, HEIGHT);
  glutCreateWindow("Image Display");
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, WIDTH, 0, HEIGHT);
  
  // set up the callback routines to be called when glutMainLoop() detects
  // an event
  glutDisplayFunc(displayimage);  // display callback
  glutKeyboardFunc(handleKey);	  // keyboard callback
  glutReshapeFunc(handleReshape); // window resize callback
  
  // specify window clear (background) color to be opaque white
  glClearColor(1, 1, 1, 1);

  if(argc > 4 || argc < 3){cout << "Incorrect use of program start parameters. Correct usage ./composite filename.extension filename.extension (filename.extension)" << endl; exit(0);}
  else if(argc == 4){infilename = argv[1]; infilename2 = argv[2]; outfilename = argv[3]; readimage();}
  else{infilename = argv[1]; infilename2 = argv[2]; readimage();}

  // Routine that loops forever looking for events. It calls the registered 
  // callback routine to handle each event that is detected
  glutMainLoop();
  delete pixmap;
  delete pixmap2;
  delete pixmap_y;
  delete pixmap_t;
  return 0;
}
