using System.Collections.Generic;
using FluentAssertions;
using LogServices.Correlation;
using Xunit;

namespace LogServices.Tests
{
    public class ProcessCorrelationScopeTests
    {
        private readonly CorrelationManager _manager;

        public ProcessCorrelationScopeTests()
        {
            _manager = CorrelationManager.Instance;
            _manager.Reset(); // Start with clean state
        }

        [Fact]
        public void ProcessCorrelationScope_ShouldGenerateAndSetProcessId()
        {
            // Arrange
            var processType = "test-processor";

            // Act
            using (var scope = new ProcessCorrelationScope(processType))
            {
                // Assert
                var processId = _manager.GetProcessId();
                processId.Should().NotBeEmpty();
                processId.Should().Contain(processType);
                
                scope.GetProcessId().Should().Be(processId);
            }
        }

        [Fact]
        public void ProcessCorrelationScope_ShouldAutoGeneratePipelineIfMissing()
        {
            // Arrange
            var processType = "auto-pipeline-processor";
            _manager.GetPipelineId().Should().BeEmpty(); // No pipeline initially

            // Act
            using (var scope = new ProcessCorrelationScope(processType))
            {
                // Assert
                var pipelineId = _manager.GetPipelineId();
                pipelineId.Should().NotBeEmpty();
                pipelineId.Should().StartWith("pipeline-");
                
                var processId = _manager.GetProcessId();
                processId.Should().Contain(pipelineId); // Process ID should include pipeline ID
            }
        }

        [Fact]
        public void ProcessCorrelationScope_WithExistingPipeline_ShouldUsePipelineId()
        {
            // Arrange
            var existingPipelineId = "existing-pipeline-123";
            var processType = "existing-pipeline-processor";
            _manager.SetPipelineId(existingPipelineId);

            // Act
            using (var scope = new ProcessCorrelationScope(processType))
            {
                // Assert
                var pipelineId = _manager.GetPipelineId();
                pipelineId.Should().Be(existingPipelineId);
                
                var processId = _manager.GetProcessId();
                processId.Should().Contain(existingPipelineId);
                processId.Should().Contain(processType);
            }
        }

        [Fact]
        public void ProcessCorrelationScope_WithContext_ShouldStoreContext()
        {
            // Arrange
            var processType = "context-processor";
            var context = new Dictionary<string, object>
            {
                { "batch_size", 100 },
                { "config_version", "1.2.3" }
            };

            // Act
            using (var scope = new ProcessCorrelationScope(processType, context))
            {
                // Assert
                var scopeContext = scope.GetContext();
                scopeContext.Should().ContainKey("batch_size").WhoseValue.Should().Be(100);
                scopeContext.Should().ContainKey("config_version").WhoseValue.Should().Be("1.2.3");
            }
        }

        [Fact]
        public void ProcessCorrelationScope_LoadFromEnvironment_ShouldUseExistingIds()
        {
            // Arrange
            var envPipelineId = "env-pipeline-456";
            var envProcessId = "env-process-789";
            var processType = "env-test-processor";
            
            Environment.SetEnvironmentVariable("LOG_PIPELINE_ID", envPipelineId);
            Environment.SetEnvironmentVariable("LOG_PROCESS_ID", envProcessId);

            try
            {
                // Act
                using (var scope = new ProcessCorrelationScope(processType))
                {
                    // Assert
                    _manager.GetPipelineId().Should().Be(envPipelineId);
                    // Process ID should be newly generated even if one exists in env
                    _manager.GetProcessId().Should().NotBe(envProcessId);
                    _manager.GetProcessId().Should().Contain(processType);
                }
            }
            finally
            {
                Environment.SetEnvironmentVariable("LOG_PIPELINE_ID", null);
                Environment.SetEnvironmentVariable("LOG_PROCESS_ID", null);
            }
        }

        [Fact]
        public void ProcessCorrelationScope_SaveToEnvironment_ShouldSetEnvVars()
        {
            // Arrange
            var processType = "env-save-processor";

            try
            {
                // Act
                using (var scope = new ProcessCorrelationScope(processType))
                {
                    var pipelineId = _manager.GetPipelineId();
                    var processId = _manager.GetProcessId();

                    // Assert
                    Environment.GetEnvironmentVariable("LOG_PIPELINE_ID").Should().Be(pipelineId);
                    Environment.GetEnvironmentVariable("LOG_PROCESS_ID").Should().Be(processId);
                }
            }
            finally
            {
                Environment.SetEnvironmentVariable("LOG_PIPELINE_ID", null);
                Environment.SetEnvironmentVariable("LOG_PROCESS_ID", null);
            }
        }

