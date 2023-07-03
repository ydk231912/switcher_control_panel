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

#pragma once

#include <functional>
#include <future>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <optional>

namespace seeder::core
{
    class executor
    {
      public:
        executor();
        ~executor();

        using F = std::function<void()>;

        void execute(std::function<void()>&& f);
        void wait();
        void done();
        void run();
        void execute_one(F&& f);
        std::function<void()> get_next();

      private:
        // struct impl;
        // std::unique_ptr<impl> impl_;

        std::mutex mutex_;
        std::condition_variable cv_;
        std::vector<F> tasks_;
        std::vector<std::thread> threads_;

        //std::atomic<int> total_   = 0;
        //std::atomic<int> pending_ = 0;
        std::atomic<bool> done_   = false;

        using FT = std::future<void>;
        std::vector<FT> futures_;
    };
}