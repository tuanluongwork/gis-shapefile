#include "structured_logger.h"
#include "correlation_manager.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <random>
#include <algorithm>

using namespace logservices;

void benchmark_logging_performance() {
    std::cout << "=== Logging Performance Benchmark ===" << std::endl;
    
    auto& logger = StructuredLogger::getInstance();
    logger.initialize("performance-test");
    
    const int num_messages = 10000;
    const int num_threads = 4;
    const int messages_per_thread = num_messages / num_threads;
    
    // Benchmark synchronous logging
    {
        LOG_PERFORMANCE_SCOPE("sync_logging_benchmark", {{"message_count", std::to_string(num_messages)}});
        
        LOG_INFO("Starting synchronous logging benchmark", {{"messages", std::to_string(num_messages)}});
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < num_messages; ++i) {
            LOG_INFO("Benchmark message", {
                {"message_id", std::to_string(i)},
                {"thread_id", std::to_string(std::this_thread::get_id().hash())},
                {"timestamp", std::to_string(std::chrono::system_clock::now().time_since_epoch().count())}
            });
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        double duration_ms = duration.count() / 1000.0;
        double throughput = num_messages / (duration_ms / 1000.0);
        
        logger.logPerformance("sync_logging", duration_ms, 
                             {{"test_type", "synchronous"}}, 
                             {{"messages_logged", static_cast<double>(num_messages)}, 
                              {"throughput_msg_per_sec", throughput}});
        
        std::cout << "Synchronous logging: " << num_messages << " messages in " 
                  << duration_ms << "ms (" << throughput << " msg/sec)" << std::endl;
    }
    
    // Benchmark multi-threaded logging
    {
        LOG_PERFORMANCE_SCOPE("multithreaded_logging_benchmark", {
            {"message_count", std::to_string(num_messages)},
            {"thread_count", std::to_string(num_threads)}
        });
        
        LOG_INFO("Starting multi-threaded logging benchmark", {
            {"messages", std::to_string(num_messages)},
            {"threads", std::to_string(num_threads)}
        });
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::thread> threads;
        
        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([t, messages_per_thread]() {
                ProcessScope process_scope("perf-worker-" + std::to_string(t));
                
                for (int i = 0; i < messages_per_thread; ++i) {
                    LOG_INFO("Multithreaded benchmark message", {
                        {"message_id", std::to_string(t * messages_per_thread + i)},
                        {"thread_id", std::to_string(t)},
                        {"worker_id", std::to_string(std::this_thread::get_id().hash())}
                    });
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        double duration_ms = duration.count() / 1000.0;
        double throughput = num_messages / (duration_ms / 1000.0);
        
        logger.logPerformance("multithreaded_logging", duration_ms, 
                             {{"test_type", "multithreaded"}}, 
                             {{"messages_logged", static_cast<double>(num_messages)}, 
                              {"throughput_msg_per_sec", throughput},
                              {"thread_count", static_cast<double>(num_threads)}});
        
        std::cout << "Multi-threaded logging: " << num_messages << " messages in " 
                  << duration_ms << "ms (" << throughput << " msg/sec) using " 
                  << num_threads << " threads" << std::endl;
    }
}

void benchmark_correlation_overhead() {
    std::cout << "\n=== Correlation Overhead Benchmark ===" << std::endl;
    
    const int num_operations = 100000;
    
    // Benchmark without correlation
    {
        LOG_PERFORMANCE_SCOPE("no_correlation_benchmark");
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < num_operations; ++i) {
            // Simulate some work
            volatile int dummy = i * 2;
            (void)dummy;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        double avg_ns = static_cast<double>(duration.count()) / num_operations;
        
        std::cout << "Without correlation: " << avg_ns << " ns per operation" << std::endl;
    }
    
    // Benchmark with correlation
    {
        LOG_PERFORMANCE_SCOPE("with_correlation_benchmark");
        
        ProcessScope process_scope("correlation-benchmark");
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < num_operations; ++i) {
            ActivityScope activity("operation_" + std::to_string(i % 100));
            
            // Simulate some work
            volatile int dummy = i * 2;
            (void)dummy;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        double avg_ns = static_cast<double>(duration.count()) / num_operations;
        
        std::cout << "With correlation: " << avg_ns << " ns per operation" << std::endl;
    }
}

void benchmark_structured_data() {
    std::cout << "\n=== Structured Data Benchmark ===" << std::endl;
    
    auto& logger = StructuredLogger::getInstance();
    const int num_messages = 1000;
    
    // Generate sample structured data
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1000.0);
    
    // Benchmark minimal context
    {
        LOG_PERFORMANCE_SCOPE("minimal_context_logging");
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < num_messages; ++i) {
            LOG_INFO("Minimal message", {{"id", std::to_string(i)}});
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        double duration_ms = duration.count() / 1000.0;
        
        std::cout << "Minimal context: " << duration_ms << "ms for " << num_messages << " messages" << std::endl;
    }
    
    // Benchmark rich context
    {
        LOG_PERFORMANCE_SCOPE("rich_context_logging");
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < num_messages; ++i) {
            logger.log(spdlog::level::info, "Rich context message", {
                {"message_id", std::to_string(i)},
                {"user_id", "user_" + std::to_string(i % 100)},
                {"session_id", "sess_" + std::to_string(i % 50)},
                {"operation", "data_processing"},
                {"status", "processing"},
                {"source_ip", "192.168.1." + std::to_string(i % 255)},
                {"user_agent", "BenchmarkClient/1.0"},
                {"request_method", "POST"}
            }, {
                {"processing_time_ms", dis(gen)},
                {"memory_usage_mb", dis(gen)},
                {"cpu_usage_percent", dis(gen) / 10.0},
                {"network_latency_ms", dis(gen) / 100.0},
                {"cache_hit_ratio", dis(gen) / 1000.0}
            });
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        double duration_ms = duration.count() / 1000.0;
        
        std::cout << "Rich context: " << duration_ms << "ms for " << num_messages << " messages" << std::endl;
    }
}

void demonstrate_memory_usage() {
    std::cout << "\n=== Memory Usage Pattern ===" << std::endl;
    
    ProcessScope process_scope("memory-demo");
    
    // Simulate memory usage patterns
    std::vector<std::unique_ptr<std::vector<int>>> memory_blocks;
    
    for (int i = 0; i < 100; ++i) {
        LOG_ACTIVITY_SCOPE("memory_allocation", {{"block_id", std::to_string(i)}});
        
        // Allocate memory block
        auto block = std::make_unique<std::vector<int>>(10000, i);
        memory_blocks.push_back(std::move(block));
        
        // Log memory allocation
        logger.logEvent("memory_allocated", "Allocated memory block", {
            {"block_id", std::to_string(i)},
            {"block_size_bytes", std::to_string(10000 * sizeof(int))},
            {"total_blocks", std::to_string(memory_blocks.size())}
        });
        
        // Simulate processing time
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        
        // Occasionally free some memory
        if (i > 50 && i % 10 == 0) {
            memory_blocks.erase(memory_blocks.begin(), memory_blocks.begin() + 5);
            
            LOG_INFO("Memory cleanup performed", {
                {"blocks_freed", "5"},
                {"remaining_blocks", std::to_string(memory_blocks.size())}
            });
        }
    }
    
    LOG_INFO("Memory usage demonstration completed", {
        {"final_block_count", std::to_string(memory_blocks.size())}
    });
}

int main() {
    try {
        auto& logger = StructuredLogger::getInstance();
        logger.loadConfigFromYaml("logging-development.yaml");
        logger.initialize("performance-benchmark");
        
        LOG_INFO("Starting performance benchmarks");
        
        benchmark_logging_performance();
        benchmark_correlation_overhead();
        benchmark_structured_data();
        demonstrate_memory_usage();
        
        LOG_INFO("All benchmarks completed successfully");
        
        // Ensure all logs are written
        logger.flush();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        std::cout << "\n=== Benchmarks completed ===\nCheck log files for detailed structured output" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Performance benchmark failed: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}