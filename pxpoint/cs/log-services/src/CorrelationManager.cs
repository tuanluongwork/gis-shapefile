using System;
using System.Collections.Generic;
using System.IO;
using System.Text.Json;
using System.Threading;

namespace LogServices.Correlation
{
    /// <summary>
    /// Configuration for correlation behavior
    /// </summary>
    public class CorrelationConfig
    {
        public string PipelineIdPrefix { get; set; } = "pipeline";
        public string ProcessIdPrefix { get; set; } = "proc";
        public string ActivityIdPrefix { get; set; } = "act";
        public string EnvVarPipeline { get; set; } = "LOG_PIPELINE_ID";
        public string EnvVarProcess { get; set; } = "LOG_PROCESS_ID";
        public bool AutoGeneratePipeline { get; set; } = true;
        public bool AutoGenerateProcess { get; set; } = true;
        public bool PropagateToEnvironment { get; set; } = true;
        
        public Func<string> PipelineIdGenerator { get; set; }
        public Func<string, string> ProcessIdGenerator { get; set; }
        public Func<string, string> ActivityIdGenerator { get; set; }
    }

    /// <summary>
    /// Enhanced correlation manager for multi-process pipeline correlation
    /// Supports hierarchical correlation IDs: Pipeline -> Process -> Activity
    /// Thread-safe with AsyncLocal support for async/await scenarios
    /// </summary>
    public class CorrelationManager
    {
        private static readonly Lazy<CorrelationManager> _instance = 
            new Lazy<CorrelationManager>(() => new CorrelationManager());
        
        public static CorrelationManager Instance => _instance.Value;
        
        // AsyncLocal provides better async/await support than ThreadLocal
        private static readonly AsyncLocal<string> ThreadActivityId = new AsyncLocal<string>();
        
        // Process-level correlation (shared across threads in same process)
        private string _pipelineId = string.Empty;
        private string _processId = string.Empty;
        private CorrelationConfig _config = new CorrelationConfig();
        private readonly object _lock = new object();
        
        private CorrelationManager() { }
        
        // Configuration
        public void Configure(CorrelationConfig config)
        {
            lock (_lock)
            {
                _config = config ?? throw new ArgumentNullException(nameof(config));
            }
        }
        
        public void LoadConfigFromJson(string jsonPath)
        {
            try
            {
                var json = File.ReadAllText(jsonPath);
                var config = JsonSerializer.Deserialize<CorrelationConfig>(json, new JsonSerializerOptions
                {
                    PropertyNamingPolicy = JsonNamingPolicy.CamelCase,
                    PropertyNameCaseInsensitive = true
                });
                Configure(config);
            }
            catch (Exception ex)
            {
                throw new InvalidOperationException($"Failed to load correlation config from {jsonPath}: {ex.Message}", ex);
            }
        }
        
        public CorrelationConfig GetConfig()
        {
            lock (_lock)
            {
                return _config;
            }
        }
        
        // Pipeline-level correlation (shared across all processes)
        public void SetPipelineId(string pipelineId)
        {
            lock (_lock)
            {
                _pipelineId = pipelineId ?? string.Empty;
            }
        }
        
        public string GetPipelineId()
        {
            lock (_lock)
            {
                return _pipelineId;
            }
        }
        
        // Process-level correlation (unique per process type)
        public void SetProcessId(string processId)
        {
            lock (_lock)
            {
                _processId = processId ?? string.Empty;
            }
        }
        
        public string GetProcessId()
        {
            lock (_lock)
            {
                return _processId;
            }
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
            var config = GetConfig();
            if (config.PipelineIdGenerator != null)
            {
                return config.PipelineIdGenerator();
            }
            
            var timestamp = DateTimeOffset.UtcNow.ToUnixTimeSeconds();
            var uuid = Guid.NewGuid().ToString("N")[..8];
            return $"{config.PipelineIdPrefix}-{timestamp}-{uuid}";
        }
        
        public string GenerateProcessId(string processType)
        {
            var config = GetConfig();
            if (config.ProcessIdGenerator != null)
            {
                return config.ProcessIdGenerator(processType);
            }
            
            lock (_lock)
            {
                if (string.IsNullOrEmpty(_pipelineId) && config.AutoGeneratePipeline)
                {
                    _pipelineId = GeneratePipelineId();
                }
                
                var uuid = Guid.NewGuid().ToString("N")[..8];
                return string.IsNullOrEmpty(_pipelineId)
                    ? $"{config.ProcessIdPrefix}-{processType}-{uuid}"
                    : $"{_pipelineId}-{processType}-{uuid}";
            }
        }
        
        public string GenerateActivityId(string activityName)
        {
            var config = GetConfig();
            if (config.ActivityIdGenerator != null)
            {
                return config.ActivityIdGenerator(activityName);
            }
            
            var baseId = GetProcessId();
            var uuid = Guid.NewGuid().ToString("N")[..8];
            
            return string.IsNullOrEmpty(baseId) 
                ? $"{config.ActivityIdPrefix}-{activityName}-{uuid}"
                : $"{baseId}-{activityName}-{uuid}";
        }
        
