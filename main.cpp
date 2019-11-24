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
  texture_set["test_image"] = gl::texture::create(gl::size2d(img.width, img.height),
                                                  GL_R32F);

  if (gl::texture::set_content(texture_set["image"],
                               img.buf,
                               img.width,
                               img.height,
                               GL_LUMINANCE,
                               GL_R32F) == false)
    fprintf(stdout, "ERR: could not load image to opengl texture\n");
  else
    fprintf(stdout, "SUC: uploaded image to opengl texture\n");
}

void log(const char *str, ...)
{
  fprintf(stderr, "%s\n", str);
}

/*
 copy the gl texture data to a buffer
 FIXME: try the new c++ slice function:
        http://www.cplusplus.com/reference/valarray/slice/
*/
bool tex_to_buf(const int w,
                const int h,
                const int d,
                const unsigned char* img_buf,
                unsigned char* out_buf)
{
  // rgba data is returned so unpack into the output
  for (auto y=0u; y<h; y++)
  {
    for (auto x=0u; x<w; x++)
    {
      out_buf[(y*w)+x] = img_buf[(y*w*d)+(x*d)+0];
    }
  }

  return true;
}

void radon_transform(gl::texture::texture_set_t& textures,
                     gl::framebuffer::framebuffer_set_t& framebuffers)
{
  log("radon_transform");

  gl::texture::bind(textures["image"]);
  auto w = gl::texture::width();
  auto h = gl::texture::height();

  textures["radon"] = gl::texture::create(gl::size2d(w, h));
  framebuffers["radon"] = gl::framebuffer::create();

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

  auto tex_buf_f = new float[w*h];
  auto row_sum_f = new float[w];
  auto out_buf_f = new float[w*h];
  float mn = 1.0f, mx = 0.0f;

  std::fill(out_buf_f, out_buf_f+(w*h), 0.0f);

  for (auto i=0u; i<h;i++)
  {
    gl::framebuffer::bind(framebuffers["radon"]);
    gl::framebuffer::attach_texture(textures["radon"]);

    if (gl::framebuffer::verify() == false)
    {
      fprintf(stdout, "ERR: non-verify fbo\n");
      return;
    }

    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ONE);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textures["image"]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glClear(GL_COLOR_BUFFER_BIT);

    float yfrac = ((float)i)/((float)h);

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glTranslatef(0.5f, 0.5f, 0.0f);
    glRotatef(yfrac*360.0f, 0, 0, 1);
    glTranslatef(-0.5f, -0.5f, 0.0f);
    glMatrixMode(GL_MODELVIEW);

    fprintf(stdout, "%i, y: %0.5f\n", i, yfrac);

    glBegin(GL_QUADS);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glTexCoord2f(0,1); glVertex2f(-1.0f, -1.0f);
    glTexCoord2f(0,0); glVertex2f(-1.0f,  1.0f);
    glTexCoord2f(1,0); glVertex2f( 1.0f,  1.0f);
    glTexCoord2f(1,1); glVertex2f( 1.0f, -1.0f);
    glEnd();

    gl::framebuffer::unbind();

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);

    // copy the texture ata out
    gl::texture::bind(textures["radon"]);
    glGetTexImage (GL_TEXTURE_2D, 0, GL_LUMINANCE, GL_FLOAT, tex_buf_f);

    mn = *std::min_element(tex_buf_f, tex_buf_f+(w*h));
    mx = *std::max_element(tex_buf_f, tex_buf_f+(w*h));
    mn = 1.0f;
    mx = 0.0f;

    // sum the texture along the axis
    std::fill(row_sum_f, row_sum_f+w, 0.0f);

    for (auto y=0u; y<h; y++)
    {
      for (auto x=0u; x<w; x++)
      {
        row_sum_f[x] += tex_buf_f[(y*w)+x] / (float)h;
      }

      mn = std::min(mn, *std::min_element(row_sum_f, row_sum_f+w));
      mx = std::max(mx, *std::max_element(row_sum_f, row_sum_f+w));
    }

    // write into a temporary array
    std::copy(row_sum_f, row_sum_f+w, &out_buf_f[(i*w)]);
  }

  // copy the radon transform
  glTexImage2D(GL_TEXTURE_2D,
               0,
               GL_R32F,
               w,
               h,
               0,
               GL_LUMINANCE,
               GL_FLOAT,
               out_buf_f);

  delete[] tex_buf_f;
  delete[] out_buf_f;
  delete[] row_sum_f;
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
  else
  {
    fprintf(stdout, "require an image in -i flag\n");
    return 1;
  }

  // do a radon transform
  radon_transform(texture_set, framebuffer_set);

  // draw the images
  for (auto i=0u; i<1000u; i++)
  {
    //fprintf(stdout, "test iter %i\n", i);

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
    glDisable(GL_TEXTURE_2D);

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_TEXTURE_2D);
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
