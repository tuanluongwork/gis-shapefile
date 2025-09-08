using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;
using LogServices.Correlation;

namespace LogServices.Logging
{
    /// <summary>
    /// Log level enumeration
    /// </summary>
    public enum LogLevel
    {
        Debug,
        Info,
        Warning,
        Error,
        Critical
    }

    /// <summary>
    /// Sink configuration for structured logging
    /// </summary>
    public class SinkConfig
    {
        public enum SinkType
        {
            Console,
            File,
            RotatingFile,
            DailyFile
        }
        
        public SinkType Type { get; set; }
        public string Name { get; set; } = "default";
        public LogLevel Level { get; set; } = LogLevel.Info;
        public string Pattern { get; set; } = "{timestamp:yyyy-MM-ddTHH:mm:ss.fffZ} [{level}] [{name}] {message}";
        
        // File-specific options
        public string FilePath { get; set; } = "";
        public long MaxFileSize { get; set; } = 10 * 1024 * 1024; // 10MB
        public int MaxFiles { get; set; } = 5;
        public int RotationHour { get; set; } = 0;
        public int RotationMinute { get; set; } = 0;
        
        // Console-specific options
        public bool ColorMode { get; set; } = true;
    }

    /// <summary>
    /// Logger configuration structure
    /// </summary>
    public class LoggerConfig
    {
        public string Name { get; set; } = "application";
        public LogLevel Level { get; set; } = LogLevel.Info;
        public string DefaultPattern { get; set; } = "{timestamp:yyyy-MM-ddTHH:mm:ss.fffZ} [{level}] [{name}] {message}";
        public string JsonPattern { get; set; } = "json";
        
        // Async logging configuration
        public bool AsyncLogging { get; set; } = true;
        public int AsyncQueueSize { get; set; } = 8192;
        public int AsyncThreadCount { get; set; } = 1;
        public string AsyncOverflowPolicy { get; set; } = "block";
        
        // Output directory
        public string LogDirectory { get; set; } = "/tmp/pxpoint-logs";
        
        // Automatic correlation integration
        public bool AutoAddCorrelation { get; set; } = true;
        
        // Error handling
        public bool FlushOnError { get; set; } = true;
        public int FlushIntervalSeconds { get; set; } = 5;
        
        public List<SinkConfig> Sinks { get; set; } = new List<SinkConfig>();
    }

    /// <summary>
    /// Log entry structure for async processing
    /// </summary>
    internal class LogEntry
    {
        public LogLevel Level { get; set; }
        public DateTime Timestamp { get; set; }
        public string Component { get; set; } = "";
        public string Message { get; set; } = "";
        public Dictionary<string, object> Context { get; set; } = new Dictionary<string, object>();
        public Dictionary<string, double> Metrics { get; set; } = new Dictionary<string, double>();
        public string CorrelationId { get; set; } = "";
        public string ProcessType { get; set; } = "";
        public int ProcessId { get; set; }
        public int ThreadId { get; set; }
    }

    /// <summary>
    /// Enhanced structured logger with async capabilities and configuration support
    /// Equivalent to C++ log-services StructuredLogger with .NET optimizations
    /// </summary>
    public class StructuredLogger : IDisposable
    {
        private static readonly Lazy<StructuredLogger> _instance = 
            new Lazy<StructuredLogger>(() => new StructuredLogger());
        
        public static StructuredLogger Instance => _instance.Value;
        
        private bool _initialized = false;
        private string _processType = string.Empty;
        private LoggerConfig _config = new LoggerConfig();
        private readonly object _configLock = new object();
        
        // Async logging infrastructure
        private readonly ConcurrentQueue<LogEntry> _logQueue = new ConcurrentQueue<LogEntry>();
        private readonly CancellationTokenSource _cancellationTokenSource = new CancellationTokenSource();
        private Task _backgroundTask;
        private readonly ManualResetEventSlim _flushEvent = new ManualResetEventSlim(true);
        
        // File writers for different sinks
        private readonly Dictionary<string, StreamWriter> _fileWriters = new Dictionary<string, StreamWriter>();
        private readonly object _writersLock = new object();
        
