#include <iostream>

#include "gl/defs.h"
#include "gl/context.h"
#include "gl/texture.h"
#include "gl/framebuffer.h"

#include "img_io/png_io.h"

#define _USE_MATH_DEFINES
#include <math.h>

void load_image(const char *filename, gl::texture::texture_set_t& texture_set)
{
  png_io::img_t img;

  if (png_io::read(filename, img) == false)
  {
    fprintf(stdout,"ERR: unable to read '%s'\n", filename);
    return;
  }

  fprintf(stdout, "SUC: read '%s'\n", filename);
  fprintf(stdout, "INFO: %ix%ix%i@%i\n", img.width,
                                         img.height,
                                         img.channels,
                                         img.bit_depth);

  // upload to a texture
  texture_set["test_image"] = gl::texture::create(gl::size2d(img.width, img.height));

  if (gl::texture::set_content(texture_set["image"],
                               img.buf,
                               img.width,
                               img.height,
                               GL_RGB) == false)
    fprintf(stdout, "ERR: could not load image to opengl texture\n");
  else
    fprintf(stdout, "SUC: uploaded image to opengl texture\n");
}

void radon_transform(gl::texture::texture_set_t& textures,
                     gl::framebuffer::framebuffer_set_t& framebuffers)
{
  gl::texture::bind(textures["image"]);
  auto w = gl::texture::width();
  auto h = gl::texture::height();

  textures["radon"] = gl::texture::create(gl::size2d(w, h));
  framebuffers["radon"] = gl::framebuffer::create();

  //
  gl::framebuffer::bind(framebuffers["radon"]);
  gl::framebuffer::attach_texture(textures["radon"]);

  if (gl::framebuffer::verify() == false)
  {
    fprintf(stdout, "ERR: non-verify fbo\n");
  }

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, textures["image"]);
  glBegin(GL_QUADS);
  glColor4f(1.0f,1.0f,1.0f,1.0f);
  glTexCoord2f(0,0); glVertex2f(-1.0f, -1.0f);
  glTexCoord2f(0,1); glVertex2f(-1.0f,  1.0f);
  glTexCoord2f(1,1); glVertex2f( 1.0f,  1.0f);
  glTexCoord2f(1,0); glVertex2f( 1.0f, -1.0f);
  glEnd();
  glDisable(GL_TEXTURE_2D);

  gl::framebuffer::unbind();
}

int main(int argc, char **argv)
{
  auto ctx = gl::context::create();
  auto texture_set = gl::texture::texture_set_t();
  auto framebuffer_set = gl::framebuffer::framebuffer_set_t();

  if (strcmp(argv[1], "-i") == 0)
  {
    // load an image
    load_image(argv[2], texture_set);
  }

  // do a radon transform
  radon_transform(texture_set, framebuffer_set);

  // draw the images
  for (auto i=0u; i<1000u; i++)
  {
//    fprintf(stdout, "test iter %i\n", i);

    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glTranslatef(0.5f, 0.5f, 0.0f);
    glRotatef( ((float)i/1000.0f)*360.0f, 0, 0, 1);
    glTranslatef(-0.5f, -0.5f, 0.0f);
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture_set["image"]);
    glBegin(GL_QUADS);
    glColor4f(1.0f,1.0f,1.0f,1.0f);
    glTexCoord2f(0,0); glVertex2f(-1.0f, -1.0f);
    glTexCoord2f(0,1); glVertex2f(-1.0f,  1.0f);
    glTexCoord2f(1,1); glVertex2f( 0.0f,  1.0f);
    glTexCoord2f(1,0); glVertex2f( 0.0f, -1.0f);
    glEnd();

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);

    glBindTexture(GL_TEXTURE_2D, texture_set["radon"]);
    glBegin(GL_QUADS);
    glTexCoord2i(0,0); glVertex2f( 0.0f, -1.0f);
    glTexCoord2i(0,1); glVertex2f( 0.0f,  1.0f);
    glTexCoord2i(1,1); glVertex2f( 1.0f,  1.0f);
    glTexCoord2i(1,0); glVertex2f( 1.0f, -1.0f);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    glXSwapBuffers(glXGetCurrentDisplay(),
                   glXGetCurrentDrawable());
  }

  return 0;
}
