/*
 * Ubitrack - Library for Ubiquitous Tracking
 * Copyright 2006, Technische Universitaet Muenchen, and individual
 * contributors as indicated by the @authors tag. See the
 * copyright.txt in the distribution for a full listing of individual
 * contributors.
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA, or see the FSF site: http://www.fsf.org.
 */

#ifndef _OPENGLWRAPPER_H_
#define _OPENGLWRAPPER_H_

#include <utVision.h>

#ifdef HAVE_GLAD
  #include <glad/glad.h>
#else
  #ifdef HAVE_GLEW
    #include <GL/glew.h>
  #endif
  #ifdef _WIN32
	#include <utUtil/CleanWindows.h>
	#include <GL/gl.h>
	#include <GL/glu.h>
	// #include <GL/glext.h>
  #elif __APPLE__
	#include <OpenGL/gl.h>
    #include <OpenGL/glu.h>
  #else
	#include <GL/gl.h>
	// #include <GL/glext.h>
	#include <GL/glu.h>
	#include <GL/glx.h>
  #endif
#endif

#endif // _OPENGLWRAPPER_H_