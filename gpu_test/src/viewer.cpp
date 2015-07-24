// Generic 3D environment for drawing basic geometry.

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>

#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <stdio.h>
#include "timer.h"
#include "viewer.h"
#include "shaderloader.h"
#include "gl_canvas.h"


// GLUT CALLBACK functions ////////////////////////////////////////////////////
void displayCB();
void reshapeCB(int w, int h);
void timerCB(int millisec);
void idleCB();
void keyboardCB(unsigned char key, int x, int y);
void mouseCB(int button, int stat, int x, int y);
void mouseMotionCB(int x, int y);

// CALLBACK function when exit() called ///////////////////////////////////////
void exitCB();

void initGL();
int  initGLUT(int argc, char **argv);
void clearSharedMem();
void initLights();
void setCamera(float posX, float posY, float posZ, float targetX, float targetY, float targetZ);
void updatePixels(GLubyte* dst, int size);
void drawString(const char *str, int x, int y, float color[4], void *font);
void drawString3D(const char *str, float pos[3], float color[4], void *font);
void DrawGeometry();
void showInfo();
void showTransferRate();
void printTransferRate();

// constants
const int    SCREEN_WIDTH    = 400;
const int    SCREEN_HEIGHT   = 300;
const float  CAMERA_DISTANCE = 3.0f;
const int    TEXT_WIDTH      = 8;
const int    TEXT_HEIGHT     = 13;
const int    IMAGE_WIDTH     = 1024;
const int    IMAGE_HEIGHT    = 1024;
const int    CHANNEL_COUNT   = 4;
const int    DATA_SIZE       = IMAGE_WIDTH * IMAGE_HEIGHT * CHANNEL_COUNT;
const GLenum PIXEL_FORMAT    = GL_BGRA;

// global variables
int screenWidth;
int screenHeight;
bool mouse_left_down, mouse_mid_down, mouse_right_down;
float mouseX, mouseY;
float cam_angle_x, cam_angle_y;
float cam_pos_x, cam_pos_y;
float cam_distance;
Timer timer;
float fps;
float last_draw_time;

void CheckGLError(int id) {
  GLenum error = glGetError();
  if (error != GL_NO_ERROR) {
    printf("%d, Error initializing OpenGL: %s\n", id, gluErrorString(error));
  }
}


///////////////////////////////////////////////////////////////////////////////
GLCanvas* canvas;
void StartWindow() {
  // register exit callback
  atexit(exitCB);

  // init GLUT and GL
  int argc = 1;
  char *argv[1] = {(char*)"Simulator"};
  initGLUT(argc, argv);
  CheckGLError(6);
  initGL();
  CheckGLError(7);


  canvas = new GLCanvas();

  //canvas.AddStage(2, 2, "shaders/basic.frag");
  //canvas.AddStage(2, 2, "shaders/glow.frag");
  //canvas.Init();

  CheckGLError(8);
  uint8_t texture[] = {
    // R, G, B
    0, 0, 255, 0,   255, 0, 0, 255,
    255, 0, 0, 255,   0, 0, 255, 0 
  };
  //canvas.SetData(texture);
  CheckGLError(9);

  // Create the immediate sources to the pipeline
  PipelineSource diffuse_texture("diffuse_texture", 2, 2);
  std::vector<PipelineSource*> sources;
  sources.push_back(&diffuse_texture);

  // Create the intermediate stages
  int upscale = 500;
  PipelineStage blur_x("blur_x", upscale, upscale, "shaders/blur_x.frag");
  PipelineStage blur_y("blur_y", upscale, upscale, "shaders/blur_y.frag");
  PipelineStage glow("glow", upscale, upscale, "shaders/glow.frag");
  //PipelineStage glow("glow", upscale, upscale, "shaders/autoglow.frag");
  // These need to be added in render order
  // (there isn't any smart dependency checking to determine that on the fly)
  std::vector<PipelineStage*> stages;
  stages.push_back(&blur_x);
  stages.push_back(&blur_y);
  stages.push_back(&glow);

  // Link
  canvas->SetStages(sources, stages);

  // Set the initial data
  diffuse_texture.SetData(texture);

  timer.start();
  glutMainLoop();
}

