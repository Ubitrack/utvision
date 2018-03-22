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

/**
 * @file
 * Core definitions of the ubitrack vision library
 *
 * @author Ulrich Eck <ulrich.eck@tum.de>
 */

 #ifndef __UBITRACK_VISION_CONFIG_H_INCLUDED__
#define __UBITRACK_VISION_CONFIG_H_INCLUDED__

#ifndef HAVE_GLAD
#cmakedefine HAVE_GLAD
#endif

#ifndef HAVE_GLEW
#cmakedefine HAVE_GLEW
#endif

#if defined(HAVE_GLEW) && defined(HAVE_GLAD)
#error cannot use GLEW and GLAD at the same time.
#endif

#ifndef HAVE_OPENGL
#cmakedefine HAVE_OPENGL
#endif

#ifndef HAVE_OPENCL
#cmakedefine HAVE_OPENCL
#endif

#ifndef HAVE_LAPACK 
#cmakedefine HAVE_LAPACK
#endif
#ifndef HAVE_OPENCV 
#cmakedefine HAVE_OPENCV
#endif
#endif
