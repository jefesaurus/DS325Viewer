#include <GL/glew.h>
#include <GL/gl.h>

#include "shaderloader.h"


class PipelineStage {
  GLuint basic_pass;

  // Input size
  int width, height;
  std::string shader_location;


  // Input texture
  GLuint data_tex;

  // Position
  GLuint pos_attrib;
  // Texture coordinate
  GLint tex_attrib;
  // Pass the rendering resolution to the fragment shader
  GLuint resolution_uni;

  // Input framebuffer
  GLuint fbo;

  bool is_initialized;

public:
  PipelineStage(int width, int height, std::string fragment_shader) : width(width), height(height), shader_location(fragment_shader), is_initialized(false) {
  }

  void Init(bool source) {
    basic_pass = LoadShader("shaders/basic.vert", shader_location.c_str());
    glUseProgram(basic_pass);

    // Set up color position variable
    pos_attrib = glGetAttribLocation(basic_pass, "pass_position");
    glEnableVertexAttribArray(pos_attrib);
    glVertexAttribPointer(pos_attrib, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), 0);

    // Set up fragment shader output
    glBindFragDataLocation(basic_pass, 0, "color_out");

    // Init texture
    glGenTextures(1, &data_tex);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, data_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    // Reserve space on GPU
    if (!source) {
      glGenFramebuffers(1, &fbo);
      glBindFramebuffer(GL_FRAMEBUFFER, fbo);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, data_tex, 0);
    }

    // Pass texture to shader
    glUniform1i(glGetUniformLocation(basic_pass, "diffuse_texture"), 0);

    // Pass texture coordinates to shader
    tex_attrib = glGetAttribLocation(basic_pass, "pass_texture_coord");
    glEnableVertexAttribArray(tex_attrib);
    glVertexAttribPointer(tex_attrib, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(4*sizeof(GLfloat)));

    resolution_uni = glGetUniformLocation(basic_pass, "resolution");
    is_initialized = true;
  }

  void BindOutput() {
    glUseProgram(basic_pass);
  }

  void BindInput() {
    glViewport(0, 0, width, height);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glClear(GL_COLOR_BUFFER_BIT);
  }

  void SetRenderResolution(int s_width, int s_height) {
    glUseProgram(basic_pass);
    glUniform2f(resolution_uni, s_width, s_height);
  }

  void SetData(uint8_t* data) {
    // Send Texture data to GPU
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, data_tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)data);
  }

  bool IsInitialized() {
    return is_initialized;
  }
};


class GLCanvas {
  int width, height;

  // Vertex array
  GLuint vao;
  // Vertex buffer
  GLuint vbo;
  // Vertex data
  const GLfloat vertices[24] = {
      // X,  Y, Z, tex x, tex y
      -1.0f, 1.0f,  0.0f, 1.0f,  0.0f, 0.0f, // Top-left
      1.0f, 1.0f,   0.0f, 1.0f,  1.0f, 0.0f, // Top-right
      1.0f, -1.0f,  0.0f, 1.0f,  1.0f, 1.0f, // Bottom-right
      -1.0f, -1.0f, 0.0f, 1.0f,  0.0f, 1.0f  // Bottom-left
  };
  // Element buffer
  GLuint ebo;
  const GLuint elements[6] = {
        0, 1, 2,
        2, 3, 0
  };

  std::vector<PipelineStage> stages;

public:
  GLCanvas() {}

  void Init() {
    // Init VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Init VBO
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Init EBO
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);

    if (!stages[0].IsInitialized()) {
      stages[0].Init(true);
    }
    for (uint i = 0; i < stages.size(); i++) {
      if (!stages[i].IsInitialized()) {
        stages[i].Init(false);
      }
    }
  }

  void AddStage(int width, int height, std::string frag_shader) {
    stages.push_back(PipelineStage(width, height, frag_shader));
  }

  void Render(GLuint framebuffer, int s_width, int s_height) {
    // Bind the reused canvas geometry
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);


    uint final_stage = stages.size() - 1;
    for (uint i = 0; i < final_stage; i++) {
      stages[i].BindOutput();
      stages[i+1].BindInput();
      glClear(GL_COLOR_BUFFER_BIT);
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }

    stages[final_stage].SetRenderResolution(s_width, s_height);
    stages[final_stage].BindOutput();
    glViewport(0, 0, s_width, s_height);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
  }

  void Render(GLuint framebuffer) {
    Render(framebuffer, width, height);
  }

  void SetData(uint8_t* data) {
    stages[0].SetData(data);
  }
};
