#pragma once

#include "Manager.h"

namespace NetRumble
{
	class AsyncTaskManager : public Manager
	{
	public:
		AsyncTaskManager() noexcept;

		void Tick();
	};
}
