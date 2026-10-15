/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   RenderingBootstrap.cpp                            :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/10/08          by flaviobc      #+#        #+# #+#    #+#    #+#   */
/*   Updated: 2026/10/08          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#include "graphics/runtime/RenderingBootstrap.hpp"

#include "graphics/runtime/OpenGlFunctions.hpp"
#include "graphics/runtime/RenderingResources.hpp"

#include <SDL3/SDL_error.h>

#include <cstdio>
#include <memory>

namespace
{
constexpr int kOpenGlMajorVersion = 3;
constexpr int kOpenGlMinorVersion = 3;

bool reportSdlError(const char *operation)
{
	std::fprintf(stderr, "%s failed: %s\n", operation, SDL_GetError());
	return false;
}
} // namespace

namespace engine::graphics::detail
{
RenderingBootstrap::RenderingBootstrap(SDL_Window &window) : window_(&window)
{
}

RenderingBootstrap::~RenderingBootstrap() = default;

bool RenderingBootstrap::initialize()
{
	return createContext() && loadFunctions() && createResources();
}

void RenderingBootstrap::shutdown()
{
	destroyResources();
	destroyContext();
}

bool RenderingBootstrap::hasFunctions() const
{
	return functions_ != nullptr;
}

bool RenderingBootstrap::hasResources() const
{
	return resources_ != nullptr;
}

OpenGlFunctions &RenderingBootstrap::functions()
{
	return *functions_;
}

RenderingResources &RenderingBootstrap::resources()
{
	return *resources_;
}

bool RenderingBootstrap::createContext()
{
	context_ = SDL_GL_CreateContext(window_);
	if (!context_)
	{
		return reportSdlError("SDL_GL_CreateContext");
	}
	if (!SDL_GL_MakeCurrent(window_, context_))
	{
		return reportSdlError("SDL_GL_MakeCurrent");
	}
	if (!SDL_GL_SetSwapInterval(0))
	{
		std::fprintf(stderr, "SDL_GL_SetSwapInterval(0) failed: %s\n",
		             SDL_GetError());
	}
	int swapInterval = 0;
	if (SDL_GL_GetSwapInterval(&swapInterval))
	{
		std::fprintf(stderr, "OpenGL swap interval: %d.\n", swapInterval);
	}
	else
	{
		std::fprintf(stderr, "SDL_GL_GetSwapInterval failed: %s\n",
		             SDL_GetError());
	}

	int majorVersion = 0;
	int minorVersion = 0;
	int profile = 0;
	if (!SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &majorVersion) ||
	    !SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minorVersion) ||
	    !SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &profile))
	{
		return reportSdlError("SDL_GL_GetAttribute");
	}
	if (majorVersion < kOpenGlMajorVersion ||
	    (majorVersion == kOpenGlMajorVersion &&
	     minorVersion < kOpenGlMinorVersion) ||
	    (profile & SDL_GL_CONTEXT_PROFILE_CORE) == 0)
	{
		std::fprintf(stderr,
		             "OpenGL context %d.%d profile=0x%x is not 3.3 Core.\n",
		             majorVersion, minorVersion, profile);
		return false;
	}
	return true;
}

bool RenderingBootstrap::loadFunctions()
{
	functions_ = std::make_unique<OpenGlFunctions>();
	return functions_->load();
}

bool RenderingBootstrap::createResources()
{
	resources_ = std::make_unique<RenderingResources>();
	if (!resources_->create(*functions_))
	{
		return false;
	}
	int width = 0;
	int height = 0;
	if (!SDL_GetWindowSizeInPixels(window_, &width, &height))
	{
		return reportSdlError("SDL_GetWindowSizeInPixels");
	}
	return resources_->resizePostProcessTargets(*functions_, width, height);
}

void RenderingBootstrap::destroyResources()
{
	if (!resources_ || !functions_)
	{
		return;
	}
	if (context_ && !SDL_GL_MakeCurrent(window_, context_))
	{
		std::fprintf(stderr,
		             "SDL_GL_MakeCurrent during renderer cleanup failed: %s\n",
		             SDL_GetError());
		resources_.reset();
		return;
	}
	resources_->destroy(*functions_);
	resources_.reset();
}

void RenderingBootstrap::destroyContext()
{
	functions_.reset();
	if (!context_)
	{
		return;
	}
	if (!SDL_GL_DestroyContext(context_))
	{
		std::fprintf(stderr, "SDL_GL_DestroyContext failed: %s\n",
		             SDL_GetError());
	}
	else
	{
		std::fputs("OpenGL context destroyed.\n", stderr);
	}
	context_ = nullptr;
}
} // namespace engine::graphics::detail
