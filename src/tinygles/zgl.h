#ifndef _tgl_zgl_h_
#define _tgl_zgl_h_

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GL/gl.h>
#include "zbuffer.h"
#include "zmath.h"
#include "util/mat4.h"

#ifndef MIN
#define MIN(a, b) (((a) < (b) ? (a) : (b)))
#endif

#ifndef MAX
#define MAX(a, b) (((a) > (b) ? (a) : (b)))
#endif

/* initially # of allocated GLVertexes (will grow when necessary) */
#define POLYGON_MAX_VERTEX 16

/* Max # of specular light pow buffers */
#define MAX_SPECULAR_BUFFERS 8
/* # of entries in specular buffer */
#define SPECULAR_BUFFER_SIZE 1024
/* specular buffer granularity */
#define SPECULAR_BUFFER_RESOLUTION 1024


#define MAX_MODELVIEW_STACK_DEPTH  64
#define MAX_PROJECTION_STACK_DEPTH 32
#define MAX_TEXTURE_STACK_DEPTH    32

#define MAX_NAME_STACK_DEPTH       64
#define MAX_TEXTURE_LEVELS         11
#define MAX_LIGHTS                 16

#define VERTEX_HASH_SIZE 1031

#define OP_BUFFER_MAX_SIZE 512

#define TGL_OFFSET_FILL    0x1
#define TGL_OFFSET_LINE    0x2
#define TGL_OFFSET_POINT   0x4

#define TGL_PIXEL_ENUM GL_UNSIGNED_BYTE
#define TGL_PIXEL_TYPE GLuint

typedef struct GLSpecBuf {
    int shininess_i;
    int last_used;
    float buf[SPECULAR_BUFFER_SIZE+1];
    struct GLSpecBuf *next;
} GLSpecBuf;

typedef struct GLLight {
    V4 ambient;
    V4 diffuse;
    V4 specular;
    V4 position;
    V3 spot_direction;
    float spot_exponent;
    float spot_cutoff;
    float attenuation[3];
    /* precomputed values */
    float cos_spot_cutoff;
    V3 norm_spot_direction;
    V3 norm_position;
    /* we use a linked list to know which are the enabled lights */
    int enabled;
    struct GLLight *next,*prev;
} GLLight;

typedef struct GLMaterial {
    V4 emission;
    V4 ambient;
    V4 diffuse;
    V4 specular;
    float shininess;

    /* computed values */
    int shininess_i;
    int do_specular;
} GLMaterial;

typedef struct GLViewport {
    int xmin, ymin, xsize, ysize;
    V3 scale;
    V3 trans;
    int updated;
} GLViewport;

typedef union {
    int op;
    float f;
    int i;
    unsigned int ui;
    void *p;
} GLParam;

typedef struct GLParamBuffer {
    GLParam ops[OP_BUFFER_MAX_SIZE];
    struct GLParamBuffer *next;
} GLParamBuffer;

typedef struct GLVertex {
    int edge_flag;
    V3 normal;
    V4 coord;
    V4 tex_coord;
    V4 color;

    /* computed values */
    V4 ec;                /* eye coordinates */
    V4 pc;                /* coordinates in the normalized volume */
    int clip_code;        /* clip code */
    ZBufferPoint zp;      /* integer coordinates for the rasterization */
} GLVertex;

typedef struct GLImage {
    void *pixmap;
    int xsize, ysize;
} GLImage;

/* textures */

#define TEXTURE_HASH_TABLE_SIZE 256

typedef struct GLTexture {
    GLImage images[MAX_TEXTURE_LEVELS];
    int handle;
    struct GLTexture *next,*prev;
} GLTexture;


/* shared state */

typedef struct GLSharedState {
    GLTexture **texture_hash_table;
} GLSharedState;

typedef struct {
    float x, y, z;
} GLRasterPos;

typedef struct {
    const float *p;
    int size;
    int stride;
} GLArray;

struct GLContext;

typedef void (*gl_draw_triangle_func)(struct GLContext *c, GLVertex *p0, GLVertex *p1, GLVertex *p2);

/* display context */
typedef struct GLContext {
    /* Z buffer */
    ZBuffer *zb;

    /* shared state */
    GLSharedState shared_state;

    /* viewport */
    GLViewport viewport;

    /* lights */
    struct {
        GLLight lights[MAX_LIGHTS];
        GLLight *first;
        struct {
            V4 ambient;
            int local;
            int two_side;
        } model;
        int enabled;
    } light;

    /* materials */
    struct {
        GLMaterial materials[2];
        struct {
            int enabled;
            int current_mode;
            int current_type;
        } color;
    } material;

    /* textures */
    struct {
        GLTexture *current;
        int enabled_2d;
    } texture;

    /* current list */
    GLParamBuffer *current_op_buffer;
    int current_op_buffer_index;
    int exec_flag, compile_flag, print_flag;

    /* matrix */
    struct {
        mat4 model_view, projection, texture;
        mat4 model_view_inv, model_projection;
        int model_projection_updated;
        int apply_texture;
    } matrix;

    /* current state */
    int polygon_mode_back;
    int polygon_mode_front;

    int current_front_face;
    int current_shade_model;
    int current_cull_face;
    int cull_face_enabled;
    int normalize_enabled;
    gl_draw_triangle_func draw_triangle_front, draw_triangle_back;

    /* clear */
    struct {
        float depth;
        V4 color;
    } clear;

    /* glBegin / glEnd */
    int in_begin;
    int begin_type;
    int vertex_n, vertex_cnt;
    int vertex_max;
    GLVertex *vertex;

    /* current vertex state */
    struct {
        V4 color;
        unsigned int longcolor[3]; /* precomputed integer color */
        V4 normal;
        V4 tex_coord;
        int edge_flag;
    } current;

    /* opengl 1.1 polygon offset */
    struct {
        float factor;
        float units;
        int states;
    } offset;

    /* specular buffer. could probably be shared between contexts,
       but that wouldn't be 100% thread safe */
    GLSpecBuf *specbuf_first;
    int specbuf_used_counter;
    int specbuf_num_buffers;

    /* opaque structure for user's use */
    void *opaque;

    /* resize viewport function */
    int (*gl_resize_viewport)(struct GLContext *c, int *xsize, int *ysize);

    /* depth test */
    int depth_test;

    struct {
        int dfactor;
        int sfactor;
        int enabled;
    } blend;

    struct {
        int func;
        int ref;
    } alpha;

    struct {
        int op;
    } logic;

    // TODO: glPushAttrib
    GLRasterPos raster_pos;
} GLContext;