        [Fact]
        public void ProcessCorrelationScope_Dispose_WithCreatedPipeline_ShouldCleanupEnvVars()
        {
            // Arrange
            var processType = "cleanup-processor";
            _manager.GetPipelineId().Should().BeEmpty(); // No initial pipeline

            try
            {
                // Act
                using (var scope = new ProcessCorrelationScope(processType))
                {
                    // Pipeline should be created and env vars set
                    Environment.GetEnvironmentVariable("LOG_PIPELINE_ID").Should().NotBeNullOrEmpty();
                    Environment.GetEnvironmentVariable("LOG_PROCESS_ID").Should().NotBeNullOrEmpty();
                }

                // Assert - after dispose, env vars should be cleaned up
                Environment.GetEnvironmentVariable("LOG_PIPELINE_ID").Should().BeNull();
                Environment.GetEnvironmentVariable("LOG_PROCESS_ID").Should().BeNull();
            }
            finally
            {
                // Extra cleanup just in case
                Environment.SetEnvironmentVariable("LOG_PIPELINE_ID", null);
                Environment.SetEnvironmentVariable("LOG_PROCESS_ID", null);
            }
        }

        [Fact]
        public void ProcessCorrelationScope_Dispose_WithExistingPipeline_ShouldNotCleanupEnvVars()
        {
            // Arrange
            var existingPipelineId = "existing-cleanup-pipeline";
            var processType = "no-cleanup-processor";
            _manager.SetPipelineId(existingPipelineId);

            try
            {
                // Act
                using (var scope = new ProcessCorrelationScope(processType))
                {
                    Environment.GetEnvironmentVariable("LOG_PIPELINE_ID").Should().Be(existingPipelineId);
                }

                // Assert - env vars should still be there since we didn't create the pipeline
                Environment.GetEnvironmentVariable("LOG_PIPELINE_ID").Should().Be(existingPipelineId);
            }
            finally
            {
                Environment.SetEnvironmentVariable("LOG_PIPELINE_ID", null);
                Environment.SetEnvironmentVariable("LOG_PROCESS_ID", null);
            }
        }

        [Fact]
        public void ProcessCorrelationScope_GetContext_ShouldReturnCopy()
        {
            // Arrange
            var processType = "copy-test-processor";
            var originalContext = new Dictionary<string, object> { { "original", "value" } };

            // Act
            using (var scope = new ProcessCorrelationScope(processType, originalContext))
            {
                var context1 = scope.GetContext();
                var context2 = scope.GetContext();

                // Assert
                context1.Should().NotBeSameAs(context2); // Should be different instances
                context1.Should().BeEquivalentTo(context2); // But with same content
                
                // Modifying returned context should not affect scope's context
                context1["modified"] = "test";
                scope.GetContext().Should().NotContainKey("modified");
            }
        }

        [Fact]
        public void ProcessCorrelationScope_MultipleDispose_ShouldBeIdempotent()
        {
            // Arrange
            var processType = "dispose-test-processor";
            var scope = new ProcessCorrelationScope(processType);

            try
            {
                var envVarAfterConstruction = Environment.GetEnvironmentVariable("LOG_PIPELINE_ID");
                envVarAfterConstruction.Should().NotBeNullOrEmpty();

                // Act
                scope.Dispose();
                var envVarAfterFirstDispose = Environment.GetEnvironmentVariable("LOG_PIPELINE_ID");
                
                scope.Dispose(); // Second dispose should not affect anything
                var envVarAfterSecondDispose = Environment.GetEnvironmentVariable("LOG_PIPELINE_ID");

                // Assert
                envVarAfterFirstDispose.Should().BeNull();
                envVarAfterSecondDispose.Should().Be(envVarAfterFirstDispose);
            }
            finally
            {
                Environment.SetEnvironmentVariable("LOG_PIPELINE_ID", null);
                Environment.SetEnvironmentVariable("LOG_PROCESS_ID", null);
            }
        }
    }
}