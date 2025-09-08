using System;
using System.Collections.Generic;
using LogServices.Correlation;
using LogServices.Logging;

namespace LogServices.Examples
{
    /// <summary>
    /// Example demonstrating how to configure the LogServices library
    /// </summary>
    public class ConfigurationExample
    {
        public static void Main(string[] args)
        {
            // Example 1: Load configuration from JSON files
            ConfigureFromJsonFiles();
            
            // Example 2: Programmatic configuration
            ConfigureProgrammatically();
            
            // Example 3: Environment-based configuration
            ConfigureFromEnvironment();
            
            // Example 4: Combined configuration with custom generators
            ConfigureWithCustomGenerators();
        }
        
        /// <summary>
        /// Load configuration from JSON files (recommended approach)
        /// </summary>
        static void ConfigureFromJsonFiles()
        {
            Console.WriteLine("=== Configuration from JSON Files ===");
            
            try
            {
                // Load correlation configuration
                CorrelationManager.Instance.LoadConfigFromJson("config/correlation.json");
                
                // Load logger configuration
                var logger = StructuredLogger.Instance;
                logger.LoadConfigFromJson("config/logging.json");
                logger.Initialize("ConfigExample", LogLevel.Info);
                
                // Test the configuration
                using var processScope = new ProcessCorrelationScope("JsonConfigTest");
                logger.LogInfo("Configuration", "Successfully loaded configuration from JSON files");
                
                using var activityScope = new ActivityCorrelationScope("TestActivity");
                logger.LogInfo("Activity", "Testing JSON-configured logging", new Dictionary<string, object>
                {
                    {"config_source", "json_files"},
                    {"process_type", "ConfigExample"}
                });
                
                logger.Shutdown();
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error loading JSON configuration: {ex.Message}");
            }
        }
        
        /// <summary>
        /// Programmatic configuration for full control
        /// </summary>
        static void ConfigureProgrammatically()
        {
            Console.WriteLine("\n=== Programmatic Configuration ===");
            
            // Configure correlation manager
            var correlationConfig = new CorrelationConfig
            {
                PipelineIdPrefix = "custom-pipeline",
                ProcessIdPrefix = "custom-proc",
                ActivityIdPrefix = "custom-act",
                EnvVarPipeline = "CUSTOM_PIPELINE_ID",
                EnvVarProcess = "CUSTOM_PROCESS_ID",
                AutoGeneratePipeline = true,
                AutoGenerateProcess = true,
                PropagateToEnvironment = true
            };
            
            CorrelationManager.Instance.Configure(correlationConfig);
            
            // Configure logger with custom sinks
            var loggerConfig = new LoggerConfig
            {
                Name = "ProgrammaticExample",
                Level = LogLevel.Debug,
                AsyncLogging = true,
                AsyncQueueSize = 4096,
                LogDirectory = "/tmp/custom-logs",
                AutoAddCorrelation = true,
                FlushOnError = true,
                FlushIntervalSeconds = 3,
                Sinks = new List<SinkConfig>
                {
                    new SinkConfig
                    {
                        Type = SinkConfig.SinkType.Console,
                        Name = "console",
                        Level = LogLevel.Info,
                        ColorMode = true
                    },
                    new SinkConfig
                    {
                        Type = SinkConfig.SinkType.DailyFile,
                        Name = "debug_file",
                        Level = LogLevel.Debug,
                        FilePath = "/tmp/custom-logs/debug-{process_type}-{date}.log",
                        MaxFileSize = 5 * 1024 * 1024, // 5MB
                        MaxFiles = 10
                    },
                    new SinkConfig
                    {
                        Type = SinkConfig.SinkType.RotatingFile,
                        Name = "error_file",
                        Level = LogLevel.Error,
                        FilePath = "/tmp/custom-logs/errors-{process_type}.log",
                        MaxFileSize = 2 * 1024 * 1024, // 2MB
                        MaxFiles = 5
                    }
                }
            };
            
            var logger = StructuredLogger.Instance;
            logger.Configure(loggerConfig);
            logger.Initialize("ProgrammaticExample", LogLevel.Debug);
            
            // Test the configuration
            using var processScope = new ProcessCorrelationScope("ProgrammaticTest");
            logger.LogDebug("Configuration", "Programmatic configuration completed");
            
            using var activityScope = new ActivityCorrelationScope("DetailedTest");
            logger.LogInfo("Test", "Testing programmatically configured logging", new Dictionary<string, object>
            {
                {"config_source", "programmatic"},
                {"custom_field", "example_value"},
                {"test_number", 42}
            });
            
            // Test performance logging
            using var timer = new PerformanceTimer("ConfigurationTest", new Dictionary<string, object>
            {
                {"test_type", "programmatic_config"}
            });
            
            System.Threading.Thread.Sleep(100); // Simulate work
            
            logger.Shutdown();
        }
        
