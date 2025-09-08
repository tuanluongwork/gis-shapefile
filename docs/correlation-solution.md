# Multi-Process Correlation ID Architecture for PxPoint

## Core Challenge Analysis

The PxPoint pipeline presents unique correlation challenges:
- **Process Spawning**: C# applications spawn C++ executables via `Process.Start()`
- **Mixed Languages**: C# ↔ C++ ↔ SQL operations need correlation
- **FIPS-based Parallelism**: Multiple counties processing simultaneously
- **Long-running Operations**: Hours/days of processing with potential crashes
- **Legacy Integration**: Cannot break existing CLI interfaces

## Correlation ID Strategy

### Hierarchical ID Structure
```
Build-Level:    2024Q1-001
├─ Job-Level:   2024Q1-001.J12345 (FIPS-based)
   ├─ Step-Level: 2024Q1-001.J12345.S003 (Normalize)
      ├─ Process-Level: 2024Q1-001.J12345.S003.P001 (ParcelNormalizer.exe)
         └─ Operation-Level: 2024Q1-001.J12345.S003.P001.O123 (Individual SQL calls)
```

### ID Generation Pattern
```csharp
public static class CorrelationIdGenerator 
{
    private static readonly string _buildId = Environment.GetEnvironmentVariable("BUILD_ID") 
        ?? $"{DateTime.UtcNow:yyyyMM}-{Environment.MachineName.Substring(0,3)}";
    
    public static string GenerateBuildId() => _buildId;
    
    public static string GenerateJobId(string fips, int jobId) 
        => $"{_buildId}.J{jobId:D5}";
        
    public static string GenerateStepId(string jobCorrelationId, string stepName) 
        => $"{jobCorrelationId}.S{GetStepNumber(stepName):D3}";
        
    public static string GenerateProcessId(string stepCorrelationId) 
        => $"{stepCorrelationId}.P{Interlocked.Increment(ref _processCounter):D3}";
        
    public static string GenerateOperationId(string processCorrelationId) 
        => $"{processCorrelationId}.O{Interlocked.Increment(ref _opCounter):D3}";
}
```

## Implementation Strategies

### 1. Environment Variable Propagation

**C# Process Spawning**:
```csharp
public class ProcessExecutor 
{
    public async Task<ProcessResult> ExecuteAsync(string executable, string arguments, 
        string correlationId, Dictionary<string, object> context)
    {
        var processCorrelationId = CorrelationIdGenerator.GenerateProcessId(correlationId);
        
        var processInfo = new ProcessStartInfo(executable, arguments)
        {
            UseShellExecute = false,
            RedirectStandardOutput = true,
            RedirectStandardError = true
        };
        
        // Propagate correlation context via environment variables
        processInfo.Environment["CORRELATION_ID"] = processCorrelationId;
        processInfo.Environment["PARENT_CORRELATION_ID"] = correlationId;
        processInfo.Environment["FIPS"] = context["FIPS"]?.ToString();
        processInfo.Environment["JOB_ID"] = context["JobId"]?.ToString();
        processInfo.Environment["BUILD_ID"] = CorrelationIdGenerator.GenerateBuildId();
        
        using var process = new Process { StartInfo = processInfo };
        using var logger = PxPointLogger.BeginScope(processCorrelationId, context);
        
        _logger.LogInformation("Starting process {Executable} with correlation {CorrelationId}", 
            executable, processCorrelationId);
            
        process.Start();
        
        // Capture and correlate output streams
        var outputTask = CaptureOutputStream(process.StandardOutput, processCorrelationId, "stdout");
        var errorTask = CaptureOutputStream(process.StandardError, processCorrelationId, "stderr");
        
        await process.WaitForExitAsync();
        await Task.WhenAll(outputTask, errorTask);
        
        _logger.LogInformation("Process {Executable} completed with exit code {ExitCode}", 
            executable, process.ExitCode);
            
        return new ProcessResult 
        { 
            ExitCode = process.ExitCode, 
            CorrelationId = processCorrelationId 
        };
    }
    
    private async Task CaptureOutputStream(StreamReader stream, string correlationId, string streamType)
    {
        while (!stream.EndOfStream)
        {
            var line = await stream.ReadLineAsync();
            if (!string.IsNullOrEmpty(line))
            {
                // Parse and enrich subprocess output with correlation context
                _logger.LogDebug("[{StreamType}] {CorrelationId}: {Output}", 
                    streamType, correlationId, line);
            }
        }
    }
}
```

