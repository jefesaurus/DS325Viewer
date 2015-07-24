#include <GL/glew.h>
#include <GL/gl.h>
#include <assert.h>
#include <map>

#include "shaderloader.h"

class PipelineInput {
protected:
  const int width;
  const int height;

  GLuint texture;
public:
  PipelineInput(int width, int height) : width(width), height(height) {
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  }

  GLuint GetTexture() {
    return texture;
  }
};

// Functions as an input, but allows you to set the input data directly.
class PipelineSource : public PipelineInput {
  const std::string output_name;
public:
  PipelineSource(std::string output_name, int width, int height) : PipelineInput(width, height), output_name(output_name) {}

  void SetData(uint8_t* data) {
    // Send Texture data to GPU
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)data);
  }

  std::string GetOutputName() {
    return output_name;
  }
};

// Functions as an output by also being a renderable framebuffer
class PipelineOutput : public PipelineInput {
  GLuint fbo;

public:
  PipelineOutput(int width, int height) : PipelineInput(width, height) {
    // Same as Input, but we bind an FBO to the texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    //printf("Created FBO: %d from texture %d\n", fbo, texture);
  }
  GLuint GetFramebuffer() {
    return fbo;
  }
};

class PipelineStage {
  GLuint shader_prog;
  const std::string output_name;

  // Input size
  const int width;
  const int height;

  // Input texture
  std::vector<PipelineInput*> inputs;
  std::vector<GLuint> input_unis;

  GLuint data_tex;

  // Position
  static constexpr const char* pos_coord_name = "pass_position";
  GLint pos_attrib;

  // Texture coordinate
  static constexpr const char* tex_coord_name = "pass_texture_coord";
  GLint tex_attrib;

  // Pass the rendering resolution to the fragment shader
  static constexpr const char* resolution_name = "resolution";
  GLint resolution_uni;

  // Input framebuffer
  GLuint fbo;

  PipelineOutput* output;

  // Set up color position attribute
  void InitPosCoordinates() {
    pos_attrib = glGetAttribLocation(shader_prog, pos_coord_name);
    glEnableVertexAttribArray(pos_attrib);
    if (pos_attrib == -1 ) {
      printf("Couldn't find %s in shader: %s\n", pos_coord_name, output_name.c_str());
    }
    glVertexAttribPointer(pos_attrib, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), 0);
  }

  // Set up fragment shader output
  void InitFragOutput() {
    glBindFragDataLocation(shader_prog, 0, "color_out");
  }

  // Pass texture coordinates to shader
  void InitTexCoordinates() {
    tex_attrib = glGetAttribLocation(shader_prog, tex_coord_name);
    if (tex_attrib == -1 ) {
      printf("Couldn't find %s in shader: %s\n", tex_coord_name, output_name.c_str());
    }
    glEnableVertexAttribArray(tex_attrib);
    glVertexAttribPointer(tex_attrib, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(4*sizeof(GLfloat)));
  }

  void InitResolutionUni(int width, int height) {
    resolution_uni = glGetUniformLocation(shader_prog, resolution_name);
    if (resolution_uni == -1) {
      printf("no resolution uniform requested.\n");
    } else {
      glUniform2f(resolution_uni, width, height);
    }
  }

