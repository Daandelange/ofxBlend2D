# ofxBlend2D

An OpenFrameworks addon integrating [libBlend2D](https://blend2d.com/), a blazing fast CPU 2D vector graphics renderer powered by a [JIT compiler](https://en.wikipedia.org/wiki/Just-in-time_compilation).

Features:
- Library loaded as an embedded library with access to C++ types.
- A wrapper (`ofxBlend2DThreadedRenderer`) to use for asynchronous multithreaded rendering, keeping the framerate of your openFrameworks pipeline.
- Can also be used in the main thread, at risk of reducing your `ofApp` framerate (blocking API).

Technically :
	- Draw commands are submitted by the main thread (a dedicated thread is also possible).
	- Then the pipeline is flushed using multiple threads.
	- When the threads are done rendering, the resulting pixels are loaded into an `ofTexture` (from within the GL thread).
	- The texture is available for rendering and updates as soon as a new frame is available.

# Compatibility
Tested on OSX + of0.11.2.  
Should work on Linux and Windows too.

# Installation
- Warning! libBlend2D doesn't compile with **Clang 3.8 and below** (until Xcode 8.x).  
  If so, update your Command Line Toolkit to at least Xcode 9 and configure it with `xcode-select -s`.
- Clone recursively (to get the submodules).
- Install dependencies: [ofxFPS](https://github.com/tobiasebsen/ofxFps).

# Usage
I recommend reading trough the [Blend2D guide](https://blend2d.com/doc/getting-started.html) to get started using their graphics API.

# Future ideas
- Implement as ofRenderer subclass (would need porting all `ofDraw...` functions to ofxBlend2D, but provide an easier integration with OF).
- Provide instructions for building libBlend2D as a library.
- Offline rendering to files.

# License
libBlend2D is [zLib](https://github.com/blend2d/blend2d/blob/master/LICENSE.md), by [Petr Kobalicek](https://kobalicek.com).  
ofxBlend2D is [MIT](https://github.com/daandelange/ofxBlend2D/blob/master/LICENSE.md), by [Daan de Lange](https://daandelange.com/).  
ofxFPS is [Apache](https://github.com/tobiasebsen/ofxFps/blob/master/LICENSE), by [Tobias Ebsen](https://github.com/tobiasebsen).
