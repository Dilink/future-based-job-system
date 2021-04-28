# Future based Job System

## Usage

Create a JobSystem instance
```cpp
job_system::JobSystem jobSystem;
```

Create a job with a callback
```cpp
jobSystem.AddJob([]() {
    // This code will be executed in another thread
}, []() {
    // This callback is executed in the caller thread
});
```

Create a job with a callback and a result
```cpp
// Job returns an int
jobSystem.AddJob<int>([]() {
    return 4;
}, [](int result) {

});
```

Callback is optional. If not provided, user can let current thread wait for the job to be finished.
```cpp
auto&& job = jobSystem.AddJob([]() {
    // Job code here
});
job.Wait(); // Wait for the job to be finished before continuing
```

Job can also returns a result
```cpp
auto&& job = jobSystem.AddJob<int>([]() {
    return 4;
});
job.Wait(); // Wait for the job to be finished before continuing
int result = job.GetResult();
```

In order for the callbacks to be triggered, user has to call Tick method at some point.
```cpp
while (jobSystem.HasJobs()) // This can be the game loop
{
    jobSystem.Tick();
}
```

## Example

A working example can be found in [Example.cpp](Example.cpp).

Here is a possible output:

```
Waiting for job3 to finish
(1) Start working...
(2) Start working...
(3) Start working...
(1) Work done
(3) Work done
Waiting for job4...
(4) Start working...
(4) Work done
Job4 finished, result: 42
(1) Callback
(5) Start working...
(5) Work done
(5) Callback - pre
(5) Callback - post
(6) Start working...
(6) Work done
(6) Callback
(2) Work done
(2) Callback, result: 4

=== Rest of the application here ===
```
