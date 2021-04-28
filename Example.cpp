#include <iostream>
#include <chrono>

#include "JobSystem.h"

using namespace std::chrono_literals;

int main()
{
	job_system::JobSystem jobSystem;

	jobSystem.AddJob([]() {
		std::cout << "(1) Start working..." << std::endl;
		std::this_thread::sleep_for(300ms);
		std::cout << "(1) Work done" << std::endl;
	}, []() {
		std::cout << "(1) Callback" << std::endl;
	});

	jobSystem.AddJob<int>([]() {
		std::cout << "(2) Start working..." << std::endl;
		std::this_thread::sleep_for(1500ms);
		std::cout << "(2) Work done" << std::endl;
		return 4;
	}, [](int result) {
		std::cout << "(2) Callback, result: " << result << std::endl;
	});

	auto&& job3 = jobSystem.AddJob([]() {
		std::cout << "(3) Start working..." << std::endl;
		std::this_thread::sleep_for(500ms);
		std::cout << "(3) Work done" << std::endl;
	});
	std::cout << "Waiting for job3 to finish" << std::endl;
	job3.Wait();

	auto&& job4 = jobSystem.AddJob<int>([]() {
		std::cout << "(4) Start working..." << std::endl;
		std::this_thread::sleep_for(100ms);
		std::cout << "(4) Work done" << std::endl;
		return 42;
	});
	std::cout << "Waiting for job4..." << std::endl;
	job4.Wait();
	std::cout << "Job4 finished, result: " << job4.GetResult() << std::endl;

	jobSystem.AddJob([]() {
		std::cout << "(5) Start working..." << std::endl;
		std::this_thread::sleep_for(100ms);
		std::cout << "(5) Work done" << std::endl;
	}, [&]() {
		std::cout << "(5) Callback - pre" << std::endl;
		jobSystem.AddJob([]() {
			std::cout << "(6) Start working..." << std::endl;
			std::this_thread::sleep_for(125ms);
			std::cout << "(6) Work done" << std::endl;
		}, []() {
			std::cout << "(6) Callback" << std::endl;
		});
		std::cout << "(5) Callback - post" << std::endl;
	});

	// This can be the game loop
	while (jobSystem.HasJobs())
	{
		jobSystem.Tick();
	}

	std::cout << "\n=== Rest of the application here ===\n" << std::endl;
	return 0;
}
