#include <iostream>
#include <chrono>

#include "gl/defs.h"
#include "gl/context.h"
#include "gl/texture.h"
#include "gl/framebuffer.h"
#include "gl/shader.h"

#include "img_io/png_io.h"

#define _USE_MATH_DEFINES
#include <math.h>

#define PASS_THROUGH_VERT_SRC_FILENAME "gl/pass-through.vert"
#define YSUM_FRAG_SRC_FILENAME "gl/ysum.frag"

//
// SHADERS
//
bool load_shaders(gl::shader::shader_t& shader)
{
  char *vert_src = NULL;
  char *frag_src = NULL;
  size_t vert_len = 0u;
  size_t frag_len = 0u;

  gl::shader::read_file(PASS_THROUGH_VERT_SRC_FILENAME, NULL, vert_len);
  vert_src = new char[vert_len+1];
  std::fill(vert_src, vert_src+vert_len+1, 0);
  gl::shader::read_file(PASS_THROUGH_VERT_SRC_FILENAME, vert_src, vert_len);

  gl::shader::read_file(YSUM_FRAG_SRC_FILENAME, NULL, frag_len);
  frag_src = new char[frag_len+1];
  std::fill(frag_src, frag_src+frag_len+1, 0);
  gl::shader::read_file(YSUM_FRAG_SRC_FILENAME, frag_src, frag_len);

  gl::shader::definition_t def = {{"vertex", vert_src},
                                  {"fragment", frag_src}};

  gl::shader::create(def, shader);

  delete[] frag_src;
  delete[] vert_src;

  // check the logs
  gl::shader::get_log(shader, "vertex", "vert-shader");
  gl::shader::get_log(shader, "fragment", "frag-shader");
  gl::shader::get_log(shader, "program", "prog-shader");

  // verify the shader
  if (gl::shader::verify(shader) == false)
  {
    std::cerr << "ERR: could not verify shader" << std::endl;
    gl::shader::clean(shader);
    return false;
  }

  std::cerr << "INFO: loaded shaders" << std::endl;
  return true;
}

//
// RADON TRANSFORM
//
void gl_radon_transform(gl::texture::texture_set_t& textures,
                        gl::framebuffer::framebuffer_set_t& framebuffers,
                        gl::shader::shader_t& radon_shader)
{
  gl::texture::bind(textures["image"]);
  auto w = gl::texture::width();
  auto h = gl::texture::height();

  textures["radon"] = gl::texture::create(gl::size2d(w, h), GL_LUMINANCE);
  framebuffers["radon"] = gl::framebuffer::create();

  glViewport(0,0,w,h);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  gl::framebuffer::bind(framebuffers["radon"]);
  gl::framebuffer::attach_texture(textures["radon"]);
  gl::framebuffer::draw_buffers(1);

  if (gl::framebuffer::verify() == false)
  {
    std::cerr << "ERR: non-verify fbo" << std::endl;
    return;
  }

  // render the image to the fbo
  gl::shader::bind(radon_shader);

  glActiveTexture(GL_TEXTURE0);
  gl::texture::bind(textures["image"]);

  glClear(GL_COLOR_BUFFER_BIT);
  glBegin(GL_TRIANGLE_STRIP);
  // FIXME: gl invoke 4 verts
  glVertex2f(-1.0f, -1.0f);
  glVertex2f(-1.0f,  1.0f);
  glVertex2f( 1.0f, -1.0f);
  glVertex2f( 1.0f,  1.0f);
  glEnd();

  gl::shader::unbind();
  gl::framebuffer::unbind();
}

//
// MAIN
//
int main(int argc, char **argv)
{
  auto ctx = gl::context::create();
  auto texture_set = gl::texture::texture_set_t();
  auto framebuffer_set = gl::framebuffer::framebuffer_set_t();
  auto shader_set = gl::shader::shader_t();
  png_io::img_t img = {0};

  //
  // read the image from stdin
  //
  std::cerr << "INFO: reading from stdin" << std::endl;
  std::cin >> img;

  // output the image data
  std::cerr << "INFO: read image " << img.width
                            << "x" << img.height
                            << "x" << img.channels
                            << "@" << img.bit_depth
                                   << std::endl;

  //
  // read the shaders
  //
  if (load_shaders(shader_set) == false)
  {
    std::cerr << "ERR: could not load shaders" << std::endl;
    return 1;
  }

  //
  // upload to a texture
  //
  texture_set["image"] = gl::texture::create(gl::size2d(img.width, img.height), GL_R32F);

  if (gl::texture::set_content(texture_set["image"],
                               img.buf,
                               img.width,
                               img.height,
                               GL_LUMINANCE,
                               GL_R32F) == false)
    std::cerr << "ERR: could not load image to opengl texture" << std::endl;
  else
    std::cerr << "SUC: uploaded image to opengl texture" << std::endl;

  //
  // do a radon transform
  //
  auto T = std::chrono::high_resolution_clock::now();
  gl_radon_transform(texture_set, framebuffer_set, shader_set);
  auto T1 = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - T);
  std::cerr << "transform in " << T1.count() << "ms" << std::endl;

  //
  // write the opengl output image to stdout
  //
  gl::texture::write(texture_set["radon"],
                     [](const int w, const int h, const int d, const unsigned char *buf) -> bool
                     {
                       return png_io::write_to_stream(std::cout,
                                               static_cast<const unsigned int>(w),
                                               static_cast<const unsigned int>(h),
                                               static_cast<const unsigned int>(d),
                                               buf);
                     });

  return 0;
}
