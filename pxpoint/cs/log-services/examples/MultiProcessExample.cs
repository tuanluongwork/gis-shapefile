using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Threading.Tasks;
using LogServices.Correlation;
using LogServices.Logging;

namespace LogServices.Examples
{
    /// <summary>
    /// Example demonstrating multi-process correlation similar to PxPoint workflow
    /// </summary>
    public class MultiProcessExample
    {
        public static async Task Main(string[] args)
        {
            // Check if this is a child process
            if (args.Length > 0 && args[0] == "child")
            {
                await RunChildProcess(args);
                return;
            }
            
            // This is the parent/orchestrator process
            await RunOrchestratorProcess();
        }
        
        /// <summary>
        /// Main orchestrator process that spawns child processes
        /// </summary>
        static async Task RunOrchestratorProcess()
        {
            Console.WriteLine("=== Multi-Process Orchestrator Example ===");
            
            // Initialize logger for orchestrator
            var logger = StructuredLogger.Instance;
            logger.Initialize("Orchestrator", LogLevel.Info);
            
            // Create process scope - this generates the pipeline ID
            using var processScope = new ProcessCorrelationScope("Orchestrator");
            
            logger.LogInfo("Orchestrator", "Starting multi-process workflow");
            
            // Simulate multiple phases with child processes
            await SimulateDataIngestionPhase(logger);
            await SimulateProcessingPhase(logger);
            await SimulateOutputPhase(logger);
            
            logger.LogInfo("Orchestrator", "Multi-process workflow completed successfully");
            logger.Shutdown();
        }
        
        /// <summary>
        /// Child process that inherits correlation context
        /// </summary>
        static async Task RunChildProcess(string[] args)
        {
            var processType = args.Length > 1 ? args[1] : "Worker";
            var taskName = args.Length > 2 ? args[2] : "DefaultTask";
            
            // Load correlation context from environment (set by parent)
            CorrelationManager.Instance.LoadFromEnvironment();
            
            // Initialize logger for child process
            var logger = StructuredLogger.Instance;
            logger.Initialize(processType, LogLevel.Info);
            
            // Create process scope for this child
            using var processScope = new ProcessCorrelationScope(processType);
            
            logger.LogInfo("ChildProcess", $"Child process {processType} started", new Dictionary<string, object>
            {
                {"task_name", taskName},
                {"parent_pipeline", CorrelationManager.Instance.GetPipelineId()}
            });
            
            // Simulate work with activities
            using var activityScope = new ActivityCorrelationScope(taskName);
            
            // Simulate some work
            logger.LogInfo("Activity", $"Starting {taskName} processing");
            
            using var timer = new PerformanceTimer(taskName, new Dictionary<string, object>
            {
                {"process_type", processType},
                {"child_process", true}
            });
            
            // Simulate processing time
            await Task.Delay(Random.Shared.Next(100, 500));
            
            logger.LogInfo("Activity", $"Completed {taskName} processing", new Dictionary<string, object>
            {
                {"success", true},
                {"records_processed", Random.Shared.Next(100, 1000)}
            });
            
            logger.LogInfo("ChildProcess", $"Child process {processType} completed successfully");
            logger.Shutdown();
        }
        
        /// <summary>
        /// Simulate data ingestion phase with child processes
        /// </summary>
        static async Task SimulateDataIngestionPhase(StructuredLogger logger)
        {
            using var activityScope = new ActivityCorrelationScope("DataIngestion");
            
            logger.LogInfo("Phase", "Starting data ingestion phase");
            
            // Spawn multiple data ingestion workers
            var tasks = new List<Task>();
            var dataSources = new[] { "ParcelData", "AddressData", "GeoData" };
            
            foreach (var dataSource in dataSources)
            {
                tasks.Add(SpawnChildProcess($"Ingestion-{dataSource}", $"Ingest{dataSource}"));
            }
            
            // Wait for all ingestion processes to complete
            await Task.WhenAll(tasks);
            
            logger.LogInfo("Phase", "Data ingestion phase completed", new Dictionary<string, object>
            {
                {"data_sources_processed", dataSources.Length},
                {"phase_duration_ms", activityScope.GetContext().GetValueOrDefault("duration_ms", 0)}
            });
        }
        
        /// <summary>
        /// Simulate processing phase with parallel workers
        /// </summary>
        static async Task SimulateProcessingPhase(StructuredLogger logger)
        {
            using var activityScope = new ActivityCorrelationScope("DataProcessing");
            
            logger.LogInfo("Phase", "Starting data processing phase");
            
            // Spawn processing workers for different regions/batches
            var tasks = new List<Task>();
            var regions = new[] { "North", "South", "East", "West" };
            
            foreach (var region in regions)
            {
                tasks.Add(SpawnChildProcess($"Processor-{region}", $"Process{region}Region"));
            }
            
            // Wait for all processing to complete
            await Task.WhenAll(tasks);
            
            logger.LogInfo("Phase", "Data processing phase completed", new Dictionary<string, object>
            {
                {"regions_processed", regions.Length},
                {"parallel_workers", tasks.Count}
            });
        }
        
        /// <summary>
        /// Simulate output phase with child processes
        /// </summary>
        static async Task SimulateOutputPhase(StructuredLogger logger)
        {
            using var activityScope = new ActivityCorrelationScope("OutputGeneration");
            
            logger.LogInfo("Phase", "Starting output generation phase");
            
            // Spawn output generators
            var tasks = new List<Task>
            {
                SpawnChildProcess("ReportGenerator", "GenerateReports"),
                SpawnChildProcess("FileExporter", "ExportFiles"),
                SpawnChildProcess("NotificationSender", "SendNotifications")
            };
            
            // Wait for all output processes to complete
            await Task.WhenAll(tasks);
            
            logger.LogInfo("Phase", "Output generation phase completed");
        }
        
        /// <summary>
        /// Spawn a child process with correlation context
        /// </summary>
        static async Task SpawnChildProcess(string processType, string taskName)
        {
            try
            {
                // Get current executable path
                var currentProcess = Process.GetCurrentProcess();
                var executablePath = currentProcess.MainModule?.FileName ?? "dotnet";
                var args = $"run --project . child {processType} {taskName}";
                
                var startInfo = new ProcessStartInfo
                {
                    FileName = executablePath.EndsWith(".exe") ? executablePath : "dotnet",
                    Arguments = executablePath.EndsWith(".exe") ? $"child {processType} {taskName}" : args,
                    UseShellExecute = false,
                    RedirectStandardOutput = true,
                    RedirectStandardError = true
                };
                
                // Correlation context is automatically propagated via environment variables
                // by the ProcessCorrelationScope
                
                using var process = new Process { StartInfo = startInfo };
                process.Start();
                
                // Optionally capture output
                var output = await process.StandardOutput.ReadToEndAsync();
                var error = await process.StandardError.ReadToEndAsync();
                
                await process.WaitForExitAsync();
                
                if (process.ExitCode != 0)
                {
                    throw new InvalidOperationException($"Child process {processType} failed with exit code {process.ExitCode}. Error: {error}");
                }
                
                // Log successful completion
                Log.Info("ProcessManager", $"Child process {processType} completed successfully", new Dictionary<string, object>
                {
                    {"process_type", processType},
                    {"task_name", taskName},
                    {"exit_code", process.ExitCode}
                });
            }
            catch (Exception ex)
            {
                Log.Error("ProcessManager", $"Failed to spawn child process {processType}", new Dictionary<string, object>
                {
                    {"process_type", processType},
                    {"task_name", taskName},
                    {"error", ex.Message}
                });
                throw;
            }
        }
    }
}