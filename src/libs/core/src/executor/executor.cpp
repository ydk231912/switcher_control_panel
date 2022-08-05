/**
 * @file executor.h
 * @author 
 * @brief thread pool and asychronous exectutor
 * @version 1.0
 * @date 2022-08-02
 * 
 * @copyright Copyright (c) 2022
 * 
 */


//#include<iostream>
//#include<vector>
#include<algorithm>

#include "core/executor/executor.h"
#include "core/util/logger.h"


namespace seeder::core
{
    constexpr auto n_threads = 4;
    constexpr auto high_water_mark = n_threads * 16;

    

    executor::executor()
    {
        threads_.reserve(n_threads);

        for(auto i = 0; i < n_threads; ++i)
        {
            threads_.emplace_back(std::thread([this]() { this->run(); }));
        }
    }

    executor::~executor()
    {
        wait();
    }

    void executor::execute(F&& f)
    {
        std::unique_lock<std::mutex> lock(mutex_);

        cv_.wait(lock, [this]() { return tasks_.size() < high_water_mark; });

        tasks_.push_back(std::move(f));
        cv_.notify_one();
    }

    void executor::done()
    {
        done_ = true;
        cv_.notify_all();
    }

    void executor::wait()
    {
        done();
        
        std::for_each(threads_.begin(), threads_.end(), [](std::thread& t) {
            if(t.joinable()) t.join();
        });
    }

    void executor::run()
    {
        for(;;)
        {
            auto maybe_f = get_next();
            if(!maybe_f) return;
            execute_one(std::move(maybe_f));
        }
    }

    void executor::execute_one(F&& f)
    {
        try
        {
            f();
        }
        catch(std::exception& ex)
        {
            logger->error("exception while executing asynchronous task: {}", ex.what());
        }
        catch(...)
        {
            logger->error("unknown exception while executing asynchronous task");
        }
    }

    std::function<void()> executor::get_next()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this]() { return !tasks_.empty() || done_; });

        if(tasks_.empty())
        {
            //assert(done_);
            return nullptr;
        }

        auto f = tasks_.front();
        tasks_.erase(tasks_.begin());

        cv_.notify_one();

        return f;
    }


    // struct executor::impl
    // {
    //     impl()
    //     {
    //         threads_.reserve(n_threads);

    //         for(auto i = 0; i < n_threads; ++i)
    //         {
    //             threads_.emplace_back(std::thread([this]() { this->run(); }));
    //         }
    //     }

    //     void execute(F&& f)
    //     {
    //         std::unique_lock<std::mutex> lock(mutex_);

    //         cv_.wait(lock, [this]() { return tasks_.size() < high_water_mark; });

    //         tasks_.push_back(std::move(f));
    //         cv_.notify_one();
    //     }

    //     void done()
    //     {
    //         done_ = true;
    //         cv_.notify_all();
    //     }

    //     void wait()
    //     {
    //         std::for_each(threads_.begin(), threads_.end(), [](std::thread& t) {
    //             if(t.joinable()) t.join();
    //         });
    //     }

    //     void run()
    //     {
    //         for(;;)
    //         {
    //             auto maybe_f = get_next();
    //             if(!maybe_f) return;
    //             execute_one(std::move(*maybe_f));
    //         }
    //     }

    //     void execute_one(F&& f)
    //     {
    //         try
    //         {
    //             f();
    //         }
    //         catch(std::exception& ex)
    //         {
    //             //logger()->error("exception while executing asynchronous task: {}", ex.what());
    //         }
    //         catch(...)
    //         {
    //             //logger()->error("unknown exception while executing asynchronous task");
    //         }
    //     }

    //     std::optional<F> get_next()
    //     {
    //         std::unique_lock lock(mutex_);
    //         cv_.wait(lock, [this]() { return !tasks_.empty() || done_; });

    //         if(tasks_.empty())
    //         {
    //             assert(done_);
    //             return std::nullopt;
    //         }

    //         auto f = std::move(tasks_.front());
    //         tasks_.erase(tasks_.begin());

    //         cv_.notify_one();

    //         return f;
    //     }

    //     std::mutex mutex_;
    //     std::condition_variable cv_;
    //     std::vector<F> tasks_;
    //     std::vector<std::thread> threads_;

    //     std::atomic<int> total_   = 0;
    //     std::atomic<int> pending_ = 0;
    //     std::atomic<bool> done_   = false;

    //     using FT = std::future<void>;
    //     std::vector<FT> futures_;
    // };
//------------------------------------------------------------------------------
}