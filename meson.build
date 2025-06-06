#
#  Copyright (c) 2018 Lyude Paul <thatslyude@gmail.com>
#
#  Permission is hereby granted, free of charge, to any person obtaining a
#  copy of this software and associated documentation files (the "Software"),
#  to deal in the Software without restriction, including without limitation
#  the rights to use, copy, modify, merge, publish, distribute, sublicense,
#  and/or sell copies of the Software, and to permit persons to whom the
#  Software is furnished to do so, subject to the following conditions:
#
#  The above copyright notice and this permission notice (including the next
#  paragraph) shall be included in all copies or substantial portions of the
#  Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
#  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
#  SOFTWARE.
#
project(
  'kmscube',
  'c',
  version : '0.0.1',
  license : 'MIT',
  meson_version : '>= 0.47',
  default_options : ['c_std=gnu99', 'warning_level=2']
)

if get_option('c_std') != 'gnu99'
  error('c_std must be gnu99')
endif

sources = files(
  'common.c',
  'cube-smooth.c',
  'cube-gears.c',
  'cube-tex.c',
  'drm-atomic.c',
  'drm-common.c',
  'drm-legacy.c',
  'drm-offscreen.c',
  'esTransform.c',
  'frame-512x512-NV12.c',
  'frame-512x512-RGBA.c',
  'kmscube.c',
  'perfcntrs.c',
)

cc = meson.get_compiler('c')
dep_m = cc.find_library('m', required : false)
dep_threads = dependency('threads')
dep_libdrm = dependency('libdrm', version : '>=2.4.71')
dep_gbm = dependency('gbm', version : '>=13.0')
dep_egl = dependency('egl')
dep_gles2 = dependency('glesv2')

with_gles3 = cc.check_header('GLES3/gl3.h', dependencies: dep_gles2)

if with_gles3
  sources += files('cube-shadertoy.c')
  add_project_arguments('-DHAVE_GLES3', language : 'c')
  message('GLES3 supported; shadertoy & texturator are included in this build')
else
  warning('GLES3 not supported; shadertoy & texturator are NOT included in this build')
endif

dep_libpng = dependency('libpng', required : false)

if dep_libpng.found()
  add_project_arguments('-DHAVE_LIBPNG', language : 'c')
endif

dep_common = [dep_m, dep_threads, dep_libdrm, dep_gbm, dep_egl, dep_gles2, dep_libpng]

dep_gst = []
foreach _dep : [
  'gstreamer-1.0',
  'gstreamer-plugins-base-1.0',
  'gstreamer-app-1.0',
  'gstreamer-allocators-1.0',
  'gstreamer-video-1.0',
]
  dep_gst += dependency(_dep, version : '>= 1.22.0', required : get_option('gstreamer'))
endforeach
dep_gst += dependency('glib-2.0', required : get_option('gstreamer'))

# See if we found everything we needed
with_gst = true
foreach _dep : dep_gst
  if not _dep.found()
    with_gst = false
  endif
endforeach

if with_gst
  dep_common += dep_gst
  sources += files('cube-video.c', 'gst-decoder.c')
  add_project_arguments('-DHAVE_GST', language : 'c')
  message('Building with gstreamer support')
else
  message('Building without gstreamer support')
endif

executable('kmscube', sources, dependencies : dep_common, install : true)

if with_gles3
  executable('texturator', files(
    'common.c',
    'drm-legacy.c',
    'drm-common.c',
    'perfcntrs.c',  # not used, but required to link
    'texturator.c',
  ), dependencies : dep_common, install : true)
endif
