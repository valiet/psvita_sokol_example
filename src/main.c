
#include <stdio.h>
#include <math.h>
#include <string.h>

#include <psp2/kernel/clib.h> 
#include <psp2/kernel/processmgr.h>
#include <psp2/gxm.h>
#include <psp2/ctrl.h>
#include <psp2/touch.h>
#include <pib.h>

#define SCE_DISPLAY_WIDTH  960
#define SCE_DISPLAY_HEIGHT 544

#define SCE_TOUCH_FRONT_MAX_WITDH 	1919
#define SCE_TOUCH_FRONT_MAX_HEIGHT 	1087
#define SCE_TOUCH_BACK_MAX_WITDH 	1919
#define SCE_TOUCH_BACK_MAX_HEIGHT 	889-108

#define SCE_TOUCH_FRONT_MAX_REPORT 6
#define SCE_TOUCH_BACK_MAX_REPORT 4

#define	SCE_CTRL_PAD_ONE 0
#define	SCE_CTRL_PAD_TWO 1
#define	SCE_CTRL_PAD_THREE 2
#define	SCE_CTRL_PAD_FOUR 3
#define	SCE_CTRL_PAD_MAX_NUM 4

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
#include "sokol_time.h"
#define SOKOL_DEBUGTEXT_IMPL
#include "sokol_debugtext.h"
#define SOKOL_GL_IMPL
#include "sokol_gl.h"