        private bool _disposed = false;
        
        private StructuredLogger() { }
        
        /// <summary>
        /// Configure the logger with detailed configuration
        /// </summary>
        public void Configure(LoggerConfig config)
        {
            lock (_configLock)
            {
                _config = config ?? throw new ArgumentNullException(nameof(config));
            }
        }
        
        /// <summary>
        /// Load configuration from JSON file
        /// </summary>
        public void LoadConfigFromJson(string jsonPath)
        {
            try
            {
                var json = File.ReadAllText(jsonPath);
                var config = JsonSerializer.Deserialize<LoggerConfig>(json, new JsonSerializerOptions
                {
                    PropertyNamingPolicy = JsonNamingPolicy.CamelCase,
                    PropertyNameCaseInsensitive = true
                });
                Configure(config);
            }
            catch (Exception ex)
            {
                throw new InvalidOperationException($"Failed to load logger config from {jsonPath}: {ex.Message}", ex);
            }
        }
        
        /// <summary>
        /// Initialize the logger for a specific process type
        /// </summary>
        public void Initialize(string processType, LogLevel logLevel = LogLevel.Info)
        {
            if (_initialized)
                return;
                
            _processType = processType;
            
            lock (_configLock)
            {
                _config.Level = logLevel;
            }
            
            try
            {
                // Ensure log directory exists
                EnsureDirectoryExists(_config.LogDirectory);
                
                // Initialize sinks
                InitializeSinks();
                
                // Start background logging task if async logging is enabled
                if (_config.AsyncLogging)
                {
                    _backgroundTask = Task.Run(BackgroundLoggingLoop, _cancellationTokenSource.Token);
                }
                
                _initialized = true;
                
                // Log initialization
                LogInfo("Logger", "Structured logger initialized", new Dictionary<string, object>
                {
                    {"process_type", processType},
                    {"log_directory", _config.LogDirectory},
                    {"async_logging", _config.AsyncLogging}
                });
            }
            catch (Exception e)
            {
                throw new InvalidOperationException($"Failed to initialize structured logger: {e.Message}", e);
            }
        }
        
        /// <summary>
        /// Core logging method with full context support
        /// </summary>
        public void Log(LogLevel level, string component, string message,
            Dictionary<string, object> context = null, Dictionary<string, double> metrics = null)
        {
            if (!_initialized)
                throw new InvalidOperationException("Structured logger not initialized");
            
            if (level < _config.Level)
                return;
            
            var entry = new LogEntry
            {
                Level = level,
                Timestamp = DateTime.UtcNow,
                Component = component ?? "",
                Message = message ?? "",
                Context = context ?? new Dictionary<string, object>(),
                Metrics = metrics ?? new Dictionary<string, double>(),
                ProcessType = _processType,
                ProcessId = Environment.ProcessId,
                ThreadId = Thread.CurrentThread.ManagedThreadId
            };
            
            // Add correlation context if enabled
            if (_config.AutoAddCorrelation)
            {
                entry.CorrelationId = CorrelationManager.Instance.GetFullCorrelationId();
                var correlationContext = CorrelationManager.Instance.GetCorrelationContext();
                foreach (var kvp in correlationContext)
                {
                    entry.Context[kvp.Key] = kvp.Value;
                }
            }
            
            // Process synchronously or asynchronously
            if (_config.AsyncLogging)
            {
                _logQueue.Enqueue(entry);
            }
            else
            {
                WriteLogEntry(entry);
            }
            
            // Flush immediately on error if configured
            if (level >= LogLevel.Error && _config.FlushOnError)
            {
                Flush();
            }
        }
        
        // Convenience methods
        public void LogDebug(string component, string message, Dictionary<string, object> context = null)
            => Log(LogLevel.Debug, component, message, context);
            
        public void LogInfo(string component, string message, Dictionary<string, object> context = null)
            => Log(LogLevel.Info, component, message, context);
            
        public void LogWarning(string component, string message, Dictionary<string, object> context = null)
            => Log(LogLevel.Warning, component, message, context);
            
