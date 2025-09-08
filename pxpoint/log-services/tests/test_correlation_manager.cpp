#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "correlation_manager.h"
#include <thread>
#include <cstdlib>

using namespace logservices;

TEST_CASE("CorrelationManager basic functionality", "[correlation]") {
    auto& manager = CorrelationManager::getInstance();
    manager.reset();  // Ensure clean state
    
    SECTION("Generate pipeline ID") {
        std::string pipeline_id = manager.generatePipelineId();
        REQUIRE_FALSE(pipeline_id.empty());
        REQUIRE(pipeline_id.find("pipeline") != std::string::npos);
    }
    
    SECTION("Generate process ID") {
        std::string process_id = manager.generateProcessId("test-process");
        REQUIRE_FALSE(process_id.empty());
        REQUIRE(process_id.find("proc") != std::string::npos);
        REQUIRE(process_id.find("test-process") != std::string::npos);
    }
    
    SECTION("Generate activity ID") {
        std::string activity_id = manager.generateActivityId("test-activity");
        REQUIRE_FALSE(activity_id.empty());
        REQUIRE(activity_id.find("act") != std::string::npos);
        REQUIRE(activity_id.find("test-activity") != std::string::npos);
    }
    
    SECTION("Set and get pipeline ID") {
        manager.setPipelineId("test-pipeline-123");
        REQUIRE(manager.getPipelineId() == "test-pipeline-123");
    }
    
    SECTION("Set and get process ID") {
        manager.setProcessId("test-process-456");
        REQUIRE(manager.getProcessId() == "test-process-456");
    }
    
    SECTION("Thread-local activity ID") {
        manager.setActivityId("test-activity-789");
        REQUIRE(manager.getActivityId() == "test-activity-789");
        
        manager.clearActivityId();
        REQUIRE(manager.getActivityId().empty());
    }
}

TEST_CASE("CorrelationManager environment integration", "[correlation][environment]") {
    auto& manager = CorrelationManager::getInstance();
    manager.reset();
    
    // Clean up environment first
    unsetenv("LOG_PIPELINE_ID");
    unsetenv("LOG_PROCESS_ID");
    
    SECTION("Save to environment") {
        manager.setPipelineId("env-test-pipeline");
        manager.setProcessId("env-test-process");
        manager.saveToEnvironment();
        
        const char* pipeline_env = std::getenv("LOG_PIPELINE_ID");
        const char* process_env = std::getenv("LOG_PROCESS_ID");
        
        REQUIRE(pipeline_env != nullptr);
        REQUIRE(process_env != nullptr);
        REQUIRE(std::string(pipeline_env) == "env-test-pipeline");
        REQUIRE(std::string(process_env) == "env-test-process");
        
        // Clean up
        unsetenv("LOG_PIPELINE_ID");
        unsetenv("LOG_PROCESS_ID");
    }
    
    SECTION("Load from environment") {
        setenv("LOG_PIPELINE_ID", "loaded-pipeline", 1);
        setenv("LOG_PROCESS_ID", "loaded-process", 1);
        
        manager.loadFromEnvironment();
        
        REQUIRE(manager.getPipelineId() == "loaded-pipeline");
        REQUIRE(manager.getProcessId() == "loaded-process");
        
        // Clean up
        unsetenv("LOG_PIPELINE_ID");
        unsetenv("LOG_PROCESS_ID");
    }
}

TEST_CASE("CorrelationManager custom configuration", "[correlation][config]") {
    auto& manager = CorrelationManager::getInstance();
    manager.reset();
    
    SECTION("Custom prefixes") {
        CorrelationConfig config;
        config.pipeline_id_prefix = "custom-pipeline";
        config.process_id_prefix = "custom-proc";
        config.activity_id_prefix = "custom-act";
        
        manager.configure(config);
        
        std::string pipeline_id = manager.generatePipelineId();
        std::string process_id = manager.generateProcessId("test");
        std::string activity_id = manager.generateActivityId("test");
        
        REQUIRE(pipeline_id.find("custom-pipeline") == 0);
        REQUIRE(process_id.find("custom-proc") != std::string::npos);
        REQUIRE(activity_id.find("custom-act") != std::string::npos);
    }
    
    SECTION("Custom environment variables") {
        CorrelationConfig config;
        config.env_var_pipeline = "CUSTOM_PIPELINE";
        config.env_var_process = "CUSTOM_PROCESS";
        
        manager.configure(config);
        manager.setPipelineId("custom-env-pipeline");
        manager.setProcessId("custom-env-process");
        manager.saveToEnvironment();
        
        const char* pipeline_env = std::getenv("CUSTOM_PIPELINE");
        const char* process_env = std::getenv("CUSTOM_PROCESS");
        
        REQUIRE(pipeline_env != nullptr);
        REQUIRE(process_env != nullptr);
        REQUIRE(std::string(pipeline_env) == "custom-env-pipeline");
        REQUIRE(std::string(process_env) == "custom-env-process");
        
        // Clean up
        unsetenv("CUSTOM_PIPELINE");
        unsetenv("CUSTOM_PROCESS");
    }
    
    SECTION("Custom ID generators") {
        CorrelationConfig config;
        config.pipeline_id_generator = []() { return "custom-generated-pipeline"; };
        config.process_id_generator = [](const std::string& type) { 
            return "custom-generated-" + type; 
        };
        config.activity_id_generator = [](const std::string& name) { 
            return "custom-generated-" + name; 
        };
        
        manager.configure(config);
        
        REQUIRE(manager.generatePipelineId() == "custom-generated-pipeline");
        REQUIRE(manager.generateProcessId("test") == "custom-generated-test");
        REQUIRE(manager.generateActivityId("test") == "custom-generated-test");
    }
}