#define math_lerp(value, from_max, to_max) (((((value)*10) * ((to_max)*10))/((from_max)*10))/10)


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

    // gamepad
    SceCtrlData ctrl_pad_one = { 0 };
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);

    // touch
    SceTouchData touch_front = { 0 };
    sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
    //sceTouchEnableTouchForce(SCE_TOUCH_PORT_FRONT);
    //sceTouchSetSamplingState(SCE_TOUCH_PORT_BACK, SCE_TOUCH_SAMPLING_STATE_START);
    //sceTouchEnableTouchForce(SCE_TOUCH_PORT_BACK);

	const char* ctrl_btn_label[]={"SELECT ","","","START ",
		"UP ","RIGHT ","DOWN ","LEFT ","L ","R ","","",
		"TRIANGLE ","CIRCLE ","CROSS ","SQUARE "};

    stm_setup();
    sg_setup(&(sg_desc){0});
    sdtx_setup(&(sdtx_desc_t){.fonts = {
        [0] = sdtx_font_kc853()
    }});
    sgl_setup(&(sgl_desc_t){.max_vertices = 512, .max_commands = 128, .pipeline_pool_size = 32});

    sg_pass_action pass_action = (sg_pass_action) {
        .colors[0] = { .action = SG_ACTION_CLEAR, .val = { 0.10f,0.30f,0.50f, 1.0f } }
    };
    uint64_t last_time = 0;
    uint64_t min_raw_frame_time = 0xFFFFFFFFFFFFFFFF;
    uint64_t max_raw_frame_time = 0;
    uint64_t min_rounded_frame_time = 0xFFFFFFFFFFFFFFFF;
    uint64_t max_rounded_frame_time = 0;
    float rot[2] = { 0.0f, 0.0f };
    while(1)
    {
        uint64_t raw_frame_time = stm_laptime(&last_time);
        uint64_t rounded_frame_time = stm_round_to_common_refresh_rate(raw_frame_time);

        if (raw_frame_time > 0) {
            if (raw_frame_time < min_raw_frame_time) {
                min_raw_frame_time = raw_frame_time;
            }
            if (raw_frame_time > max_raw_frame_time) {
                max_raw_frame_time = raw_frame_time;
            }
        }
        if (rounded_frame_time > 0) {
            if (rounded_frame_time < min_rounded_frame_time) {
                min_rounded_frame_time = rounded_frame_time;
            }
            if (rounded_frame_time > max_rounded_frame_time) {
                max_rounded_frame_time = rounded_frame_time;
            }
        }

        // gamepad update
        {
            SceCtrlData ctrl_old = { 0 };
            memcpy(&ctrl_old, &ctrl_pad_one, sizeof(ctrl_old));
            sceCtrlPeekBufferPositive(SCE_CTRL_PAD_ONE, &ctrl_pad_one, 1);
            // gamepad button events
            const unsigned int changed = ctrl_old.buttons ^ ctrl_pad_one.buttons;
            if (changed) {
                if (changed & SCE_CTRL_TRIANGLE) {
                }
                if (changed & SCE_CTRL_CIRCLE) {
                }
                if (changed & SCE_CTRL_CROSS) {
                }
                if (changed & SCE_CTRL_SQUARE) {
                }
                if (changed & SCE_CTRL_LTRIGGER) {
                }
                if (changed & SCE_CTRL_RTRIGGER) {
                }
                if (changed & SCE_CTRL_DOWN) {
                }
                if (changed & SCE_CTRL_LEFT) {
                }
                if (changed & SCE_CTRL_UP) {
                }
                if (changed & SCE_CTRL_RIGHT) {
                }
                if (changed & SCE_CTRL_SELECT) {
                }
                if (changed & SCE_CTRL_START) {
                }
            }
            // gamepad axes events
            if (ctrl_old.lx != ctrl_pad_one.lx) {
            }
            if (ctrl_old.ly != ctrl_pad_one.ly) {
            }
            if (ctrl_old.rx != ctrl_pad_one.rx) {
            }
            if (ctrl_old.ry != ctrl_pad_one.ry) {
            }
        }
        // touch update
        {
            SceTouchData touch_old = { 0 };
            memcpy(&touch_old, &touch_front, sizeof(touch_old));
            sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch_front, 1);
            // touch events
            for (int i = 0; i < touch_front.reportNum; i++) {
                bool pressed = true;
                for (int j = 0; j < touch_old.reportNum; j++) {
                    if (touch_front.report[i].id == touch_old.report[j].id) {
                        pressed = false;
                        // touch move
                        if ((touch_front.report[i].x != touch_old.report[j].x) || (touch_front.report[i].y != touch_old.report[j].y)) {
                            /*
                            const int raw_x = math_lerp(touch_front.report[i].x, SCE_TOUCH_FRONT_MAX_WITDH, SCE_DISPLAY_WIDTH);
                            const int raw_y = math_lerp(touch_front.report[i].y, SCE_TOUCH_FRONT_MAX_HEIGHT, SCE_DISPLAY_HEIGHT);
							const int old_raw_x = math_lerp(touch_old.report[j].x, SCE_TOUCH_FRONT_MAX_WITDH, SCE_DISPLAY_WIDTH);
                            const int old_raw_y = math_lerp(touch_old.report[j].y, SCE_TOUCH_FRONT_MAX_HEIGHT, SCE_DISPLAY_HEIGHT);
                            const int dx = raw_x - old_raw_x;
                            const int dy = raw_y - old_raw_y;
                            */
                        }
                    }
                }
                // touch begin
                if (pressed) {
                    //const int raw_x = math_lerp(touch_front.report[i].x, SCE_TOUCH_FRONT_MAX_WITDH, SCE_DISPLAY_WIDTH);
                    //const int raw_y = math_lerp(touch_front.report[i].y, SCE_TOUCH_FRONT_MAX_HEIGHT, SCE_DISPLAY_HEIGHT);
                    
                }
            }
            for (int i = 0; i < touch_old.reportNum; i++) {
                bool released = true;
                for (int j = 0; j < touch_front.reportNum; j++) {
                    if (touch_old.report[i].id == touch_front.report[j].id) {
                        released = false;
                    }
                }
                // touch end
                if (released) {
                    //const int raw_x = math_lerp(touch_old.report[i].x, SCE_TOUCH_FRONT_MAX_WITDH, SCE_DISPLAY_WIDTH);
                    //const int raw_y = math_lerp(touch_old.report[i].y, SCE_TOUCH_FRONT_MAX_HEIGHT, SCE_DISPLAY_HEIGHT);
                    
                }
            }
        }
		
        
        sdtx_canvas(egl_surface_width*0.5f, egl_surface_height*0.5f);     
        sdtx_home();
        sdtx_color4b(0x00, 0xff, 0x00, 0xff);
        sdtx_puts("Hello world\n");
        sdtx_printf("raw frame time:     %.3fms (min: %.3f, max: %.3f)\n",
            stm_ms(raw_frame_time), stm_ms(min_raw_frame_time), stm_ms(max_raw_frame_time));
        sdtx_printf("rounded frame time: %.3fms (min: %.3f, max: %.3f)\n",
            stm_ms(rounded_frame_time), stm_ms(min_rounded_frame_time), stm_ms(max_rounded_frame_time));

        sdtx_printf("Buttons:%08X\n", ctrl_pad_one.buttons);
		for(int i=0; i < sizeof(ctrl_btn_label)/sizeof(*ctrl_btn_label); i++){
            if((ctrl_pad_one.buttons & (1<<i)))
			    sdtx_printf("%s ",ctrl_btn_label[i]);
		}
        sdtx_putc('\n');
        sdtx_printf("Sticks: %i:%i %i:%i\n", ctrl_pad_one.lx,ctrl_pad_one.ly, ctrl_pad_one.rx,ctrl_pad_one.ry);

        sdtx_printf("Front Touch: ");
        for(int i = 0; i < touch_front.reportNum; i++) {
            const int raw_x = math_lerp(touch_front.report[i].x, SCE_TOUCH_FRONT_MAX_WITDH, SCE_DISPLAY_WIDTH);
            const int raw_y = math_lerp(touch_front.report[i].y, SCE_TOUCH_FRONT_MAX_HEIGHT, SCE_DISPLAY_HEIGHT);
	        sdtx_printf("%4i:%-4i ", raw_x, raw_y);
        }
        sdtx_putc('\n');

        rot[0] += 1.0f;
        rot[1] += 2.0f;

        sgl_defaults();
        sgl_matrix_mode_projection();
        sgl_perspective(sgl_rad(45.0f), 1.0f, 0.1f, 100.0f);
        sgl_matrix_mode_modelview();
        sgl_translate(0.0f, 0.0f, -12.0f);
        sgl_rotate(sgl_rad(rot[0]), 1.0f, 0.0f, 0.0f);
        sgl_rotate(sgl_rad(rot[1]), 0.0f, 1.0f, 0.0f);
        sgl_begin_quads();
        sgl_c3f(1.0f, 0.0f, 0.0f);
            sgl_v3f_t2f(-1.0f,  1.0f, -1.0f, -1.0f,  1.0f);
            sgl_v3f_t2f( 1.0f,  1.0f, -1.0f,  1.0f,  1.0f);
            sgl_v3f_t2f( 1.0f, -1.0f, -1.0f,  1.0f, -1.0f);
            sgl_v3f_t2f(-1.0f, -1.0f, -1.0f, -1.0f, -1.0f);
        sgl_c3f(0.0f, 1.0f, 0.0f);
            sgl_v3f_t2f(-1.0, -1.0,  1.0, -1.0f,  1.0f);
            sgl_v3f_t2f( 1.0, -1.0,  1.0,  1.0f,  1.0f);
            sgl_v3f_t2f( 1.0,  1.0,  1.0,  1.0f, -1.0f);
            sgl_v3f_t2f(-1.0,  1.0,  1.0, -1.0f, -1.0f);
        sgl_c3f(0.0f, 0.0f, 1.0f);
            sgl_v3f_t2f(-1.0, -1.0,  1.0, -1.0f,  1.0f);
            sgl_v3f_t2f(-1.0,  1.0,  1.0,  1.0f,  1.0f);
            sgl_v3f_t2f(-1.0,  1.0, -1.0,  1.0f, -1.0f);
            sgl_v3f_t2f(-1.0, -1.0, -1.0, -1.0f, -1.0f);
        sgl_c3f(1.0f, 0.5f, 0.0f);
            sgl_v3f_t2f(1.0, -1.0,  1.0, -1.0f,   1.0f);
            sgl_v3f_t2f(1.0, -1.0, -1.0,  1.0f,   1.0f);
            sgl_v3f_t2f(1.0,  1.0, -1.0,  1.0f,  -1.0f);
            sgl_v3f_t2f(1.0,  1.0,  1.0, -1.0f,  -1.0f);
        sgl_c3f(0.0f, 0.5f, 1.0f);
            sgl_v3f_t2f( 1.0, -1.0, -1.0, -1.0f,  1.0f);
            sgl_v3f_t2f( 1.0, -1.0,  1.0,  1.0f,  1.0f);
            sgl_v3f_t2f(-1.0, -1.0,  1.0,  1.0f, -1.0f);
            sgl_v3f_t2f(-1.0, -1.0, -1.0, -1.0f, -1.0f);
        sgl_c3f(1.0f, 0.0f, 0.5f);
            sgl_v3f_t2f(-1.0,  1.0, -1.0, -1.0f,  1.0f);
            sgl_v3f_t2f(-1.0,  1.0,  1.0,  1.0f,  1.0f);
            sgl_v3f_t2f( 1.0,  1.0,  1.0,  1.0f, -1.0f);
            sgl_v3f_t2f( 1.0,  1.0, -1.0, -1.0f, -1.0f);
        sgl_end();


        sg_begin_default_pass(&pass_action, egl_surface_width, egl_surface_height);    
            sgl_draw();
            sdtx_draw();
        sg_end_pass();
        sg_commit();

        eglSwapBuffers(egl_display, egl_surface);
    }
    sgl_shutdown();
    sdtx_shutdown();
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