        public void LogError(string component, string message, Dictionary<string, object> context = null)
            => Log(LogLevel.Error, component, message, context);
            
        public void LogCritical(string component, string message, Dictionary<string, object> context = null)
            => Log(LogLevel.Critical, component, message, context);
        
        // Overloaded methods without component parameter
        public void LogDebug(string message, Dictionary<string, object> context = null)
            => Log(LogLevel.Debug, "", message, context);
            
        public void LogInfo(string message, Dictionary<string, object> context = null)
            => Log(LogLevel.Info, "", message, context);
            
        public void LogWarning(string message, Dictionary<string, object> context = null)
            => Log(LogLevel.Warning, "", message, context);
            
        public void LogError(string message, Dictionary<string, object> context = null)
            => Log(LogLevel.Error, "", message, context);
            
        public void LogCritical(string message, Dictionary<string, object> context = null)
            => Log(LogLevel.Critical, "", message, context);
        
        /// <summary>
        /// Log event with specific event type
        /// </summary>
        public void LogEvent(string eventType, string description,
            Dictionary<string, object> context = null, Dictionary<string, double> metrics = null)
        {
            var eventContext = context ?? new Dictionary<string, object>();
            eventContext["event_type"] = eventType;
            eventContext["timestamp"] = DateTime.UtcNow.ToString("yyyy-MM-ddTHH:mm:ss.fffZ");
            
            Log(LogLevel.Info, "Event", description, eventContext, metrics);
        }
        
        /// <summary>
        /// Log process lifecycle events
        /// </summary>
        public void LogProcessStart(string processType, Dictionary<string, object> config = null)
        {
            var context = config ?? new Dictionary<string, object>();
            context["event_type"] = "process_start";
            context["timestamp"] = DateTime.UtcNow.ToString("yyyy-MM-ddTHH:mm:ss.fffZ");
            
            LogInfo("Process", $"Process started: {processType}", context);
        }
        
        public void LogProcessEnd(string processType, bool success = true, 
            Dictionary<string, double> metrics = null)
        {
            var context = new Dictionary<string, object>
            {
                ["event_type"] = "process_end",
                ["success"] = success,
                ["timestamp"] = DateTime.UtcNow.ToString("yyyy-MM-ddTHH:mm:ss.fffZ")
            };
            
            var logLevel = success ? LogLevel.Info : LogLevel.Error;
            var message = success 
                ? $"Process completed successfully: {processType}"
                : $"Process failed: {processType}";
            
            Log(logLevel, "Process", message, context, metrics);
        }
        
        /// <summary>
        /// Log activity lifecycle events
        /// </summary>
        public void LogActivityStart(string activityName, Dictionary<string, object> context = null)
        {
            var activityContext = context ?? new Dictionary<string, object>();
            activityContext["event_type"] = "activity_start";
            activityContext["timestamp"] = DateTime.UtcNow.ToString("yyyy-MM-ddTHH:mm:ss.fffZ");
            
            LogDebug("Activity", $"Activity started: {activityName}", activityContext);
        }
        
        public void LogActivityEnd(string activityName, bool success = true,
            Dictionary<string, double> metrics = null)
        {
            var context = new Dictionary<string, object>
            {
                ["event_type"] = "activity_end",
                ["success"] = success,
                ["timestamp"] = DateTime.UtcNow.ToString("yyyy-MM-ddTHH:mm:ss.fffZ")
            };
            
            var logLevel = success ? LogLevel.Debug : LogLevel.Warning;
            var message = success 
                ? $"Activity completed: {activityName}"
                : $"Activity failed: {activityName}";
            
            Log(logLevel, "Activity", message, context, metrics);
        }
        
        /// <summary>
        /// Log performance metrics
        /// </summary>
        public void LogPerformance(string operation, double durationMs,
            Dictionary<string, object> context = null, Dictionary<string, double> metrics = null)
        {
            var perfContext = context ?? new Dictionary<string, object>();
            perfContext["operation"] = operation;
            perfContext["event_type"] = "performance";
            
            var perfMetrics = metrics ?? new Dictionary<string, double>();
            perfMetrics["duration_ms"] = durationMs;
            
            Log(LogLevel.Info, "Performance", $"Operation completed: {operation}", perfContext, perfMetrics);
        }
        
