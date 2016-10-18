#pragma once

#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <array>

#include "event_queue.h"
#include "events.h"
#include "worker.h"
#include "task.h"

namespace oak {

	class TaskManager {
	public:
		TaskManager();
		~TaskManager();

		void init();
		void destroy();

		void run();

		void addTask(Task task);
		void operator()(const GameExitEvent&);

	private:

		std::mutex tasksMutex_;
		std::array<std::queue<Task>, 2> tasks_;

		std::vector<Worker> workers_;

		std::atomic<bool> running_;

	};

}