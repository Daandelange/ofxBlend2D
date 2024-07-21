# ofxBlend2D

An OpenFrameworks addon integrating [libBlend2D](https://blend2d.com/), a blazing fast CPU 2D vector graphics renderer powered by a [JIT compiler](https://en.wikipedia.org/wiki/Just-in-time_compilation).

**Features:**  
- Library loaded as an embedded library with access to C++ types.
- A wrapper (`ofxBlend2DThreadedRenderer`) to use for asynchronous multithreaded rendering, keeping the framerate of your openFrameworks pipeline.
- Can also be used in the main thread, at risk of reducing your `ofApp` framerate (blocking API).

**Technically:**  
- Draw commands are submitted by the main thread (a dedicated thread is also possible).
- Then the pipeline is flushed using multiple threads.
- When the threads are done rendering, the resulting pixels are loaded into an `ofTexture` (from within the GL thread).
- The texture is available for rendering and updates as soon as a new frame is available.

# Compatibility
Tested on MacOs + Linux, both of_v0.11.2 and of_v0.12.0, both c++14 and c++17.  
Should work on Windows too.

# Installation
- Warning! libBlend2D doesn't compile with **Clang 3.8 and below** (until Xcode 8.x).  
  If so, update your Command Line Toolkit to at least Xcode 9 and configure it with `xcode-select -s`.
- Clone **recursively** (to get the submodules).
- Install **optional** dependencies:  
  - [ofxFPS](https://github.com/tobiasebsen/ofxFps) to embed an FPS counter for the renderer.
  - [ofxImGui/develop](https://github.com/jvcleave/ofxImGui/tree/develop) to provide GUI helper widgets.
- Install `example-svg` dependencies: (*`example-simple` doesn't need any*)  
  - The optional dependencies (*above*)
  - [ofxSvgLoader](https://github.com/NickHardeman/ofxSvgLoader)


# Usage
I recommend reading trough the [Blend2D guide](https://blend2d.com/doc/getting-started.html) to get started using their graphics API.

Some OpenFrameworks / Blend2D glue utilities are being written, any contribution is welcome to facilitate interaction with OF objects.

Please note that Blend2d runs on a JIT interpreter and performance varies a lot between Debug and Release builds due to their respective exported debug symbols and compile-time optimisations. For performance, prefer Release builds.

# Examples
- `example-simple` : A bare-bones example of how to use the C++ Blend2D API, pretty similar to the Blend2D "getting started" examples.
- `example-svg` : Loads an SVG to provide some `ofPath` which are converted to `BLPath` for rendering in Blend2D. Also has a GUI which lets you interactively change some parameters.

# Future ideas
- Implement as ofRenderer subclass (would need porting all `ofDraw...` functions to ofxBlend2D, but provide an easier integration with OF).
- Provide instructions for building libBlend2D as a library (*to prevent recompiling for every single project*).
- Offline rendering outputting to files.

# License
libBlend2D is [zLib](https://github.com/blend2d/blend2d/blob/master/LICENSE.md), by [Petr Kobalicek](https://kobalicek.com).  
ofxBlend2D is [MIT](https://github.com/daandelange/ofxBlend2D/blob/master/LICENSE.md), by [Daan de Lange](https://daandelange.com/).  
