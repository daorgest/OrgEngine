//
// Created by Orgest on 8/22/2024.
//

#ifndef PLATFORMSDL_H
#define PLATFORMSDL_H

#include "../Core/Logger.h"
#include "WindowContext.h"

namespace Platform
{
	class SDL
	{
	public:
		SDL(WindowContext* windowContext);
		~	 SDL();
		void Init() const;
		void Cleanup() const;
		void Update();
	private:
		WindowContext* windowContext_;
#ifdef _DEBUG
		const char* appName = "OrgEngine - Debug";
#else
		const char* appName = "OrgEngine - Release";
#endif
	};
}
#endif //PLATFORMSDL_H
