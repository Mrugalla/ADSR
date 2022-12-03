﻿/*
 * CLAP - CLever Audio Plugin
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * Copyright (c) 2014...2022 Alexandre BIQUE <bique.alexandre@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#pragma once

#include "entry.h"
#include "plugin-factory.h"
#include "plugin-invalidation.h"
#include "plugin-features.h"

#include "plugin.h"
#include "host.h"

#include "ext/audio-ports-config.h"
#include "ext/audio-ports.h"
#include "ext/event-registry.h"
#include "ext/gui.h"
#include "ext/latency.h"
#include "ext/log.h"
#include "ext/note-name.h"
#include "ext/note-ports.h"
#include "ext/params.h"
#include "ext/posix-fd-support.h"
#include "ext/render.h"
#include "ext/state.h"
#include "ext/tail.h"
#include "ext/thread-check.h"
#include "ext/thread-pool.h"
#include "ext/timer-support.h"
#include "ext/voice-info.h"

#include "ext/draft/ambisonic.h"
#include "ext/draft/cv.h"
#include "ext/draft/file-reference.h"
#include "ext/draft/midi-mappings.h"
#include "ext/draft/preset-load.h"
#include "ext/draft/quick-controls.h"
#include "ext/draft/state-context.h"
#include "ext/draft/surround.h"
#include "ext/draft/track-info.h"
#include "ext/draft/tuning.h"
