/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   RenderingBootstrap.hpp                            :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/10/08          by flaviobc      #+#        #+# #+#    #+#    #+#   */
/*   Updated: 2026/10/08          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#pragma once

#include <SDL3/SDL_video.h>

#include <memory>

namespace engine::graphics::detail
{
class OpenGlFunctions;
struct RenderingResources;

// Prepares and owns the graphics environment used by Renderer. The SDL window
// is borrowed from Core; shutdown remains explicit so every OpenGL cleanup runs
// while this component's context is current.
class RenderingBootstrap
{
  public:
	explicit RenderingBootstrap(SDL_Window &window);
	~RenderingBootstrap();

	RenderingBootstrap(const RenderingBootstrap &) = delete;
	RenderingBootstrap &operator=(const RenderingBootstrap &) = delete;
	RenderingBootstrap(RenderingBootstrap &&) = delete;
	RenderingBootstrap &operator=(RenderingBootstrap &&) = delete;

	[[nodiscard]] bool initialize();
	void shutdown();

	[[nodiscard]] bool hasFunctions() const;
	[[nodiscard]] bool hasResources() const;
	[[nodiscard]] OpenGlFunctions &functions();
	[[nodiscard]] RenderingResources &resources();

  private:
	[[nodiscard]] bool createContext();
	[[nodiscard]] bool loadFunctions();
	[[nodiscard]] bool createResources();
	void destroyResources();
	void destroyContext();

	SDL_Window *window_;
	SDL_GLContext context_ = nullptr;
	std::unique_ptr<OpenGlFunctions> functions_;
	std::unique_ptr<RenderingResources> resources_;
};
} // namespace engine::graphics::detail