int main(int argc, char** argv) {
  StartWindow();
}


///////////////////////////////////////////////////////////////////////////////
// initialize GLUT for windowing
///////////////////////////////////////////////////////////////////////////////
int initGLUT(int argc, char **argv) {
  // GLUT stuff for windowing
  // initialization openGL window.
  // it is called before any other GLUT routine
  glutInit(&argc, argv);
  glutInitContextVersion (3, 3);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH); // display mode

  glutInitWindowSize(400, 300);               // window size

  glutInitWindowPosition(100, 100);           // window location

  // finally, create a window with openGL context
  // Window will not displayed until glutMainLoop() is called
  // it returns a unique ID
  int handle = glutCreateWindow(argv[0]);     // param is the title of window

  // register GLUT callback functions
  glutDisplayFunc(displayCB);
  //glutTimerFunc(33, timerCB, 33);             // redraw only every given millisec
  glutIdleFunc(idleCB);                       // redraw only every given millisec
  glutReshapeFunc(reshapeCB);
  glutKeyboardFunc(keyboardCB);
  glutMouseFunc(mouseCB);
  glutMotionFunc(mouseMotionCB);

  return handle;
}


///////////////////////////////////////////////////////////////////////////////
// initialize OpenGL
// disable unused features
///////////////////////////////////////////////////////////////////////////////
void initGL() {
  //Initialize GLEW
  glewExperimental = GL_TRUE;
  GLenum glewError = glewInit();
  if( glewError != GLEW_OK ) {
    printf( "Error initializing GLEW! %s\n", glewGetErrorString( glewError ) );
  } else {
    printf("All good I guess..\n");
  }

  //glPixelStorei(GL_UNPACK_ALIGNMENT, 4);      // 4-byte pixel alignment
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glClearColor(0, 0, 0, 0);                   // background color
}


//=============================================================================
// CALLBACKS
//=============================================================================

void displayCB() {
  // clear buffer
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  canvas->Render(0, screenWidth, screenHeight);

  // draw info messages
  float current_time = timer.getElapsedTimeInMicroSec();
  fps = 1000000.0 / (current_time - last_draw_time);
  //printf("%f fps\n", fps);
  last_draw_time = current_time;

  glutSwapBuffers();
  CheckGLError(10);
}

void reshapeCB(int width, int height) {
  screenWidth = width;
  screenHeight = height;
}

void timerCB(int millisec) {
  glutTimerFunc(millisec, timerCB, millisec);
  glutPostRedisplay();
}

void idleCB() {
  glutPostRedisplay();
}

void keyboardCB(unsigned char key, int x, int y) {
  switch(key) {
  case 27: // ESCAPE
    exit(0);
    break;
  }
}


void mouseCB(int button, int state, int x, int y) {
  mouseX = x;
  mouseY = y;

  if(button == GLUT_LEFT_BUTTON) {
    if(state == GLUT_DOWN) {
      mouse_left_down = true;
    } else if(state == GLUT_UP) {
      mouse_left_down = false;
    }
  } else if(button == GLUT_RIGHT_BUTTON) {
    if(state == GLUT_DOWN) {
      mouse_right_down = true;
    } else if(state == GLUT_UP) {
      mouse_right_down = false;
    }
  } else if(button == GLUT_MIDDLE_BUTTON) {
    if(state == GLUT_DOWN) {
      mouse_mid_down = true;
    } else if(state == GLUT_UP) {
      mouse_mid_down = false;
    }
  }
}

void mouseMotionCB(int x, int y) {
  /*
  if(mouse_left_down) {
    cam_angle_y += (x - mouseX)*.5f;
    cam_angle_x += (y - mouseY)*.5f;
    mouseX = x;
    mouseY = y;
  }
  if(mouse_right_down) {
    cam_distance += (y - mouseY) * 0.2f;
    if(cam_distance < 2.0f) {
      cam_distance = 2.0f;
    }

    mouseY = y;
  }

  if (mouse_mid_down) {
    cam_pos_x -= (x - mouseX)/screenWidth*2.0f;
    cam_pos_y += (y - mouseY)/screenHeight*2.0f;
    mouseX = x;
    mouseY = y;
  }
  */
}

void exitCB() {
}
