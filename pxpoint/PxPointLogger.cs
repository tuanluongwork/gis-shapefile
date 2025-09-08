using System;
using System.Collections.Generic;
using System.IO;
using System.Text.Json;
using PxPoint.Correlation;

namespace PxPoint.Logging
{
    /// <summary>
    /// PxPoint-specific logger that integrates with the correlation system
    /// and writes to /tmp/pxpoint-logs/
    /// </summary>
    public class PxPointLogger
    {
        private static readonly Lazy<PxPointLogger> _instance = 
            new Lazy<PxPointLogger>(() => new PxPointLogger());
        
        public static PxPointLogger Instance => _instance.Value;
        
        private bool _initialized = false;
        private string _processType = string.Empty;
        private StreamWriter _fileWriter;
        
        private PxPointLogger() { }
        
        // Initialize the logger for a specific process type
        public void Initialize(string processType, LogLevel logLevel = LogLevel.Info)
        {
            if (_initialized)
                return;
                
            _processType = processType;
            
            // Ensure log directory exists
            Directory.CreateDirectory("/tmp/pxpoint-logs");
            
            // Create log filename with process type and timestamp
            var timestamp = DateTimeOffset.UtcNow.ToUnixTimeSeconds();
            var filename = $"/tmp/pxpoint-logs/pxpoint-{processType}-{timestamp}.log";
            
            try
            {
                _fileWriter = new StreamWriter(filename, append: true, bufferSize: 1024)
                {
                    AutoFlush = true
                };
                
                _initialized = true;
                
                // Log initialization
                LogWithContext(LogLevel.Info, "Logger", "PxPoint logger initialized", 
                    new Dictionary<string, object>
                    {
                        {"process_type", processType},
                        {"log_file", filename}
                    });
            }
            catch (Exception e)
            {
                throw new InvalidOperationException($"Failed to initialize PxPoint logger: {e.Message}", e);
            }
        }
        
        // Log with context and correlation
        public void LogWithContext(LogLevel level, string component, string message,
            Dictionary<string, object> context = null, Dictionary<string, double> performance = null)
        {
            if (!_initialized)
                throw new InvalidOperationException("PxPoint logger not initialized");
            
            var timestamp = DateTime.UtcNow.ToString("yyyy-MM-ddTHH:mm:ss.fffZ");
            var correlationId = PxPointCorrelationManager.Instance.GetFullCorrelationId();
            
            // Create structured log entry
            var logEntry = new
            {
                timestamp = timestamp,
                level = level.ToString().ToUpper(),
                process = _processType,
                component = component,
                message = message,
                correlation = correlationId,
                context = context ?? new Dictionary<string, object>(),
                performance = performance ?? new Dictionary<string, double>()
            };
            
            var jsonLog = JsonSerializer.Serialize(logEntry, new JsonSerializerOptions
            {
                WriteIndented = false,
                PropertyNamingPolicy = JsonNamingPolicy.CamelCase
            });
            
            // Write to file
            _fileWriter.WriteLine(jsonLog);
            
            // Also write to console for immediate feedback
            var consoleMessage = $"[{timestamp}] [{level}] [{component}] {message}";
            if (!string.IsNullOrEmpty(correlationId))
                consoleMessage += $" | correlation:{correlationId}";
            
            Console.WriteLine(consoleMessage);
        }
        
        // Log process milestone events
        public void LogProcessStart(string processType, Dictionary<string, object> config = null)
        {
            var context = config ?? new Dictionary<string, object>();
            context["event_type"] = "process_start";
            context["timestamp"] = DateTime.UtcNow.ToString("yyyy-MM-ddTHH:mm:ss.fffZ");
            
            LogWithContext(LogLevel.Info, "Process", $"Process started: {processType}", context);
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
            
            LogWithContext(logLevel, "Process", message, context, metrics);
        }
        
        // Log activity events
        public void LogActivityStart(string activityName, Dictionary<string, object> context = null)
        {
            var activityContext = context ?? new Dictionary<string, object>();
            activityContext["event_type"] = "activity_start";
            activityContext["timestamp"] = DateTime.UtcNow.ToString("yyyy-MM-ddTHH:mm:ss.fffZ");
            
            LogWithContext(LogLevel.Debug, "Activity", $"Activity started: {activityName}", activityContext);
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
            
            LogWithContext(logLevel, "Activity", message, context, metrics);
        }
        
        // Log error with full context
        public void LogError(string component, string message, Exception exception = null,
            Dictionary<string, object> context = null)
        {
            var errorContext = context ?? new Dictionary<string, object>();
            errorContext["event_type"] = "error";
            errorContext["timestamp"] = DateTime.UtcNow.ToString("yyyy-MM-ddTHH:mm:ss.fffZ");
            
            if (exception != null)
            {
                errorContext["exception"] = exception.ToString();
            }
            
            var errorMessage = $"ERROR: {message}";
            if (exception != null)
            {
                errorMessage += $" | Exception: {exception.Message}";
            }
            
            LogWithContext(LogLevel.Error, component, errorMessage, errorContext);
        }
        
        // Convenience methods
        public void LogInfo(string component, string message, Dictionary<string, object> context = null)
        {
            LogWithContext(LogLevel.Info, component, message, context);
        }
        
        public void LogWarning(string component, string message, Dictionary<string, object> context = null)
        {
            LogWithContext(LogLevel.Warning, component, message, context);
        }
        
        public void LogDebug(string component, string message, Dictionary<string, object> context = null)
        {
            LogWithContext(LogLevel.Debug, component, message, context);
        }
        
        // Shutdown
        public void Shutdown()
        {
            _fileWriter?.Flush();
            _fileWriter?.Close();
            _fileWriter?.Dispose();
        }
    }
    
    public enum LogLevel
    {
        Debug,
        Info,
        Warning,
        Error
    }
}