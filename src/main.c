
#include <stdio.h>
#include <math.h>
#include <string.h>

#include <psp2/kernel/clib.h> 
#include <psp2/kernel/processmgr.h>
#include <psp2/gxm.h>

#include <pib.h>

#define GL_GLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

// ANGLE functions not supported. error: arm-dolce-eabi/bin/ld.exe: main.c:(.text+0x5e1a): undefined reference to `glDrawArraysInstancedANGLE'
// sokol_gfx checks for GL_ANGLE_instanced_arrays first then GL_EXT_draw_instanced so undef it ???
#undef GL_ANGLE_instanced_arrays 
#define SOKOL_IMPL
#define SOKOL_GLES2
#include "sokol_gfx.h"


int main(int args, char *argv[]) {
    /*     
    *   Always initialize PIB before callling any EGL/GLES functions 
    *   Enable the ShaccCg Shader Compiler 'PIB_SHACCCG' and Enabled -nostdlib support 'PIB_NOSTDLIB'(No need if you don't use -nostdlib)
    */
    pibInit(PIB_SHACCCG);

    EGLDisplay egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    EGLint egl_majorVersion;
    EGLint egl_minorVersion;
    eglInitialize(egl_display, &egl_majorVersion, &egl_minorVersion);  

    eglBindAPI(EGL_OPENGL_ES_API);

    EGLint egl_numConfigs = 0;
    EGLConfig egl_config;
    EGLint egl_configAttribs[] = {
        //EGL_CONFIG_ID, 2,                         // You can always provide a configuration id. The one displayed here is Configuration 2
        EGL_RED_SIZE, 8,                            // These four are always 8
        EGL_GREEN_SIZE, 8,                          //
        EGL_BLUE_SIZE, 8,                           //
        EGL_ALPHA_SIZE, 8,                          //
        EGL_DEPTH_SIZE, 32,                         // Depth is either 32 or 0
        EGL_STENCIL_SIZE, 8,                        //  Stencil Size is either 8 or 0
        EGL_SURFACE_TYPE, 5,                        // This is ALWAYS 5
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,    // Always EGL_OPENGL_ES2_BIT or 0x4
        EGL_NONE};

    eglChooseConfig(egl_display, egl_configAttribs, &egl_config, 1, &egl_numConfigs);
    // You can choose your display resoltion, up to 1080p on the PSTV (Vita requires SharpScale)
    EGLSurface egl_surface = eglCreateWindowSurface(egl_display, egl_config, VITA_WINDOW_960X544, NULL);  

    const EGLint egl_contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };

    EGLContext egl_context = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, egl_contextAttribs);

    eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);

    EGLint egl_surface_width = 0, egl_surface_height = 0;
    eglQuerySurface(egl_display, egl_surface, EGL_WIDTH, &egl_surface_width);
    eglQuerySurface(egl_display, egl_surface, EGL_HEIGHT, &egl_surface_height);

    sg_setup(&(sg_desc){0});
    sg_pass_action pass_action = (sg_pass_action) {
        .colors[0] = { .action = SG_ACTION_CLEAR, .val = { 0.50f,0.30f,0.10f, 1.0f } }
    };

    while(1)
    {
        sg_begin_default_pass(&pass_action, egl_surface_width, egl_surface_height);    
        sg_end_pass();
        sg_commit();

        eglSwapBuffers(egl_display, egl_surface);
    }
    
    sg_shutdown();

    eglDestroyContext(egl_display, egl_context); 
    eglDestroySurface(egl_display, egl_surface);
    eglTerminate(egl_display);

    sceKernelExitProcess(0);
    return 0;

}

/* needed for -nostdlib support
void _start(unsigned int args, void *argp)
{
    main(args, argp);
}
*/