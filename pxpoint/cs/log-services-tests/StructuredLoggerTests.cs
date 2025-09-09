using System.Collections.Generic;
using System.IO;
using FluentAssertions;
using LogServices.Correlation;
using LogServices.Logging;
using Xunit;

namespace LogServices.Tests
{
    public class StructuredLoggerTests : IDisposable
    {
        private readonly StructuredLogger _logger;
        private readonly string _testLogDirectory;

        public StructuredLoggerTests()
        {
            _logger = StructuredLogger.Instance;
            _testLogDirectory = Path.Combine(Path.GetTempPath(), $"test-logs-{Guid.NewGuid()}");
            
            // Configure logger for testing
            var config = new LoggerConfig
            {
                Name = "test-logger",
                Level = LogLevel.Debug,
                AsyncLogging = false, // Synchronous for testing
                LogDirectory = _testLogDirectory,
                AutoAddCorrelation = false, // Controlled manually in tests
                Sinks = new List<SinkConfig>
                {
                    new SinkConfig
                    {
                        Type = SinkConfig.SinkType.File,
                        Name = "test_file",
                        Level = LogLevel.Debug,
                        FilePath = Path.Combine(_testLogDirectory, "test.log")
                    }
                }
            };
            
            _logger.Configure(config);
        }

        [Fact]
        public void GetInstance_ShouldReturnSameSingletonInstance()
        {
            // Arrange & Act
            var instance1 = StructuredLogger.Instance;
            var instance2 = StructuredLogger.Instance;

            // Assert
            instance1.Should().BeSameAs(instance2);
        }

        [Fact]
        public void Initialize_ShouldSetProcessTypeAndCreateLogDirectory()
        {
            // Act
            _logger.Initialize("test-process", LogLevel.Debug);

            // Assert
            Directory.Exists(_testLogDirectory).Should().BeTrue();
        }

        [Fact]
        public void SetLevel_ShouldUpdateLoggingLevel()
        {
            // Arrange
            _logger.Initialize("test-process", LogLevel.Info);

            // Act
            _logger.SetLevel(LogLevel.Warning);

            // Assert
            _logger.GetLevel().Should().Be(LogLevel.Warning);
        }

        [Fact]
        public void LogInfo_ShouldWriteToConfiguredSinks()
        {
            // Arrange
            _logger.Initialize("test-process", LogLevel.Debug);
            var testMessage = "Test info message";
            var context = new Dictionary<string, object> { { "key1", "value1" } };

            // Act
            _logger.LogInfo("TestComponent", testMessage, context);
            _logger.Flush();

            // Assert
            var logFile = Path.Combine(_testLogDirectory, "test.log");
            File.Exists(logFile).Should().BeTrue();
            
            var logContent = File.ReadAllText(logFile);
            logContent.Should().Contain(testMessage);
            logContent.Should().Contain("TestComponent");
            logContent.Should().Contain("INFO");
        }

        [Fact]
        public void LogError_WithException_ShouldIncludeExceptionDetails()
        {
            // Arrange
            _logger.Initialize("test-process", LogLevel.Debug);
            var testMessage = "Test error message";
            var exception = new InvalidOperationException("Test exception");

            // Act
            _logger.LogError("ErrorComponent", testMessage, exception);
            _logger.Flush();

            // Assert
            var logFile = Path.Combine(_testLogDirectory, "test.log");
            var logContent = File.ReadAllText(logFile);
            logContent.Should().Contain(testMessage);
            logContent.Should().Contain("InvalidOperationException");
            logContent.Should().Contain("Test exception");
        }

        [Fact]
        public void LogEvent_ShouldIncludeEventTypeInContext()
        {
            // Arrange
            _logger.Initialize("test-process", LogLevel.Debug);
            var eventType = "user_action";
            var description = "User clicked button";

            // Act
            _logger.LogEvent(eventType, description);
            _logger.Flush();

            // Assert
            var logFile = Path.Combine(_testLogDirectory, "test.log");
            var logContent = File.ReadAllText(logFile);
            logContent.Should().Contain(eventType);
            logContent.Should().Contain(description);
            logContent.Should().Contain("Event");
        }

