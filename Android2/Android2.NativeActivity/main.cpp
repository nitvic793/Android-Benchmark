/*
* Copyright (C) 2010 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/

#include <exception>
#include "Benchmark/Benchmarks.h"
#include <thread>
#include <mutex>
#include <condition_variable>

/**
* Our saved state data.
*/
struct saved_state {
	float angle;
	int32_t x;
	int32_t y;
};

/**
* Shared state for our app.
*/
struct AndroidInstance {
	struct android_app* app;

	int animating;
	int32_t width;
	int32_t height;
	struct saved_state state;
};

struct android_app* androidState;

/**
* Process the next input event.
*/
static int32_t engine_handle_input(struct android_app* app, AInputEvent* event) {
	struct AndroidInstance* engine = (struct AndroidInstance*)app->userData;
	if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
		engine->state.x = AMotionEvent_getX(event, 0);
		engine->state.y = AMotionEvent_getY(event, 0);
		return 1;
	}
	return 0;
}

/**
* Process the next main command.
*/
static void engine_handle_cmd(struct android_app* app, int32_t cmd) {
	struct AndroidInstance* engine = (struct AndroidInstance*)app->userData;
	switch (cmd) {
	case APP_CMD_SAVE_STATE:
		// The system has asked us to save our current state.  Do so.
		engine->app->savedState = malloc(sizeof(struct saved_state));
		*((struct saved_state*)engine->app->savedState) = engine->state;
		engine->app->savedStateSize = sizeof(struct saved_state);
		break;
	case APP_CMD_INIT_WINDOW:
		// The window is being shown, get it ready.
		break;
	case APP_CMD_TERM_WINDOW:
		break;
	case APP_CMD_GAINED_FOCUS:
		// When our app gains focus, we start monitoring the accelerometer.
		break;
	case APP_CMD_LOST_FOCUS:

		engine->animating = 0;
		break;

	}
}

/**
* This is the main entry point of a native application that is using
* android_native_app_glue.  It runs in its own thread, with its own
* event loop for receiving input events and doing other things.
*/
void android_main(struct android_app* state) {
	struct AndroidInstance engine;
	androidState = state;
	memset(&engine, 0, sizeof(engine));
	state->userData = &engine;
	state->onAppCmd = engine_handle_cmd;
	state->onInputEvent = engine_handle_input;
	engine.app = state;

	if (state->savedState != NULL) {
		// We are starting with a previous saved state; restore from it.
		engine.state = *(struct saved_state*)state->savedState;
	}
	bool startRunning = false;
	std::mutex mutex;
	std::condition_variable cond_var;
	engine.animating = 1;

	std::thread th([&]() {
		{
			std::unique_lock<std::mutex> l(mutex);

			while (!startRunning)
			{
				cond_var.wait(l);
			}
		}

		double benchmarkTime[3] = { 0.0 };
		benchmarkTime[0] = BenchmarkL1ToCPU();
		benchmarkTime[1] = BenchmarkL2ToCPU();
		benchmarkTime[2] = BenchmarkMainMemToCPU();
		ANativeActivity_finish(state->activity);
	});
	// loop waiting for stuff to do.

	while (true) {
		//// Read all pending events.
		int ident;
		int events;
		struct android_poll_source* source;

		while ((ident = ALooper_pollAll(engine.animating ? 0 : -1, NULL, &events,
			(void**)&source)) >= 0) {

			// Process this event.
			if (source != NULL)
			{
				source->process(state, source);
			}

			{
				std::lock_guard<std::mutex> l(mutex);
				startRunning = true;
				cond_var.notify_one();
			}
		}
	}
}
