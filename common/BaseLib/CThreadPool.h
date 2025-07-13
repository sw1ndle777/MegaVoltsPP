#pragma once
#include <BS_thread_pool.hpp>
namespace BaseLib
{
	extern std::unique_ptr<BS::thread_pool<BS::tp::priority>> DbPool;
	extern std::unique_ptr<BS::thread_pool<BS::tp::priority>> LogPool;
}