**C++ Environment Reading**:
```cpp
class CorrelationContext 
{
private:
    std::string correlation_id_;
    std::string parent_id_;
    std::string fips_;
    std::string job_id_;
    std::string build_id_;
    
public:
    CorrelationContext() {
        correlation_id_ = GetEnvVar("CORRELATION_ID");
        parent_id_ = GetEnvVar("PARENT_CORRELATION_ID");
        fips_ = GetEnvVar("FIPS");
        job_id_ = GetEnvVar("JOB_ID");
        build_id_ = GetEnvVar("BUILD_ID");
        
        if (correlation_id_.empty()) {
            correlation_id_ = GenerateCorrelationId();
        }
    }
    
    std::string GetCorrelationId() const { return correlation_id_; }
    std::string GetFIPS() const { return fips_; }
    
    // Create child correlation for sub-operations
    std::string CreateChildId() {
        static std::atomic<int> counter{0};
        return correlation_id_ + ".O" + std::to_string(++counter);
    }
    
private:
    std::string GetEnvVar(const char* name) {
        const char* value = std::getenv(name);
        return value ? std::string(value) : std::string{};
    }
    
    std::string GenerateCorrelationId() {
        // Fallback if not provided by parent process
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        return "ORPHAN-" + std::to_string(time_t);
    }
};

// Global context accessible throughout C++ application
thread_local CorrelationContext g_correlation_context;

// Logging with automatic correlation injection
#define LOG_INFO(msg, ...) \
    PxPointLogger::GetLogger()->info( \
        fmt::format("{{\"correlation_id\": \"{}\", \"fips\": \"{}\", \"message\": \"" msg "\"}}", \
                   g_correlation_context.GetCorrelationId(), \
                   g_correlation_context.GetFIPS(), \
                   ##__VA_ARGS__))
```

### 2. Command Line Argument Propagation

For applications that cannot be modified to read environment variables:

```csharp
public class LegacyProcessWrapper 
{
    public string BuildCommandLine(string baseArgs, string correlationId, Dictionary<string, object> context)
    {
        var correlationArgs = new StringBuilder(baseArgs);
        
        // Inject correlation parameters using existing patterns
        correlationArgs.Append($" --correlation-id \"{correlationId}\"");
        correlationArgs.Append($" --fips \"{context["FIPS"]}\"");
        correlationArgs.Append($" --job-id \"{context["JobId"]}\"");
        
        return correlationArgs.ToString();
    }
}
```

### 3. File-Based Context Sharing

For processes that need to share state:

```csharp
public class ContextFileManager 
{
    private readonly string _contextDirectory;
    
    public ContextFileManager(string workingDirectory)
    {
        _contextDirectory = Path.Combine(workingDirectory, ".correlation");
        Directory.CreateDirectory(_contextDirectory);
    }
    
    public async Task WriteContextAsync(string correlationId, Dictionary<string, object> context)
    {
        var contextFile = Path.Combine(_contextDirectory, $"{correlationId}.json");
        var contextJson = JsonSerializer.Serialize(context, new JsonSerializerOptions { WriteIndented = true });
        
        await File.WriteAllTextAsync(contextFile, contextJson);
        
        // Set short TTL to prevent accumulation
        _ = Task.Delay(TimeSpan.FromHours(24))
            .ContinueWith(_ => File.Delete(contextFile), TaskContinuationOptions.OnlyOnRanToCompletion);
    }
    
    public async Task<Dictionary<string, object>> ReadContextAsync(string correlationId)
    {
        var contextFile = Path.Combine(_contextDirectory, $"{correlationId}.json");
        
        if (!File.Exists(contextFile))
            return new Dictionary<string, object>();
            
        var contextJson = await File.ReadAllTextAsync(contextFile);
        return JsonSerializer.Deserialize<Dictionary<string, object>>(contextJson) ?? new();
    }
}
```

## Database Correlation Strategy

### SQL Connection Correlation
```csharp
public class CorrelatedDbConnection : IDisposable 
{
    private readonly SqlConnection _connection;
    private readonly string _correlationId;
    
    public CorrelatedDbConnection(string connectionString, string correlationId)
    {
        _correlationId = correlationId;
        _connection = new SqlConnection(connectionString);
        
        // Set application name to include correlation ID for SQL Server monitoring
        var builder = new SqlConnectionStringBuilder(connectionString)
        {
            ApplicationName = $"PxPoint-{correlationId}"
        };
        _connection.ConnectionString = builder.ConnectionString;
    }
    
    public async Task<SqlCommand> CreateCorrelatedCommandAsync(string sql)
    {
        var command = _connection.CreateCommand();
        command.CommandText = sql;
        
        // Add correlation context as SQL comment for query log analysis
        command.CommandText = $"-- Correlation: {_correlationId}\n{sql}";
        
        return command;
    }
}
```

### SQL Query Context Injection
```csharp
public class SqlQueryInterceptor : DbCommandInterceptor
{
    public override ValueTask<InterceptionResult<DbDataReader>> ReaderExecutingAsync(
        DbCommand command, CommandEventData eventData, InterceptionResult<DbDataReader> result,
        CancellationToken cancellationToken = default)
    {
        // Extract correlation from current scope
        var correlationId = GetCurrentCorrelationId();
        
        // Inject as SQL comment for correlation in query logs
        if (!string.IsNullOrEmpty(correlationId))
        {
            command.CommandText = $"-- Correlation: {correlationId}, FIPS: {GetCurrentFIPS()}\n{command.CommandText}";
        }
        
        _logger.LogDebug("Executing SQL with correlation {CorrelationId}: {Query}", 
            correlationId, TruncateQuery(command.CommandText));
        
        return base.ReaderExecutingAsync(command, eventData, result, cancellationToken);
    }
}
```