        // Environment variable integration for cross-process correlation
        public void LoadFromEnvironment()
        {
            var config = GetConfig();
            
            var pipelineEnv = Environment.GetEnvironmentVariable(config.EnvVarPipeline);
            if (!string.IsNullOrEmpty(pipelineEnv))
            {
                SetPipelineId(pipelineEnv);
            }
            
            var processEnv = Environment.GetEnvironmentVariable(config.EnvVarProcess);
            if (!string.IsNullOrEmpty(processEnv))
            {
                SetProcessId(processEnv);
            }
        }
        
        public void SaveToEnvironment()
        {
            var config = GetConfig();
            if (!config.PropagateToEnvironment) return;
            
            var pipelineId = GetPipelineId();
            var processId = GetProcessId();
            
            if (!string.IsNullOrEmpty(pipelineId))
            {
                Environment.SetEnvironmentVariable(config.EnvVarPipeline, pipelineId);
            }
            
            if (!string.IsNullOrEmpty(processId))
            {
                Environment.SetEnvironmentVariable(config.EnvVarProcess, processId);
            }
        }
        
        // Get correlation context for logging
        public string GetFullCorrelationId()
        {
            var parts = new List<string>();
            
            var pipelineId = GetPipelineId();
            if (!string.IsNullOrEmpty(pipelineId))
            {
                parts.Add($"pipeline:{pipelineId}");
            }
            
            var processId = GetProcessId();
            if (!string.IsNullOrEmpty(processId))
            {
                parts.Add($"process:{processId}");
            }
            
            var activityId = GetActivityId();
            if (!string.IsNullOrEmpty(activityId))
            {
                parts.Add($"activity:{activityId}");
            }
            
            return string.Join("|", parts);
        }
        
        public Dictionary<string, object> GetCorrelationContext()
        {
            var context = new Dictionary<string, object>();
            
            var pipelineId = GetPipelineId();
            if (!string.IsNullOrEmpty(pipelineId))
            {
                context["pipeline_id"] = pipelineId;
            }
            
            var processId = GetProcessId();
            if (!string.IsNullOrEmpty(processId))
            {
                context["process_id"] = processId;
            }
            
            var activityId = GetActivityId();
            if (!string.IsNullOrEmpty(activityId))
            {
                context["activity_id"] = activityId;
            }
            
            return context;
        }
        
        // Reset correlation state
        public void Reset()
        {
            lock (_lock)
            {
                _pipelineId = string.Empty;
                _processId = string.Empty;
            }
            ClearActivityId();
        }
    }
    
    /// <summary>
    /// RAII scope for activity-level correlation with context support
    /// </summary>
    public class ActivityCorrelationScope : IDisposable
    {
        private readonly string _previousId;
        private readonly string _activityId;
        private readonly Dictionary<string, object> _context;
        private bool _disposed = false;
        
        public ActivityCorrelationScope(string activityName) : this(activityName, new Dictionary<string, object>())
        {
        }
        
        public ActivityCorrelationScope(string activityName, Dictionary<string, object> context)
        {
            var manager = CorrelationManager.Instance;
            _previousId = manager.GetActivityId();
            _activityId = manager.GenerateActivityId(activityName);
            _context = context ?? new Dictionary<string, object>();
            manager.SetActivityId(_activityId);
        }
        
        public string GetActivityId() => _activityId;
        
        public void AddContext(string key, object value)
        {
            _context[key] = value;
        }
        
        public Dictionary<string, object> GetContext() => new Dictionary<string, object>(_context);
        
        public void Dispose()
        {
            if (!_disposed)
            {
                var manager = CorrelationManager.Instance;
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
    /// RAII scope for process initialization with enhanced configuration support
    /// </summary>
    public class ProcessCorrelationScope : IDisposable
    {
        private readonly bool _createdNewPipeline;
        private readonly string _processId;
        private readonly Dictionary<string, object> _context;
        private bool _disposed = false;
        
        public ProcessCorrelationScope(string processType) : this(processType, new Dictionary<string, object>())
        {
        }
        
        public ProcessCorrelationScope(string processType, Dictionary<string, object> context)
        {
            var manager = CorrelationManager.Instance;
            var config = manager.GetConfig();
            _context = context ?? new Dictionary<string, object>();
            
            // Try to load existing correlation from environment
            manager.LoadFromEnvironment();
            
            // If no pipeline ID exists, create a new one
            if (string.IsNullOrEmpty(manager.GetPipelineId()) && config.AutoGeneratePipeline)
            {
                manager.SetPipelineId(manager.GeneratePipelineId());
                _createdNewPipeline = true;
            }
            else
            {
                _createdNewPipeline = false;
            }
            
            // Generate process ID for this process type
            _processId = manager.GenerateProcessId(processType);
            manager.SetProcessId(_processId);
            
            // Save to environment for child processes
            manager.SaveToEnvironment();
        }
        
        public string GetProcessId() => _processId;
        
        public Dictionary<string, object> GetContext() => new Dictionary<string, object>(_context);
        
        public void Dispose()
        {
            if (!_disposed)
            {
                // Clean up if we created the pipeline
                if (_createdNewPipeline)
                {
                    var manager = CorrelationManager.Instance;
                    var config = manager.GetConfig();
                    Environment.SetEnvironmentVariable(config.EnvVarPipeline, null);
                    Environment.SetEnvironmentVariable(config.EnvVarProcess, null);
                }
                _disposed = true;
            }
        }
    }
}