using System;
using System.Threading;

namespace PxPoint.Correlation
{
    /// <summary>
    /// C# version of the PxPoint correlation system for multi-process pipeline correlation
    /// Supports hierarchical correlation IDs: Pipeline -> Process -> Activity
    /// </summary>
    public class PxPointCorrelationManager
    {
        private static readonly Lazy<PxPointCorrelationManager> _instance = 
            new Lazy<PxPointCorrelationManager>(() => new PxPointCorrelationManager());
        
        public static PxPointCorrelationManager Instance => _instance.Value;
        
        // Thread-local storage for activity-level correlation
        private static readonly ThreadLocal<string> ThreadActivityId = 
            new ThreadLocal<string>(() => string.Empty);
        
        // Process-level correlation (shared across threads in same process)
        private string _pipelineId = string.Empty;
        private string _processId = string.Empty;
        
        private PxPointCorrelationManager() { }
        
        // Pipeline-level correlation (shared across all processes)
        public void SetPipelineId(string pipelineId)
        {
            _pipelineId = pipelineId ?? string.Empty;
        }
        
        public string GetPipelineId()
        {
            return _pipelineId;
        }
        
        // Process-level correlation (unique per process type)
        public void SetProcessId(string processId)
        {
            _processId = processId ?? string.Empty;
        }
        
        public string GetProcessId()
        {
            return _processId;
        }
        
        // Activity-level correlation (within process activities)
        public void SetActivityId(string activityId)
        {
            ThreadActivityId.Value = activityId ?? string.Empty;
        }
        
        public string GetActivityId()
        {
            return ThreadActivityId.Value ?? string.Empty;
        }
        
        public void ClearActivityId()
        {
            ThreadActivityId.Value = string.Empty;
        }
        
        // Generate new correlation IDs
        public string GeneratePipelineId()
        {
            var timestamp = DateTimeOffset.UtcNow.ToUnixTimeSeconds();
            var uuid = Guid.NewGuid().ToString("N")[..8];
            return $"pxp-{timestamp}-{uuid}";
        }
        
        public string GenerateProcessId(string processType)
        {
            if (string.IsNullOrEmpty(_pipelineId))
            {
                _pipelineId = GeneratePipelineId();
            }
            
            var uuid = Guid.NewGuid().ToString("N")[..8];
            return $"{_pipelineId}-{processType}-{uuid}";
        }
        
        public string GenerateActivityId(string activityName)
        {
            var baseId = string.IsNullOrEmpty(_processId) ? string.Empty : _processId;
            var uuid = Guid.NewGuid().ToString("N")[..8];
            
            return string.IsNullOrEmpty(baseId) 
                ? $"{activityName}-{uuid}"
                : $"{baseId}-{activityName}-{uuid}";
        }
        
        // Environment variable integration for cross-process correlation
        public void LoadFromEnvironment()
        {
            var pipelineEnv = Environment.GetEnvironmentVariable("PXPOINT_PIPELINE_ID");
            if (!string.IsNullOrEmpty(pipelineEnv))
            {
                _pipelineId = pipelineEnv;
            }
            
            var processEnv = Environment.GetEnvironmentVariable("PXPOINT_PROCESS_ID");
            if (!string.IsNullOrEmpty(processEnv))
            {
                _processId = processEnv;
            }
        }
        
        public void SaveToEnvironment()
        {
            if (!string.IsNullOrEmpty(_pipelineId))
            {
                Environment.SetEnvironmentVariable("PXPOINT_PIPELINE_ID", _pipelineId);
            }
            
            if (!string.IsNullOrEmpty(_processId))
            {
                Environment.SetEnvironmentVariable("PXPOINT_PROCESS_ID", _processId);
            }
        }
        
        // Get full correlation context for logging
        public string GetFullCorrelationId()
        {
            var parts = new List<string>();
            
            if (!string.IsNullOrEmpty(_pipelineId))
            {
                parts.Add($"pipeline:{_pipelineId}");
            }
            
            if (!string.IsNullOrEmpty(_processId))
            {
                parts.Add($"process:{_processId}");
            }
            
            var activityId = GetActivityId();
            if (!string.IsNullOrEmpty(activityId))
            {
                parts.Add($"activity:{activityId}");
            }
            
            return string.Join("|", parts);
        }
    }
    
    /// <summary>
    /// RAII scope for activity-level correlation
    /// </summary>
    public class ActivityCorrelationScope : IDisposable
    {
        private readonly string _previousId;
        private bool _disposed = false;
        
        public ActivityCorrelationScope(string activityName)
        {
            var manager = PxPointCorrelationManager.Instance;
            _previousId = manager.GetActivityId();
            var newActivityId = manager.GenerateActivityId(activityName);
            manager.SetActivityId(newActivityId);
        }
        
        public void Dispose()
        {
            if (!_disposed)
            {
                var manager = PxPointCorrelationManager.Instance;
                if (string.IsNullOrEmpty(_previousId))
                {
                    manager.ClearActivityId();
                }
                else
                {
                    manager.SetActivityId(_previousId);
                }
                _disposed = true;
            }
        }
    }
    
    /// <summary>
    /// RAII scope for process initialization
    /// </summary>
    public class ProcessCorrelationScope : IDisposable
    {
        private readonly bool _createdNewPipeline;
        private bool _disposed = false;
        
        public ProcessCorrelationScope(string processType)
        {
            var manager = PxPointCorrelationManager.Instance;
            
            // Try to load existing correlation from environment
            manager.LoadFromEnvironment();
            
            // If no pipeline ID exists, create a new one
            if (string.IsNullOrEmpty(manager.GetPipelineId()))
            {
                manager.SetPipelineId(manager.GeneratePipelineId());
                _createdNewPipeline = true;
            }
            else
            {
                _createdNewPipeline = false;
            }
            
            // Generate process ID for this process type
            var processId = manager.GenerateProcessId(processType);
            manager.SetProcessId(processId);
            
            // Save to environment for child processes
            manager.SaveToEnvironment();
        }
        
        public void Dispose()
        {
            if (!_disposed)
            {
                // Clean up if we created the pipeline
                if (_createdNewPipeline)
                {
                    Environment.SetEnvironmentVariable("PXPOINT_PIPELINE_ID", null);
                    Environment.SetEnvironmentVariable("PXPOINT_PROCESS_ID", null);
                }
                _disposed = true;
            }
        }
    }
}