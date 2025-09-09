using System.Collections.Generic;
using FluentAssertions;
using LogServices.Correlation;
using Xunit;

namespace LogServices.Tests
{
    public class CorrelationManagerTests
    {
        private readonly CorrelationManager _manager;

        public CorrelationManagerTests()
        {
            _manager = CorrelationManager.Instance;
            _manager.Reset(); // Start with clean state
        }

        [Fact]
        public void GetInstance_ShouldReturnSameSingletonInstance()
        {
            // Arrange & Act
            var instance1 = CorrelationManager.Instance;
            var instance2 = CorrelationManager.Instance;

            // Assert
            instance1.Should().BeSameAs(instance2);
        }

        [Fact]
        public void SetPipelineId_ShouldStoreAndRetrieveCorrectly()
        {
            // Arrange
            var pipelineId = "test-pipeline-123";

            // Act
            _manager.SetPipelineId(pipelineId);
            var result = _manager.GetPipelineId();

            // Assert
            result.Should().Be(pipelineId);
        }

        [Fact]
        public void SetProcessId_ShouldStoreAndRetrieveCorrectly()
        {
            // Arrange
            var processId = "test-process-456";

            // Act
            _manager.SetProcessId(processId);
            var result = _manager.GetProcessId();

            // Assert
            result.Should().Be(processId);
        }

        [Fact]
        public void SetActivityId_ShouldStoreAndRetrieveCorrectly()
        {
            // Arrange
            var activityId = "test-activity-789";

            // Act
            _manager.SetActivityId(activityId);
            var result = _manager.GetActivityId();

            // Assert
            result.Should().Be(activityId);
        }

        [Fact]
        public void GeneratePipelineId_ShouldReturnValidFormat()
        {
            // Act
            var pipelineId = _manager.GeneratePipelineId();

            // Assert
            pipelineId.Should().NotBeNullOrEmpty();
            pipelineId.Should().StartWith("pipeline-");
            pipelineId.Split('-').Should().HaveCount(3); // prefix-timestamp-uuid
        }

        [Fact]
        public void GenerateProcessId_ShouldReturnValidFormat()
        {
            // Arrange
            var processType = "test-processor";

            // Act
            var processId = _manager.GenerateProcessId(processType);

            // Assert
            processId.Should().NotBeNullOrEmpty();
            processId.Should().Contain(processType);
        }

        [Fact]
        public void GenerateActivityId_ShouldReturnValidFormat()
        {
            // Arrange
            var activityName = "test-activity";

            // Act
            var activityId = _manager.GenerateActivityId(activityName);

            // Assert
            activityId.Should().NotBeNullOrEmpty();
            activityId.Should().Contain(activityName);
        }

        [Fact]
        public void GetFullCorrelationId_WithAllIds_ShouldReturnFormattedString()
        {
            // Arrange
            _manager.SetPipelineId("pipeline-123");
            _manager.SetProcessId("process-456");
            _manager.SetActivityId("activity-789");

            // Act
            var fullId = _manager.GetFullCorrelationId();

            // Assert
            fullId.Should().Be("pipeline:pipeline-123|process:process-456|activity:activity-789");
        }

        [Fact]
        public void GetFullCorrelationId_WithPartialIds_ShouldReturnOnlyAvailable()
        {
            // Arrange
            _manager.SetPipelineId("pipeline-123");
            // Process and Activity IDs not set

            // Act
            var fullId = _manager.GetFullCorrelationId();

            // Assert
            fullId.Should().Be("pipeline:pipeline-123");
        }

        [Fact]
        public void GetCorrelationContext_ShouldReturnDictionaryWithIds()
        {
            // Arrange
            _manager.SetPipelineId("pipeline-123");
            _manager.SetProcessId("process-456");
            _manager.SetActivityId("activity-789");

            // Act
            var context = _manager.GetCorrelationContext();

            // Assert
            context.Should().ContainKey("pipeline_id").WhoseValue.Should().Be("pipeline-123");
            context.Should().ContainKey("process_id").WhoseValue.Should().Be("process-456");
            context.Should().ContainKey("activity_id").WhoseValue.Should().Be("activity-789");
        }

        [Fact]
        public void ClearActivityId_ShouldRemoveActivityId()
        {
            // Arrange
            _manager.SetActivityId("activity-123");
            _manager.GetActivityId().Should().Be("activity-123");

            // Act
            _manager.ClearActivityId();

            // Assert
            _manager.GetActivityId().Should().BeEmpty();
        }

        [Fact]
        public void Reset_ShouldClearAllIds()
        {
            // Arrange
            _manager.SetPipelineId("pipeline-123");
            _manager.SetProcessId("process-456");
            _manager.SetActivityId("activity-789");

            // Act
            _manager.Reset();

            // Assert
            _manager.GetPipelineId().Should().BeEmpty();
            _manager.GetProcessId().Should().BeEmpty();
            _manager.GetActivityId().Should().BeEmpty();
        }

        [Fact]
        public void LoadFromEnvironment_ShouldLoadFromEnvVars()
        {
            // Arrange
            Environment.SetEnvironmentVariable("LOG_PIPELINE_ID", "env-pipeline-123");
            Environment.SetEnvironmentVariable("LOG_PROCESS_ID", "env-process-456");

            try
            {
                // Act
                _manager.LoadFromEnvironment();

                // Assert
                _manager.GetPipelineId().Should().Be("env-pipeline-123");
                _manager.GetProcessId().Should().Be("env-process-456");
            }
            finally
            {
                // Cleanup
                Environment.SetEnvironmentVariable("LOG_PIPELINE_ID", null);
                Environment.SetEnvironmentVariable("LOG_PROCESS_ID", null);
            }
        }

        [Fact]
        public void SaveToEnvironment_ShouldSetEnvVars()
        {
            // Arrange
            _manager.SetPipelineId("save-pipeline-123");
            _manager.SetProcessId("save-process-456");

            try
            {
                // Act
                _manager.SaveToEnvironment();

                // Assert
                Environment.GetEnvironmentVariable("LOG_PIPELINE_ID").Should().Be("save-pipeline-123");
                Environment.GetEnvironmentVariable("LOG_PROCESS_ID").Should().Be("save-process-456");
            }
            finally
            {
                // Cleanup
                Environment.SetEnvironmentVariable("LOG_PIPELINE_ID", null);
                Environment.SetEnvironmentVariable("LOG_PROCESS_ID", null);
            }
        }
    }
}