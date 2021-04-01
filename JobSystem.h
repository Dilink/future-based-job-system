#pragma once
#include <vector>
#include <functional>
#include <future>
#include <atomic>
#include <memory>

namespace job_system
{
	namespace priv
	{
		class IJob
		{
		public:
			std::atomic_bool m_Complete;

			inline bool IsFinished() const
			{
				return m_Complete.load();
			}

			virtual void ExecuteCallback() = 0;
		};

		template <typename T>
		class JobWithResult : public IJob
		{
		public:
			std::future<T> m_Future;
			std::function<void(T&)> m_Callback;

			virtual void ExecuteCallback() override
			{
				if (!m_Future.valid())
					return;

				auto&& result = m_Future.get();
				if (m_Callback)
				{
					m_Callback(result);
				}
			}
		};

		class JobWithoutResult : public IJob
		{
		public:
			std::future<void> m_Future;
			std::function<void()> m_Callback;

			virtual void ExecuteCallback() override
			{
				if (!m_Future.valid())
					return;

				m_Future.get();
				if (m_Callback)
				{
					m_Callback();
				}
			}
		};

		struct JobCompletionScope
		{
			std::atomic_bool& m_Complete;

			JobCompletionScope(std::atomic_bool& complete)
				: m_Complete(complete)
			{
			}

			~JobCompletionScope()
			{
				m_Complete = true;
			}
		};
	}

	class Waiter
	{
	protected:
		priv::IJob* m_Job = nullptr;
	public:
		explicit Waiter(priv::IJob* job)
			: m_Job(job)
		{
		}

		void Wait()
		{
			if (m_Job)
			{
				m_Job->ExecuteCallback();
			}
		}
	};

	template <typename T>
	class WaiterWithResult : public Waiter
	{
	private:
		T m_Result;
	public:
		explicit WaiterWithResult(priv::IJob* job)
			: Waiter(job)
			, m_Result(T{})
		{
		}

		WaiterWithResult(const WaiterWithResult<T>& other) = delete;

		WaiterWithResult(WaiterWithResult<T>&& other) noexcept
			: Waiter(std::move(other))
		{
			this->m_Result = other.m_Result;
			static_cast<priv::JobWithResult<T>*>(m_Job)->m_Callback = std::bind(&WaiterWithResult<T>::OnResult, this, std::placeholders::_1);
		}

		T& GetResult()
		{
			return m_Result;
		}
	private:
		void OnResult(T& result)
		{
			m_Result = std::move(result);
		}

		friend class JobSystem;
	};

	class JobSystem
	{
	private:
		std::vector<std::unique_ptr<priv::IJob>> m_Jobs;
	public:
		template <typename T>
		void AddJob(std::function<T()> work, std::function<void(T&)> callback)
		{
			auto&& job = CreateJobWithResult<T>(work);
			job->m_Callback = callback;
			m_Jobs.push_back(std::move(job));
		}

		template <typename T>
		WaiterWithResult<T> AddJob(std::function<T()> work)
		{
			auto&& job = CreateJobWithResult<T>(work);
			WaiterWithResult<T> waiter(job.get());
			job->m_Callback = std::bind(&WaiterWithResult<T>::OnResult, &waiter, std::placeholders::_1);
			m_Jobs.push_back(std::move(job));
			return waiter;
		}

		void AddJob(std::function<void()> work, std::function<void()> callback)
		{
			auto&& job = CreateJobWithoutResult(work);
			job->m_Callback = callback;
			m_Jobs.push_back(std::move(job));
		}

		Waiter AddJob(std::function<void()> work)
		{
			auto&& job = CreateJobWithoutResult(work);
			m_Jobs.push_back(std::move(job));
			return Waiter(m_Jobs.back().get());
		}

		void Tick()
		{
			for (auto itr = m_Jobs.begin(); itr != m_Jobs.end();)
			{
				if ((*itr)->IsFinished())
				{
					(*itr)->ExecuteCallback();
					itr = m_Jobs.erase(itr);
				}
				else
				{
					++itr;
				}
			}
		}

		inline bool HasJobs()
		{
			return !m_Jobs.empty();
		}
	private:
		template <typename T>
		std::unique_ptr<priv::JobWithResult<T>> CreateJobWithResult(std::function<T()> work)
		{
			std::unique_ptr<priv::JobWithResult<T>> job = std::make_unique<priv::JobWithResult<T>>();
			job->m_Future = std::async(std::launch::async, [job=job.get(), work]() {
				priv::JobCompletionScope completion_scoped(job->m_Complete);
				return work();
			});
			return job;
		}

		std::unique_ptr<priv::JobWithoutResult> CreateJobWithoutResult(std::function<void()> work)
		{
			std::unique_ptr<priv::JobWithoutResult> job = std::make_unique<priv::JobWithoutResult>();
			job->m_Future = std::async(std::launch::async, [job=job.get(), work]() {
				priv::JobCompletionScope completion_scoped(job->m_Complete);
				work();
			});
			return job;
		}
	};
}
