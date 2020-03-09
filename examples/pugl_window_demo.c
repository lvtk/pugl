/*
  Copyright 2012-2020 David Robillard <http://drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/**
   @file pugl_windows_demo.c A demonstration of multiple Pugl windows.
*/

#define GL_SILENCE_DEPRECATION 1

#include "cube_view.h"
#include "demo_utils.h"
#include "test/test_utils.h"

#include "pugl/gl.h"
#include "pugl/pugl.h"
#include "pugl/pugl_gl.h"

#include <math.h>
#include <stdbool.h>
#include <string.h>

typedef struct {
	PuglView* view;
	double    xAngle;
	double    yAngle;
	float     dist;
	double    lastMouseX;
	double    lastMouseY;
	double    lastDrawTime;
	bool      entered;
} CubeView;

typedef struct {
	PuglWorld* world;
	CubeView   cubes[2];
	bool       continuous;
	int        quit;
	bool       verbose;
} PuglTestApp;

static void
onDisplay(PuglView* view)
{
	PuglWorld*   world = puglGetWorld(view);
	PuglTestApp* app   = (PuglTestApp*)puglGetWorldHandle(world);
	CubeView*    cube  = (CubeView*)puglGetHandle(view);

	const double thisTime = puglGetTime(app->world);
	if (app->continuous) {
		const double dTime = thisTime - cube->lastDrawTime;

		cube->xAngle = fmod(cube->xAngle + dTime * 100.0, 360.0);
		cube->yAngle = fmod(cube->yAngle + dTime * 100.0, 360.0);
	}

	displayCube(view, cube->dist, cube->xAngle, cube->yAngle, cube->entered);

	cube->lastDrawTime = thisTime;
}

static void
onKeyPress(PuglView* view, const PuglEventKey* event)
{
	PuglWorld*   world = puglGetWorld(view);
	PuglTestApp* app   = (PuglTestApp*)puglGetWorldHandle(world);
	PuglRect     frame = puglGetFrame(view);

	if (event->key == 'q' || event->key == PUGL_KEY_ESCAPE) {
		app->quit = 1;
	} else if (event->state & PUGL_MOD_SHIFT) {
		if (event->key == PUGL_KEY_UP) {
			frame.height += 10;
		} else if (event->key == PUGL_KEY_DOWN) {
			frame.height -= 10;
		} else if (event->key == PUGL_KEY_LEFT) {
			frame.width -= 10;
		} else if (event->key == PUGL_KEY_RIGHT) {
			frame.width += 10;
		} else {
			return;
		}
		puglSetFrame(view, frame);
	} else {
		if (event->key == PUGL_KEY_UP) {
			frame.y -= 10;
		} else if (event->key == PUGL_KEY_DOWN) {
			frame.y += 10;
		} else if (event->key == PUGL_KEY_LEFT) {
			frame.x -= 10;
		} else if (event->key == PUGL_KEY_RIGHT) {
			frame.x += 10;
		} else {
			return;
		}
		puglSetFrame(view, frame);
	}
}

static PuglStatus
onEvent(PuglView* view, const PuglEvent* event)
{
	PuglWorld*   world = puglGetWorld(view);
	PuglTestApp* app   = (PuglTestApp*)puglGetWorldHandle(world);
	CubeView*    cube  = (CubeView*)puglGetHandle(view);

	const char* const prefix = cube == &app->cubes[0] ? "View 1: " : "View 2: ";
	printEvent(event, prefix, app->verbose);

	switch (event->type) {
	case PUGL_CONFIGURE:
		reshapeCube((int)event->configure.width, (int)event->configure.height);
		break;
	case PUGL_EXPOSE:
		onDisplay(view);
		break;
	case PUGL_CLOSE:
		app->quit = 1;
		break;
	case PUGL_KEY_PRESS:
		onKeyPress(view, &event->key);
		break;
	case PUGL_MOTION_NOTIFY:
		cube->xAngle -= event->motion.x - cube->lastMouseX;
		cube->yAngle += event->motion.y - cube->lastMouseY;
		cube->lastMouseX = event->motion.x;
		cube->lastMouseY = event->motion.y;
		if (!app->continuous) {
			puglPostRedisplay(view);
		}
		break;
	case PUGL_SCROLL:
		cube->dist = fmaxf(10.0f, cube->dist + (float)event->scroll.dy);
		if (!app->continuous) {
			puglPostRedisplay(view);
		}
		break;
	case PUGL_ENTER_NOTIFY:
		cube->entered = true;
		break;
	case PUGL_LEAVE_NOTIFY:
		cube->entered = false;
		break;
	default:
		break;
	}

	return PUGL_SUCCESS;
}

int
main(int argc, char** argv)
{
	PuglTestApp app = {0};

	const PuglTestOptions opts = puglParseTestOptions(&argc, &argv);
	if (opts.help) {
		puglPrintTestUsage(argv[0], "");
		return 1;
	}

	app.continuous = opts.continuous;
	app.verbose    = opts.verbose;

	app.world         = puglNewWorld();
	app.cubes[0].view = puglNewView(app.world);
	app.cubes[1].view = puglNewView(app.world);

	puglSetWorldHandle(app.world, &app);
	puglSetClassName(app.world, "Pugl Test");

	PuglStatus st = PUGL_SUCCESS;
	for (size_t i = 0; i < 2; ++i) {
		CubeView*      cube  = &app.cubes[i];
		PuglView*      view  = cube->view;
		static const double pad = 64.0;
		const PuglRect frame = {pad + (256.0 + pad) * i,
		                        pad + (256.0 + pad) * i,
		                        256.0,
		                        256.0};

		cube->dist = 10;

		puglSetFrame(view, frame);
		puglSetMinSize(view, 128, 128);
		puglSetBackend(view, puglGlBackend());

		puglSetViewHint(view, PUGL_USE_DEBUG_CONTEXT, opts.errorChecking);
		puglSetViewHint(view, PUGL_RESIZABLE, opts.resizable);
		puglSetViewHint(view, PUGL_SAMPLES, opts.samples);
		puglSetViewHint(view, PUGL_DOUBLE_BUFFER, opts.doubleBuffer);
		puglSetViewHint(view, PUGL_SWAP_INTERVAL, opts.sync);
		puglSetViewHint(view, PUGL_IGNORE_KEY_REPEAT, opts.ignoreKeyRepeat);
		puglSetHandle(view, cube);
		puglSetEventFunc(view, onEvent);

		if ((st = puglCreateWindow(view, "Pugl"))) {
			return logError("Failed to create window window (%s)\n",
			                puglStrerror(st));
		}

		puglShowWindow(view);
	}

	PuglFpsPrinter fpsPrinter  = {puglGetTime(app.world)};
	unsigned       framesDrawn = 0;
	while (!app.quit) {
		if (app.continuous) {
			for (size_t i = 0; i < 2; ++i) {
				puglPostRedisplay(app.cubes[i].view);
			}
		} else {
			puglPollEvents(app.world, -1);
		}

		puglDispatchEvents(app.world);
		++framesDrawn;

		if (app.continuous) {
			puglPrintFps(app.world, &fpsPrinter, &framesDrawn);
		}
	}

	for (size_t i = 0; i < 2; ++i) {
		puglFreeView(app.cubes[i].view);
	}

	puglFreeWorld(app.world);

	return 0;
}