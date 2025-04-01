#include "CThreadPool.h"
namespace BaseLib
{
	std::unique_ptr<BS::thread_pool<BS::tp::priority>> DbPool;
	std::unique_ptr<BS::thread_pool<BS::tp::priority>> LogPool;
}
