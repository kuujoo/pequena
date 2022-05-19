#pragma once

#include "task.h"
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <cassert>
#include <deque>

namespace peq
{
	namespace concurrency
	{
		using TaskRef = std::shared_ptr<ITask>;

		// Run create multiple instances of task T and run execute them all at the same time
		// Somekind horizontal scaling thing
		template <typename T = ITask>
		class Runner
		{
		public:
			Runner() : _threadCount(1)
			{
			}
			Runner& setThreads(unsigned c)
			{
				_threadCount = c;
				return *this;
			}
			void start()
			{
				for (unsigned i = 0; i < _threadCount; i++)
				{
					_tasks.push_back(std::make_shared<T>());
				}
				for (unsigned i = 0; i < _threadCount; i++)
				{
					_threads.push_back(std::thread(&Runner::runner, this, _tasks[i]));
				}
			}
			std::shared_ptr<T> get(unsigned id)
			{
				assert(id < _threadCount);
				return _tasks[id];
			}
			void wait() {
				for (unsigned i = 0; i < _threadCount; i++)
				{
					_threads[i].join();
				}
				_threads.clear();
				_tasks.clear();
			}
		private:
			void runner(std::shared_ptr<T> task)
			{
				task->awake();
				task->execute();
				task->destroy();
			}
		private:
			unsigned _threadCount;
			std::vector<std::shared_ptr<T>> _tasks;
			std::vector<std::thread> _threads;
		};
	}
}