public:
  PipelineStage(std::string output_name, int width, int height, std::string fragment_shader) : output_name(output_name), width(width), height(height) {
    printf("Initializing shader: %s\n", output_name.c_str());
    // Compile Shader
    shader_prog = LoadShader("shaders/basic.vert", fragment_shader.c_str());
    glUseProgram(shader_prog);

    InitPosCoordinates();
    InitFragOutput();
    InitTexCoordinates();
    InitResolutionUni(width, height);

    // Create it's own FBO
    output = new PipelineOutput(width, height);
  }
  ~PipelineStage() {
    delete output; 
  }

  void AddInput(std::string glsl_var_name, PipelineInput* input) {
    inputs.push_back(input);
    input_unis.push_back(glGetUniformLocation(shader_prog, glsl_var_name.c_str()));
  }

  // Just generate output the in the interal FBO
  void BindForOutput() {
    glUseProgram(shader_prog);
    BindInputs();
    glBindFramebuffer(GL_FRAMEBUFFER, output->GetFramebuffer());
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);
  }

  // Use the supplied FBO
  void BindForDisplay(int disp_width, int disp_height, GLuint fbo) {
    glUseProgram(shader_prog);
    glUniform2f(resolution_uni, disp_width, disp_height);
    BindInputs();
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, disp_width, disp_height);
    glClear(GL_COLOR_BUFFER_BIT);
  }

  void BindInputs() {
    int n_textures = inputs.size();
    for (int i = 0; i < n_textures; i++) {
      glActiveTexture(GL_TEXTURE0 + i);
      glBindTexture(GL_TEXTURE_2D, inputs[i]->GetTexture());
      glUniform1i(input_unis[i], i);
    }
  }

  std::vector<std::string> GetInputNames() {
    std::vector<std::string> uniform_names;

    GLint n_uniforms;
    glGetProgramiv(shader_prog, GL_ACTIVE_UNIFORMS, &n_uniforms);

    GLsizei name_length;
    GLint uniform_size;
    GLenum uniform_type;
    GLchar uniform_name[128];

    for (GLuint i = 0; i < (GLuint)n_uniforms; i++) {
      glGetActiveUniform(shader_prog, i, 128, &name_length, &uniform_size, &uniform_type, &uniform_name[0]); 
      if (uniform_type == GL_SAMPLER_1D || uniform_type == GL_SAMPLER_2D || uniform_type == GL_SAMPLER_3D) {
        uniform_names.push_back(std::string(&uniform_name[0], name_length));
      }
    }
    return uniform_names;
  }

  PipelineOutput* GetOutput() {
    return output;
  }

  std::string GetOutputName() {
    return output_name;
  }
};

void LinkStages(std::vector<PipelineSource*> sources, std::vector<PipelineStage*> stages) {
  std::map<std::string, PipelineInput*> input_lookup;
  // Add all of the immediate sources
  for (uint i = 0; i < sources.size(); i++) {
    if (input_lookup.count(sources[i]->GetOutputName()) > 0) {
      printf("Multiple definition of source/output %s\n", sources[i]->GetOutputName().c_str());  
    }
    input_lookup[sources[i]->GetOutputName()] = sources[i];
  }

  // Add outputs of intermediate stages
  for (uint i = 0; i < stages.size(); i++) {
    if (input_lookup.count(stages[i]->GetOutputName()) > 0) {
      printf("Multiple definition of source/output %s\n", stages[i]->GetOutputName().c_str());  
    }
    input_lookup[stages[i]->GetOutputName()] = stages[i]->GetOutput();
  }


  for (uint i = 0; i < stages.size(); i++) {
    std::vector<std::string> input_names = stages[i]->GetInputNames();
    for (uint j = 0; j < input_names.size(); j++) {
      auto iter = input_lookup.find(input_names[j]);
      if (iter == input_lookup.end()) {
        printf("Couldn't find input %s for shader: %s\n", input_names[j].c_str(), stages[i]->GetOutputName().c_str());  
      } else {
        PipelineInput* input = iter->second;
        // This is the line that actually links the output -> input
        stages[i]->AddInput(input_names[j], input);
      }
    }
  }
}


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

  // The input to the whole pipeline.
  std::vector<PipelineSource*> sources; 
  std::vector<PipelineStage*> stages;

public:
  GLCanvas() {
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
  }

  void SetStages(std::vector<PipelineSource*> _sources, std::vector<PipelineStage*> _stages) {
    sources = _sources;
    stages = _stages;
    LinkStages(sources, stages);
  }

  /*
  void AddStage(int width, int height, std::string frag_shader, std::vector<int> inputs, std::vector<std::string> input_var_names) {
    if (stages.size() == 0) {
      // First stage has to take direct input.
      stages.push_back(PipelineStage(width, height, frag_shader));
      stages[0].AddInput("diffuse_texture", source);
    } else {
      stages.push_back(PipelineStage(width, height, frag_shader));
    }
  }

  void AddFirstStage(int width, int height, std::string frag_shader, std::string glsl_input_name) {
    
    stages.push_back(PipelineStage(width, height, frag_shader));
    
  }
  */

  void Render(GLuint framebuffer, int s_width, int s_height) {
    // Bind the reused canvas geometry
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

    uint final_stage = stages.size() - 1;
    for (uint i = 0; i < final_stage; i++) {
      stages[i]->BindForOutput();
      glClear(GL_COLOR_BUFFER_BIT);
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }
    stages[final_stage]->BindForDisplay(s_width, s_height, framebuffer);
  /*

    stages[final_stage].SetRenderResolution(s_width, s_height);
    stages[final_stage].BindOutput();
    glViewport(0, 0, s_width, s_height);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    */
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
  }

  void Render(GLuint framebuffer) {
    Render(framebuffer, width, height);
  }
};