        [Fact]
        public void LogProcessStart_ShouldLogProcessStartEvent()
        {
            // Arrange
            _logger.Initialize("test-process", LogLevel.Debug);
            var processType = "data-processor";
            var config = new Dictionary<string, object> { { "batch_size", 100 } };

            // Act
            _logger.LogProcessStart(processType, config);
            _logger.Flush();

            // Assert
            var logFile = Path.Combine(_testLogDirectory, "test.log");
            var logContent = File.ReadAllText(logFile);
            logContent.Should().Contain("Process started");
            logContent.Should().Contain(processType);
            logContent.Should().Contain("process_start");
        }

        [Fact]
        public void LogProcessEnd_WithSuccess_ShouldLogSuccessfulCompletion()
        {
            // Arrange
            _logger.Initialize("test-process", LogLevel.Debug);
            var processType = "data-processor";
            var metrics = new Dictionary<string, double> { { "processed_records", 500 } };

            // Act
            _logger.LogProcessEnd(processType, success: true, metrics);
            _logger.Flush();

            // Assert
            var logFile = Path.Combine(_testLogDirectory, "test.log");
            var logContent = File.ReadAllText(logFile);
            logContent.Should().Contain("Process completed successfully");
            logContent.Should().Contain(processType);
            logContent.Should().Contain("process_end");
            logContent.Should().Contain("INFO");
        }

        [Fact]
        public void LogProcessEnd_WithFailure_ShouldLogFailureAsError()
        {
            // Arrange
            _logger.Initialize("test-process", LogLevel.Debug);
            var processType = "data-processor";

            // Act
            _logger.LogProcessEnd(processType, success: false);
            _logger.Flush();

            // Assert
            var logFile = Path.Combine(_testLogDirectory, "test.log");
            var logContent = File.ReadAllText(logFile);
            logContent.Should().Contain("Process failed");
            logContent.Should().Contain(processType);
            logContent.Should().Contain("ERROR");
        }

        [Fact]
        public void LogPerformance_ShouldLogOperationMetrics()
        {
            // Arrange
            _logger.Initialize("test-process", LogLevel.Debug);
            var operation = "database_query";
            var durationMs = 150.5;
            var context = new Dictionary<string, object> { { "query_type", "SELECT" } };
            var metrics = new Dictionary<string, double> { { "rows_returned", 25 } };

            // Act
            _logger.LogPerformance(operation, durationMs, context, metrics);
            _logger.Flush();

            // Assert
            var logFile = Path.Combine(_testLogDirectory, "test.log");
            var logContent = File.ReadAllText(logFile);
            logContent.Should().Contain("Operation completed");
            logContent.Should().Contain(operation);
            logContent.Should().Contain("performance");
            logContent.Should().Contain("150.5");
        }

        [Fact]
        public void Log_WithCorrelationEnabled_ShouldIncludeCorrelationData()
        {
            // Arrange
            var correlationManager = CorrelationManager.Instance;
            correlationManager.SetPipelineId("test-pipeline-123");
            correlationManager.SetProcessId("test-process-456");

            var config = new LoggerConfig
            {
                Name = "correlation-test",
                Level = LogLevel.Debug,
                AsyncLogging = false,
                LogDirectory = _testLogDirectory,
                AutoAddCorrelation = true, // Enable correlation
                Sinks = new List<SinkConfig>
                {
                    new SinkConfig
                    {
                        Type = SinkConfig.SinkType.File,
                        Name = "correlation_file",
                        FilePath = Path.Combine(_testLogDirectory, "correlation-test.log")
                    }
                }
            };

            _logger.Configure(config);
            _logger.Initialize("correlation-test-process", LogLevel.Debug);

            // Act
            _logger.LogInfo("TestComponent", "Test message with correlation");
            _logger.Flush();

            // Assert
            var logFile = Path.Combine(_testLogDirectory, "correlation-test.log");
            var logContent = File.ReadAllText(logFile);
            logContent.Should().Contain("test-pipeline-123");
            logContent.Should().Contain("test-process-456");
        }

        public void Dispose()
        {
            try
            {
                _logger?.Flush();
                
                if (Directory.Exists(_testLogDirectory))
                {
                    Directory.Delete(_testLogDirectory, recursive: true);
                }
            }
            catch
            {
                // Ignore cleanup errors in tests
            }
        }
    }
}