        /// <summary>
        /// Log error with exception support
        /// </summary>
        public void LogError(string component, string message, Exception exception = null,
            Dictionary<string, object> context = null)
        {
            var errorContext = context ?? new Dictionary<string, object>();
            errorContext["event_type"] = "error";
            errorContext["timestamp"] = DateTime.UtcNow.ToString("yyyy-MM-ddTHH:mm:ss.fffZ");
            
            if (exception != null)
            {
                errorContext["exception_type"] = exception.GetType().Name;
                errorContext["exception_message"] = exception.Message;
                errorContext["stack_trace"] = exception.StackTrace;
            }
            
            var errorMessage = $"ERROR: {message}";
            if (exception != null)
            {
                errorMessage += $" | Exception: {exception.Message}";
            }
            
            Log(LogLevel.Error, component, errorMessage, errorContext);
        }
        
        /// <summary>
        /// Flush all pending log entries
        /// </summary>
        public void Flush()
        {
            if (_config.AsyncLogging && _backgroundTask != null)
            {
                _flushEvent.Reset();
                // Process all queued entries
                while (_logQueue.TryDequeue(out var entry))
                {
                    WriteLogEntry(entry);
                }
                _flushEvent.Set();
            }
            
            // Flush all file writers
            lock (_writersLock)
            {
                foreach (var writer in _fileWriters.Values)
                {
                    writer?.Flush();
                }
            }
        }
        
        /// <summary>
        /// Set logging level
        /// </summary>
        public void SetLevel(LogLevel level)
        {
            lock (_configLock)
            {
                _config.Level = level;
            }
        }
        
        public LogLevel GetLevel()
        {
            lock (_configLock)
            {
                return _config.Level;
            }
        }
        
        /// <summary>
        /// Background task for async logging
        /// </summary>
        private async Task BackgroundLoggingLoop()
        {
            while (!_cancellationTokenSource.Token.IsCancellationRequested)
            {
                try
                {
                    var processedAny = false;
                    
                    // Process up to 100 entries per batch
                    for (int i = 0; i < 100 && _logQueue.TryDequeue(out var entry); i++)
                    {
                        WriteLogEntry(entry);
                        processedAny = true;
                    }
                    
                    if (!processedAny)
                    {
                        // Wait a bit if no entries were processed
                        await Task.Delay(10, _cancellationTokenSource.Token);
                    }
                }
                catch (OperationCanceledException)
                {
                    break;
                }
                catch (Exception ex)
                {
                    // Log to console as fallback
                    Console.WriteLine($"[ERROR] Background logging failed: {ex.Message}");
                }
            }
            
            // Process remaining entries on shutdown
            while (_logQueue.TryDequeue(out var entry))
            {
                WriteLogEntry(entry);
            }
        }
        
