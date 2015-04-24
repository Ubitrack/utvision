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
 * @ingroup vision
 * @file
 * OpenCLManager manages the OpenCL Context and CommandQueue as singleton
 *
 * @author Martin Schwörer <schwoere@in.tum.de>
 */
#ifndef __Ubitrack_OPENCL_MANAGER_INCLUDED__
#define __Ubitrack_OPENCL_MANAGER_INCLUDED__

//opencl context
#include <CL/cl.h>
#include <boost/utility.hpp>
#include <utVision.h>

namespace Ubitrack { namespace Vision {
class UTVISION_EXPORT OpenCLManager
	: private boost::noncopyable
{

public:
	OpenCLManager(void);
	~OpenCLManager(void);

	/** get the OpenCLManager object */
	static OpenCLManager& singleton();

	static void destroyOpenCLManager();

	cl_context getContext() const;
	cl_command_queue getCommandQueue() const;

private:

	cl_context m_clContext;

	cl_command_queue m_clCommandQueue;
};

}}// namespace Vision::Ubitrack

#endif