extern GLContext *gl_ctx;

/* vertex.c */
void tglNormal3f(GLfloat x, GLfloat y, GLfloat z);
void tglTexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q);
void tglEdgeFlag(GLboolean flag);
void tglColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
void tglBegin(GLenum type);
void tglVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void tglEnd();

/* matrix.c */
void tglMatrixMode(GLenum mode);
void tglLoadMatrixf(const GLfloat *matrix);
void tglLoadIdentity();
void tglMultMatrixf(const GLfloat *matrix);
void tglPushMatrix();
void tglPopMatrix();
void tglRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void tglScalef(GLfloat x, GLfloat y, GLfloat z);
void tglTranslatef(GLfloat x, GLfloat y, GLfloat z);
void tglFrustumf(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near, GLfloat far);
void tglOrthof(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near, GLfloat far);

/* light.c */
void tglMaterialf(GLenum face, GLenum pname, GLfloat param);
void tglMaterialfv(GLenum face, GLenum pname, const GLfloat *v);
void tglColorMaterial(GLenum face, GLenum mode);
void tglLightf(GLenum light, GLenum pname, GLfloat param);
void tglLightfv(GLenum light, GLenum pname, const GLfloat *param);
void tglLightModeli(GLenum pname, GLint param);
void tglLightModelfv(GLenum pname, const GLfloat *param);

/* texture.c */
void tglInitTextures(GLContext *c);
void tglGenTextures(int n, unsigned int *textures);
void tglDeleteTextures(GLsizei n, const GLuint *textures);
void tglBindTexture(GLenum target, GLuint texture);
void tglTexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
void tglTexEnvf(GLenum target, GLenum pname, GLfloat param);
void tglTexEnvi(GLenum target, GLenum pname, GLint param);
void tglTexParameteri(GLenum target, GLenum pname, GLint param);
void tglPixelStorei(GLenum pname, GLint param);

/* clip.c */
void gl_transform_to_viewport(GLContext *c, GLVertex *v);
void gl_draw_triangle(GLContext *c, GLVertex *p0, GLVertex *p1, GLVertex *p2);
void gl_draw_line(GLContext *c, GLVertex *p0, GLVertex *p1);
void gl_draw_point(GLContext *c, GLVertex *p0);

void gl_draw_triangle_point(GLContext *c, GLVertex *p0, GLVertex *p1, GLVertex *p2);
void gl_draw_triangle_line(GLContext *c, GLVertex *p0, GLVertex *p1, GLVertex *p2);
void gl_draw_triangle_fill(GLContext *c, GLVertex *p0, GLVertex *p1, GLVertex *p2);

/* matrix.c */
void tgl_matrix_set(int mode, mat4 *m);

/* light.c */
void gl_shade_vertex(GLContext *c, GLVertex *v);

void tglInitTextures(GLContext *c);
void tglEndTextures(GLContext *c);
GLTexture *alloc_texture(GLContext *c, int h);

/* image_util.c */
void gl_convertRGB_to_5R6G5B(unsigned short *pixmap, unsigned char *rgb, int xsize, int ysize);
void gl_convertRGB_to_8A8R8G8B(unsigned int *pixmap, unsigned char *rgb, int xsize, int ysize);
void gl_resizeImage(unsigned char *dest, int xsize_dest, int ysize_dest, unsigned char *src, int xsize_src, int ysize_src);
void gl_resizeImageNoInterpolate(unsigned char *dest, int xsize_dest, int ysize_dest, unsigned char *src, int xsize_src, int ysize_src);

/* misc.c */
void tglViewport(GLint x, GLint y, GLint width, GLint height);

GLContext *gl_get_context(void);

void tglClose(void);
void tglInit(void *zbuffer1);

/* specular buffer "api" */
GLSpecBuf *specbuf_get_buffer(GLContext *c, const int shininess_i, const float shininess);

/* this clip epsilon is needed to avoid some rounding errors after
   several clipping stages */

#define CLIP_EPSILON (1E-5)

static inline int gl_clipcode(float x, float y, float z, float w1) {
    float w;

    w = w1 * (1.0 + CLIP_EPSILON);
    return (x < -w)     |
        ((x > w)  << 1) |
        ((y < -w) << 2) |
        ((y > w)  << 3) |
        ((z < -w) << 4) |
        ((z > w)  << 5);
}

#endif /* _tgl_zgl_h_ */