        /// <summary>
        /// Write log entry to configured sinks
        /// </summary>
        private void WriteLogEntry(LogEntry entry)
        {
            try
            {
                var jsonEntry = FormatAsJson(entry);
                var textEntry = FormatAsText(entry);
                
                foreach (var sink in _config.Sinks)
                {
                    if (entry.Level < sink.Level)
                        continue;
                    
                    switch (sink.Type)
                    {
                        case SinkConfig.SinkType.Console:
                            WriteToConsole(textEntry, entry.Level, sink.ColorMode);
                            break;
                        case SinkConfig.SinkType.File:
                        case SinkConfig.SinkType.RotatingFile:
                        case SinkConfig.SinkType.DailyFile:
                            WriteToFile(sink, jsonEntry);
                            break;
                    }
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[ERROR] Failed to write log entry: {ex.Message}");
            }
        }
        
        /// <summary>
        /// Format log entry as JSON
        /// </summary>
        private string FormatAsJson(LogEntry entry)
        {
            var logObject = new Dictionary<string, object>
            {
                ["timestamp"] = entry.Timestamp.ToString("yyyy-MM-ddTHH:mm:ss.fffZ"),
                ["level"] = entry.Level.ToString().ToUpper(),
                ["process_type"] = entry.ProcessType,
                ["process_id"] = entry.ProcessId,
                ["thread_id"] = entry.ThreadId,
                ["component"] = entry.Component,
                ["message"] = entry.Message
            };
            
            if (!string.IsNullOrEmpty(entry.CorrelationId))
            {
                logObject["correlation"] = entry.CorrelationId;
            }
            
            if (entry.Context.Count > 0)
            {
                logObject["context"] = entry.Context;
            }
            
            if (entry.Metrics.Count > 0)
            {
                logObject["metrics"] = entry.Metrics;
            }
            
            return JsonSerializer.Serialize(logObject, new JsonSerializerOptions
            {
                WriteIndented = false
            });
        }
        
        /// <summary>
        /// Format log entry as human-readable text
        /// </summary>
        private string FormatAsText(LogEntry entry)
        {
            var message = $"[{entry.Timestamp:yyyy-MM-ddTHH:mm:ss.fff}] [{entry.Level}] [{entry.Component}] {entry.Message}";
            
            if (!string.IsNullOrEmpty(entry.CorrelationId))
            {
                message += $" | correlation:{entry.CorrelationId}";
            }
            
            return message;
        }
        
        /// <summary>
        /// Write to console with optional color coding
        /// </summary>
        private void WriteToConsole(string message, LogLevel level, bool useColor)
        {
            if (useColor)
            {
                var originalColor = Console.ForegroundColor;
                Console.ForegroundColor = level switch
                {
                    LogLevel.Debug => ConsoleColor.Gray,
                    LogLevel.Info => ConsoleColor.White,
                    LogLevel.Warning => ConsoleColor.Yellow,
                    LogLevel.Error => ConsoleColor.Red,
                    LogLevel.Critical => ConsoleColor.Magenta,
                    _ => ConsoleColor.White
                };
                
                Console.WriteLine(message);
                Console.ForegroundColor = originalColor;
            }
            else
            {
                Console.WriteLine(message);
            }
        }
        
        /// <summary>
        /// Write to file sink
        /// </summary>
        private void WriteToFile(SinkConfig sink, string message)
        {
            lock (_writersLock)
            {
                if (!_fileWriters.TryGetValue(sink.Name, out var writer) || writer == null)
                {
                    var filePath = ResolvePath(sink.FilePath);
                    EnsureDirectoryExists(Path.GetDirectoryName(filePath));
                    
                    writer = new StreamWriter(filePath, append: true)
                    {
                        AutoFlush = true
                    };
                    
                    _fileWriters[sink.Name] = writer;
                }
                
                writer.WriteLine(message);
            }
        }
        
        /// <summary>
        /// Initialize all configured sinks
        /// </summary>
        private void InitializeSinks()
        {
            if (_config.Sinks.Count == 0)
            {
                // Add default console and file sinks
                _config.Sinks.Add(new SinkConfig
                {
                    Type = SinkConfig.SinkType.Console,
                    Name = "console",
                    Level = LogLevel.Info,
                    ColorMode = true
                });
                
                _config.Sinks.Add(new SinkConfig
                {
                    Type = SinkConfig.SinkType.DailyFile,
                    Name = "daily_file",
                    Level = LogLevel.Debug,
                    FilePath = Path.Combine(_config.LogDirectory, $"{_processType}-{DateTime.UtcNow:yyyy-MM-dd}.log")
                });
            }
        }
        
        /// <summary>
        /// Resolve file path with variables
        /// </summary>
        private string ResolvePath(string path)
        {
            return path
                .Replace("{process_type}", _processType)
                .Replace("{date}", DateTime.UtcNow.ToString("yyyy-MM-dd"))
                .Replace("{timestamp}", DateTimeOffset.UtcNow.ToUnixTimeSeconds().ToString());
        }
        
        /// <summary>
        /// Ensure directory exists
        /// </summary>
        private void EnsureDirectoryExists(string path)
        {
            if (!Directory.Exists(path))
            {
                Directory.CreateDirectory(path);
            }
        }
        
        /// <summary>
        /// Shutdown logger and cleanup resources
        /// </summary>
        public void Shutdown()
        {
            Dispose();
        }
        
        public void Dispose()
        {
            if (_disposed) return;
            
            try
            {
                // Cancel background task
                _cancellationTokenSource.Cancel();
                
                // Wait for background task to complete
                if (_backgroundTask != null)
                {
                    try
                    {
                        _backgroundTask.Wait(TimeSpan.FromSeconds(5));
                    }
                    catch (AggregateException)
                    {
                        // Ignore cancellation exceptions
                    }
                }
                
                // Flush and close all file writers
                lock (_writersLock)
                {
                    foreach (var writer in _fileWriters.Values)
                    {
                        try
                        {
                            writer?.Flush();
                            writer?.Close();
                            writer?.Dispose();
                        }
                        catch (Exception ex)
                        {
                            Console.WriteLine($"[ERROR] Failed to dispose writer: {ex.Message}");
                        }
                    }
                    _fileWriters.Clear();
                }
                
                _cancellationTokenSource.Dispose();
                _flushEvent.Dispose();
                
                _disposed = true;
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[ERROR] Failed to dispose logger: {ex.Message}");
            }
        }
    }
    
