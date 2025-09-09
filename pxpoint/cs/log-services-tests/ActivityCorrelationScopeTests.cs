using System.Collections.Generic;
using FluentAssertions;
using LogServices.Correlation;
using Xunit;

namespace LogServices.Tests
{
    public class ActivityCorrelationScopeTests
    {
        private readonly CorrelationManager _manager;

        public ActivityCorrelationScopeTests()
        {
            _manager = CorrelationManager.Instance;
            _manager.Reset(); // Start with clean state
        }

        [Fact]
        public void ActivityCorrelationScope_ShouldSetAndRestoreActivityId()
        {
            // Arrange
            var originalActivityId = "original-activity";
            _manager.SetActivityId(originalActivityId);
            var activityName = "test-activity";

            // Act & Assert
            using (var scope = new ActivityCorrelationScope(activityName))
            {
                // Inside scope, activity ID should be different
                var currentId = _manager.GetActivityId();
                currentId.Should().NotBe(originalActivityId);
                currentId.Should().Contain(activityName);
                
                // Scope should provide access to the activity ID
                scope.GetActivityId().Should().Be(currentId);
            }

            // After scope, original activity ID should be restored
            _manager.GetActivityId().Should().Be(originalActivityId);
        }

        [Fact]
        public void ActivityCorrelationScope_WithEmptyOriginalId_ShouldClearOnDispose()
        {
            // Arrange
            _manager.ClearActivityId(); // Ensure no activity ID initially
            var activityName = "test-activity";

            // Act & Assert
            using (var scope = new ActivityCorrelationScope(activityName))
            {
                // Inside scope, activity ID should be set
                var currentId = _manager.GetActivityId();
                currentId.Should().NotBeEmpty();
                currentId.Should().Contain(activityName);
            }

            // After scope, activity ID should be cleared
            _manager.GetActivityId().Should().BeEmpty();
        }

        [Fact]
        public void ActivityCorrelationScope_WithContext_ShouldStoreContext()
        {
            // Arrange
            var activityName = "context-activity";
            var context = new Dictionary<string, object>
            {
                { "key1", "value1" },
                { "key2", 42 }
            };

            // Act
            using (var scope = new ActivityCorrelationScope(activityName, context))
            {
                // Assert
                var scopeContext = scope.GetContext();
                scopeContext.Should().ContainKey("key1").WhoseValue.Should().Be("value1");
                scopeContext.Should().ContainKey("key2").WhoseValue.Should().Be(42);
            }
        }

        [Fact]
        public void ActivityCorrelationScope_AddContext_ShouldUpdateContext()
        {
            // Arrange
            var activityName = "dynamic-context-activity";

            // Act
            using (var scope = new ActivityCorrelationScope(activityName))
            {
                scope.AddContext("dynamic_key", "dynamic_value");
                scope.AddContext("another_key", 123);

                // Assert
                var context = scope.GetContext();
                context.Should().ContainKey("dynamic_key").WhoseValue.Should().Be("dynamic_value");
                context.Should().ContainKey("another_key").WhoseValue.Should().Be(123);
            }
        }

        [Fact]
        public void ActivityCorrelationScope_GetContext_ShouldReturnCopy()
        {
            // Arrange
            var activityName = "copy-test-activity";
            var originalContext = new Dictionary<string, object> { { "original", "value" } };

            // Act
            using (var scope = new ActivityCorrelationScope(activityName, originalContext))
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
        public void ActivityCorrelationScope_NestedScopes_ShouldWorkCorrectly()
        {
            // Arrange
            var outerActivityName = "outer-activity";
            var innerActivityName = "inner-activity";

            // Act & Assert
            using (var outerScope = new ActivityCorrelationScope(outerActivityName))
            {
                var outerActivityId = _manager.GetActivityId();
                outerActivityId.Should().Contain(outerActivityName);

                using (var innerScope = new ActivityCorrelationScope(innerActivityName))
                {
                    var innerActivityId = _manager.GetActivityId();
                    innerActivityId.Should().Contain(innerActivityName);
                    innerActivityId.Should().NotBe(outerActivityId);
                }

                // After inner scope, outer activity ID should be restored
                _manager.GetActivityId().Should().Be(outerActivityId);
            }

            // After all scopes, activity ID should be cleared
            _manager.GetActivityId().Should().BeEmpty();
        }

        [Fact]
        public void ActivityCorrelationScope_WithProcessId_ShouldGenerateHierarchicalId()
        {
            // Arrange
            _manager.SetProcessId("test-process-123");
            var activityName = "hierarchical-activity";

            // Act
            using (var scope = new ActivityCorrelationScope(activityName))
            {
                // Assert
                var activityId = scope.GetActivityId();
                activityId.Should().Contain("test-process-123");
                activityId.Should().Contain(activityName);
            }
        }

        [Fact]
        public void ActivityCorrelationScope_MultipleDispose_ShouldBeIdempotent()
        {
            // Arrange
            var activityName = "dispose-test-activity";
            var scope = new ActivityCorrelationScope(activityName);
            
            var activityIdInScope = _manager.GetActivityId();
            activityIdInScope.Should().Contain(activityName);

            // Act
            scope.Dispose();
            var activityIdAfterFirstDispose = _manager.GetActivityId();
            
            scope.Dispose(); // Second dispose should not affect anything
            var activityIdAfterSecondDispose = _manager.GetActivityId();

            // Assert
            activityIdAfterFirstDispose.Should().BeEmpty();
            activityIdAfterSecondDispose.Should().Be(activityIdAfterFirstDispose);
        }
    }
}