        /// <summary>
        /// Environment-based configuration for containerized deployments
        /// </summary>
        static void ConfigureFromEnvironment()
        {
            Console.WriteLine("\n=== Environment-based Configuration ===");
            
            // Set environment variables (normally done by container orchestrator)
            Environment.SetEnvironmentVariable("LOG_LEVEL", "Warning");
            Environment.SetEnvironmentVariable("LOG_DIRECTORY", "/tmp/env-logs");
            Environment.SetEnvironmentVariable("ASYNC_LOGGING", "false");
            Environment.SetEnvironmentVariable("CORRELATION_PREFIX", "env");
            
            // Create configuration based on environment
            var logLevel = Enum.TryParse<LogLevel>(Environment.GetEnvironmentVariable("LOG_LEVEL"), out var level) ? level : LogLevel.Info;
            var logDirectory = Environment.GetEnvironmentVariable("LOG_DIRECTORY") ?? "/tmp/default-logs";
            var asyncLogging = bool.TryParse(Environment.GetEnvironmentVariable("ASYNC_LOGGING"), out var async) && async;
            var correlationPrefix = Environment.GetEnvironmentVariable("CORRELATION_PREFIX") ?? "pipeline";
            
            // Configure correlation
            var correlationConfig = new CorrelationConfig
            {
                PipelineIdPrefix = correlationPrefix,
                ProcessIdPrefix = $"{correlationPrefix}-proc",
                ActivityIdPrefix = $"{correlationPrefix}-act"
            };
            CorrelationManager.Instance.Configure(correlationConfig);
            
            // Configure logger
            var loggerConfig = new LoggerConfig
            {
                Name = "EnvironmentExample",
                Level = logLevel,
                AsyncLogging = asyncLogging,
                LogDirectory = logDirectory,
                AutoAddCorrelation = true
            };
            
            var logger = StructuredLogger.Instance;
            logger.Configure(loggerConfig);
            logger.Initialize("EnvironmentExample", logLevel);
            
            // Test the configuration
            using var processScope = new ProcessCorrelationScope("EnvironmentTest");
            logger.LogInfo("Configuration", "Environment-based configuration completed", new Dictionary<string, object>
            {
                {"log_level", logLevel.ToString()},
                {"log_directory", logDirectory},
                {"async_logging", asyncLogging},
                {"correlation_prefix", correlationPrefix}
            });
            
            logger.Shutdown();
        }
        
        /// <summary>
        /// Advanced configuration with custom ID generators
        /// </summary>
        static void ConfigureWithCustomGenerators()
        {
            Console.WriteLine("\n=== Custom Generator Configuration ===");
            
            // Custom correlation configuration with generators
            var correlationConfig = new CorrelationConfig
            {
                PipelineIdPrefix = "batch",
                ProcessIdPrefix = "worker",
                ActivityIdPrefix = "task",
                
                // Custom pipeline ID generator (e.g., using timestamp + machine name)
                PipelineIdGenerator = () =>
                {
                    var timestamp = DateTimeOffset.UtcNow.ToString("yyyyMMdd-HHmmss");
                    var machine = Environment.MachineName.ToLower();
                    return $"batch-{timestamp}-{machine}";
                },
                
                // Custom process ID generator (includes parent pipeline)
                ProcessIdGenerator = (processType) =>
                {
                    var parentId = CorrelationManager.Instance.GetPipelineId();
                    var processId = Guid.NewGuid().ToString("N")[..6];
                    return $"{parentId}-{processType}-{processId}";
                },
                
                // Custom activity ID generator (includes timing)
                ActivityIdGenerator = (activityName) =>
                {
                    var processId = CorrelationManager.Instance.GetProcessId();
                    var timestamp = DateTimeOffset.UtcNow.ToUnixTimeMilliseconds();
                    return $"{processId}-{activityName}-{timestamp}";
                }
            };
            
            CorrelationManager.Instance.Configure(correlationConfig);
            
            // Standard logger configuration
            var logger = StructuredLogger.Instance;
            logger.Initialize("CustomGenExample", LogLevel.Info);
            
            // Test custom generators
            using var processScope = new ProcessCorrelationScope("CustomGenTest");
            logger.LogInfo("Configuration", "Custom generator configuration completed");
            
            // Multiple activities to show custom ID generation
            for (int i = 1; i <= 3; i++)
            {
                using var activityScope = new ActivityCorrelationScope($"TestActivity{i}");
                logger.LogInfo("Activity", $"Testing custom activity ID generation #{i}", new Dictionary<string, object>
                {
                    {"activity_number", i},
                    {"custom_correlation", true}
                });
                
                System.Threading.Thread.Sleep(10); // Small delay to show timestamp differences
            }
            
            logger.Shutdown();
        }
    }
}