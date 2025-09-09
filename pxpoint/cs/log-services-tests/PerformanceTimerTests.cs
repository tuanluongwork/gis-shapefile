using System.Collections.Generic;
using System.Threading.Tasks;
using FluentAssertions;
using LogServices.Logging;
using Xunit;

namespace LogServices.Tests
{
    public class PerformanceTimerTests : IDisposable
    {
        private readonly StructuredLogger _logger;
        private readonly string _testLogDirectory;

        public PerformanceTimerTests()
        {
            _logger = StructuredLogger.Instance;
            _testLogDirectory = Path.Combine(Path.GetTempPath(), $"perf-test-logs-{Guid.NewGuid()}");
            
            var config = new LoggerConfig
            {
                Name = "performance-test",
                Level = LogLevel.Debug,
                AsyncLogging = false,
                LogDirectory = _testLogDirectory,
                AutoAddCorrelation = false,
                Sinks = new List<SinkConfig>
                {
                    new SinkConfig
                    {
                        Type = SinkConfig.SinkType.File,
                        Name = "perf_test_file",
                        Level = LogLevel.Debug,
                        FilePath = Path.Combine(_testLogDirectory, "performance-test.log")
                    }
                }
            };
            
            _logger.Configure(config);
            _logger.Initialize("performance-test-process", LogLevel.Debug);
        }

        [Fact]
        public void PerformanceTimer_WithManualStop_ShouldLogPerformanceData()
        {
            // Arrange
            var operationName = "test-operation";
            var context = new Dictionary<string, object> { { "test_context", "test_value" } };

            // Act
            var timer = new PerformanceTimer(operationName, context);
            Thread.Sleep(10); // Simulate some work
            timer.Stop();
            _logger.Flush();

            // Assert
            var logFile = Path.Combine(_testLogDirectory, "performance-test.log");
            File.Exists(logFile).Should().BeTrue();
            
            var logContent = File.ReadAllText(logFile);
            logContent.Should().Contain("Operation completed");
            logContent.Should().Contain(operationName);
            logContent.Should().Contain("performance");
            logContent.Should().Contain("test_context");
        }

        [Fact]
        public void PerformanceTimer_WithDispose_ShouldLogPerformanceData()
        {
            // Arrange
            var operationName = "dispose-test-operation";

            // Act
            using (var timer = new PerformanceTimer(operationName))
            {
                Thread.Sleep(10); // Simulate some work
            } // Dispose should trigger Stop()
            
            _logger.Flush();

            // Assert
            var logFile = Path.Combine(_testLogDirectory, "performance-test.log");
            var logContent = File.ReadAllText(logFile);
            logContent.Should().Contain("Operation completed");
            logContent.Should().Contain(operationName);
        }

        [Fact]
        public void PerformanceTimer_AddContext_ShouldIncludeInLog()
        {
            // Arrange
            var operationName = "context-test-operation";
            var timer = new PerformanceTimer(operationName);

            // Act
            timer.AddContext("additional_key", "additional_value");
            timer.AddMetric("custom_metric", 42.5);
            timer.Stop();
            _logger.Flush();

            // Assert
            var logFile = Path.Combine(_testLogDirectory, "performance-test.log");
            var logContent = File.ReadAllText(logFile);
            logContent.Should().Contain("additional_key");
            logContent.Should().Contain("custom_metric");
            logContent.Should().Contain("42.5");
        }

        [Fact]
        public void PerformanceTimer_MultipleStops_ShouldOnlyLogOnce()
        {
            // Arrange
            var operationName = "multiple-stop-test";
            var timer = new PerformanceTimer(operationName);

            // Act
            timer.Stop();
            var logContentAfterFirst = GetLogContent();
            
            timer.Stop(); // Second stop should not log again
            _logger.Flush();
            var logContentAfterSecond = GetLogContent();

            // Assert
            logContentAfterFirst.Should().Contain("Operation completed");
            logContentAfterSecond.Should().Be(logContentAfterFirst); // Should be identical
        }

        [Fact]
        public void StaticLogHelper_StartPerformanceTimer_ShouldCreateValidTimer()
        {
            // Arrange
            var operationName = "static-helper-test";
            var context = new Dictionary<string, object> { { "helper_test", true } };

            // Act
            using (var timer = Log.StartPerformanceTimer(operationName, context))
            {
                Thread.Sleep(5);
            }
            
            _logger.Flush();

            // Assert
            var logFile = Path.Combine(_testLogDirectory, "performance-test.log");
            var logContent = File.ReadAllText(logFile);
            logContent.Should().Contain("Operation completed");
            logContent.Should().Contain(operationName);
            logContent.Should().Contain("helper_test");
        }

        private string GetLogContent()
        {
            _logger.Flush();
            var logFile = Path.Combine(_testLogDirectory, "performance-test.log");
            return File.Exists(logFile) ? File.ReadAllText(logFile) : string.Empty;
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