    /// <summary>
    /// Performance measurement helper equivalent to C++ PerformanceTimer
    /// </summary>
    public class PerformanceTimer : IDisposable
    {
        private readonly string _operationName;
        private readonly Dictionary<string, object> _context;
        private readonly Dictionary<string, double> _metrics;
        private readonly DateTime _startTime;
        private bool _stopped = false;
        
        public PerformanceTimer(string operationName, Dictionary<string, object> context = null)
        {
            _operationName = operationName;
            _context = context ?? new Dictionary<string, object>();
            _metrics = new Dictionary<string, double>();
            _startTime = DateTime.UtcNow;
        }
        
        public void AddContext(string key, object value)
        {
            _context[key] = value;
        }
        
        public void AddMetric(string key, double value)
        {
            _metrics[key] = value;
        }
        
        public void Stop()
        {
            if (_stopped) return;
            
            var duration = (DateTime.UtcNow - _startTime).TotalMilliseconds;
            StructuredLogger.Instance.LogPerformance(_operationName, duration, _context, _metrics);
            _stopped = true;
        }
        
        public void Dispose()
        {
            Stop();
        }
    }
    
    /// <summary>
    /// Static helper class for convenient logging (equivalent to C++ macros)
    /// </summary>
    public static class Log
    {
        public static void Debug(string message, Dictionary<string, object> context = null)
            => StructuredLogger.Instance.LogDebug(message, context);
            
        public static void Info(string message, Dictionary<string, object> context = null)
            => StructuredLogger.Instance.LogInfo(message, context);
            
        public static void Warning(string message, Dictionary<string, object> context = null)
            => StructuredLogger.Instance.LogWarning(message, context);
            
        public static void Error(string message, Dictionary<string, object> context = null)
            => StructuredLogger.Instance.LogError(message, context);
            
        public static void Critical(string message, Dictionary<string, object> context = null)
            => StructuredLogger.Instance.LogCritical(message, context);
        
        // Component-specific logging
        public static void Debug(string component, string message, Dictionary<string, object> context = null)
            => PxPointLogger.Instance.LogDebug(component, message, context);
            
        public static void Info(string component, string message, Dictionary<string, object> context = null)
            => PxPointLogger.Instance.LogInfo(component, message, context);
            
        public static void Warning(string component, string message, Dictionary<string, object> context = null)
            => PxPointLogger.Instance.LogWarning(component, message, context);
            
        public static void Error(string component, string message, Dictionary<string, object> context = null)
            => PxPointLogger.Instance.LogError(component, message, context);
            
        public static void Critical(string component, string message, Dictionary<string, object> context = null)
            => PxPointLogger.Instance.LogCritical(component, message, context);
        
        // Performance and activity scopes
        public static PerformanceTimer StartPerformanceTimer(string operationName, Dictionary<string, object> context = null)
            => new PerformanceTimer(operationName, context);
    }
}