TEST_CASE("CorrelationManager full correlation context", "[correlation][context]") {
    auto& manager = CorrelationManager::getInstance();
    manager.reset();
    
    manager.setPipelineId("test-pipeline-123");
    manager.setProcessId("test-process-456");
    manager.setActivityId("test-activity-789");
    
    SECTION("Full correlation ID string") {
        std::string full_id = manager.getFullCorrelationId();
        REQUIRE(full_id.find("pipeline:test-pipeline-123") != std::string::npos);
        REQUIRE(full_id.find("process:test-process-456") != std::string::npos);
        REQUIRE(full_id.find("activity:test-activity-789") != std::string::npos);
    }
    
    SECTION("Correlation context map") {
        auto context = manager.getCorrelationContext();
        REQUIRE(context["pipeline_id"] == "test-pipeline-123");
        REQUIRE(context["process_id"] == "test-process-456");
        REQUIRE(context["activity_id"] == "test-activity-789");
    }
}

TEST_CASE("ActivityScope RAII behavior", "[correlation][scope]") {
    auto& manager = CorrelationManager::getInstance();
    manager.reset();
    manager.clearActivityId();
    
    REQUIRE(manager.getActivityId().empty());
    
    SECTION("Activity scope sets and clears activity ID") {
        {
            ActivityScope scope("test-activity");
            REQUIRE_FALSE(manager.getActivityId().empty());
            REQUIRE(manager.getActivityId().find("test-activity") != std::string::npos);
        }
        // Activity ID should be cleared after scope ends
        REQUIRE(manager.getActivityId().empty());
    }
    
    SECTION("Nested activity scopes") {
        {
            ActivityScope outer_scope("outer-activity");
            std::string outer_id = manager.getActivityId();
            
            {
                ActivityScope inner_scope("inner-activity");
                std::string inner_id = manager.getActivityId();
                
                REQUIRE(inner_id != outer_id);
                REQUIRE(inner_id.find("inner-activity") != std::string::npos);
            }
            
            // Should restore outer activity ID
            REQUIRE(manager.getActivityId() == outer_id);
        }
        
        // Should be empty after both scopes end
        REQUIRE(manager.getActivityId().empty());
    }
    
    SECTION("Activity scope with context") {
        ActivityScope scope("context-activity", {{"key1", "value1"}, {"key2", "value2"}});
        
        REQUIRE_FALSE(manager.getActivityId().empty());
        REQUIRE(scope.getActivityId().find("context-activity") != std::string::npos);
        
        scope.addContext("key3", "value3");
        // Note: Context is stored in the scope but doesn't affect the correlation manager directly
    }
}

TEST_CASE("ProcessScope RAII behavior", "[correlation][scope]") {
    auto& manager = CorrelationManager::getInstance();
    manager.reset();
    
    // Clean environment
    unsetenv("LOG_PIPELINE_ID");
    unsetenv("LOG_PROCESS_ID");
    
    SECTION("Process scope creates pipeline and process IDs") {
        REQUIRE(manager.getPipelineId().empty());
        REQUIRE(manager.getProcessId().empty());
        
        {
            ProcessScope scope("test-process");
            
            REQUIRE_FALSE(manager.getPipelineId().empty());
            REQUIRE_FALSE(manager.getProcessId().empty());
            REQUIRE(scope.getProcessId().find("test-process") != std::string::npos);
        }
        
        // IDs should persist after scope (they're not cleared by ProcessScope destructor)
        REQUIRE_FALSE(manager.getPipelineId().empty());
        REQUIRE_FALSE(manager.getProcessId().empty());
    }
    
    SECTION("Process scope loads from environment") {
        setenv("LOG_PIPELINE_ID", "env-pipeline-123", 1);
        setenv("LOG_PROCESS_ID", "env-process-456", 1);
        
        {
            ProcessScope scope("env-test-process");
            
            // Should load existing pipeline ID from environment
            REQUIRE(manager.getPipelineId() == "env-pipeline-123");
            // But should generate new process ID
            REQUIRE(scope.getProcessId().find("env-test-process") != std::string::npos);
        }
        
        // Clean up
        unsetenv("LOG_PIPELINE_ID");
        unsetenv("LOG_PROCESS_ID");
    }
}

TEST_CASE("Thread safety of correlation manager", "[correlation][threading]") {
    auto& manager = CorrelationManager::getInstance();
    manager.reset();
    
    SECTION("Activity IDs are thread-local") {
        std::string main_activity_id;
        std::string thread_activity_id;
        
        // Set activity in main thread
        manager.setActivityId("main-activity");
        main_activity_id = manager.getActivityId();
        
        // Create thread and set different activity
        std::thread test_thread([&]() {
            manager.setActivityId("thread-activity");
            thread_activity_id = manager.getActivityId();
        });
        
        test_thread.join();
        
        // Main thread should still have its activity ID
        REQUIRE(manager.getActivityId() == main_activity_id);
        REQUIRE(main_activity_id.find("main-activity") != std::string::npos);
        REQUIRE(thread_activity_id.find("thread-activity") != std::string::npos);
        REQUIRE(main_activity_id != thread_activity_id);
    }
    
    SECTION("Pipeline and process IDs are shared across threads") {
        manager.setPipelineId("shared-pipeline");
        manager.setProcessId("shared-process");
        
        std::string thread_pipeline_id;
        std::string thread_process_id;
        
        std::thread test_thread([&]() {
            thread_pipeline_id = manager.getPipelineId();
            thread_process_id = manager.getProcessId();
        });
        
        test_thread.join();
        
        REQUIRE(thread_pipeline_id == "shared-pipeline");
        REQUIRE(thread_process_id == "shared-process");
    }
}