## Cross-Process Correlation Patterns

### Parent-Child Relationship Tracking
```csharp
public class ProcessCorrelationManager 
{
    private static readonly ConcurrentDictionary<string, ProcessInfo> _activeProcesses = new();
    
    public class ProcessInfo 
    {
        public string CorrelationId { get; set; }
        public string ParentCorrelationId { get; set; }
        public string FIPS { get; set; }
        public int JobId { get; set; }
        public DateTime StartTime { get; set; }
        public string ExecutableName { get; set; }
    }
    
    public static void RegisterProcess(ProcessInfo info)
    {
        _activeProcesses.TryAdd(info.CorrelationId, info);
        
        // Log process genealogy
        _logger.LogInformation("Process started: {CorrelationId} (Parent: {ParentId}, FIPS: {FIPS})", 
            info.CorrelationId, info.ParentCorrelationId, info.FIPS);
    }
    
    public static void UnregisterProcess(string correlationId)
    {
        if (_activeProcesses.TryRemove(correlationId, out var info))
        {
            var duration = DateTime.UtcNow - info.StartTime;
            _logger.LogInformation("Process completed: {CorrelationId} in {Duration}", 
                correlationId, duration);
        }
    }
    
    public static string[] GetProcessHierarchy(string correlationId)
    {
        var hierarchy = new List<string>();
        var current = correlationId;
        
        while (!string.IsNullOrEmpty(current) && _activeProcesses.TryGetValue(current, out var info))
        {
            hierarchy.Add(current);
            current = info.ParentCorrelationId;
        }
        
        hierarchy.Reverse();
        return hierarchy.ToArray();
    }
}
```

### Error Correlation and Bubbling
```csharp
public class CorrelatedErrorHandler 
{
    public static async Task HandleProcessError(string correlationId, Exception error, ProcessInfo processInfo)
    {
        // Enrich error with full correlation context
        var enrichedError = new Dictionary<string, object>
        {
            ["correlation_id"] = correlationId,
            ["parent_correlation_id"] = processInfo.ParentCorrelationId,
            ["fips"] = processInfo.FIPS,
            ["job_id"] = processInfo.JobId,
            ["executable"] = processInfo.ExecutableName,
            ["error_message"] = error.Message,
            ["stack_trace"] = error.StackTrace,
            ["process_hierarchy"] = ProcessCorrelationManager.GetProcessHierarchy(correlationId),
            ["timestamp"] = DateTime.UtcNow
        };
        
        // Log with full context
        _logger.LogError("Process failed with correlation {CorrelationId}: {Error}", 
            correlationId, JsonSerializer.Serialize(enrichedError));
        
        // Notify parent process of failure
        await NotifyParentProcess(processInfo.ParentCorrelationId, enrichedError);
        
        // Send to alerting system with correlation context
        await SlackNotifier.SendError("Process Failure", error, enrichedError);
    }
}
```

## Monitoring and Observability Integration

### Distributed Tracing Integration
```csharp
public class DistributedTracingContext 
{
    public static Activity StartActivity(string operationName, string correlationId, string fips)
    {
        var activity = Activity.StartActivity(operationName);
        if (activity != null)
        {
            activity.SetTag("correlation.id", correlationId);
            activity.SetTag("pxpoint.fips", fips);
            activity.SetTag("pxpoint.component", "pipeline");
        }
        return activity;
    }
}

// Usage in major operations
using var activity = DistributedTracingContext.StartActivity("normalize-parcels", correlationId, fips);
```

### Elasticsearch Correlation Queries
```json
{
  "query": {
    "bool": {
      "must": [
        { "term": { "correlation_id.keyword": "2024Q1-001.J12345.S003" } }
      ]
    }
  },
  "sort": [
    { "@timestamp": { "order": "asc" } }
  ]
}
```

## Implementation Phases

### Phase 1: Foundation (Weeks 1-2)
- Implement correlation ID generation and propagation patterns
- Update C# process spawning to include environment variables
- Basic C++ environment variable reading

### Phase 2: Integration (Weeks 3-4)
- SQL correlation injection
- Process hierarchy tracking
- Error correlation and bubbling

### Phase 3: Observability (Weeks 5-6)
- Distributed tracing integration
- Dashboard correlation views
- Automated correlation analysis

This architecture ensures that every operation in the complex PxPoint pipeline can be traced back to its origin, dramatically improving debugging capabilities while maintaining the existing process boundaries and CLI interfaces.