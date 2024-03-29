/*
 * Copyright (C) 2018, 2019, 2020, 2021, 2022
 * Computer Graphics Group, University of Siegen
 * Written by Martin Lambers <martin.lambers@uni-siegen.de>
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef TGD_DL_HPP
#define TGD_DL_HPP

/* Provide the POSIX dlopen/dlclose/dlsym API on all operating systems.
 * The different file endings for dynamic libraries are defined with
 * the DOT_SO_STR macro. */

#if defined(__unix__)
#  include <dlfcn.h>
#  if defined(__APPLE__)
#    define DOT_SO_STR ".dylib"
#  else
#    define DOT_SO_STR ".so"
#  endif
#elif defined(_WIN32)
#  include <windows.h>
#  define DOT_SO_STR ".dll"
#  ifndef RTLD_NOW
#    define RTLD_NOW 0
#  endif
   void* dlopen(const char* filename, int /* flags */)
   {
       return LoadLibrary(filename);
   }
   int dlclose(void* handle)
   {
       FreeLibrary((HMODULE)handle);
       return 0;
   }
   void* dlsym(void* handle, const char* symbol)
   {
       return (void*)GetProcAddress((HMODULE)handle, symbol);
   }
#endif

#endif
