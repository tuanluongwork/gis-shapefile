<!-- Use this file to provide workspace-specific custom instructions to Copilot. For more details, visit https://code.visualstudio.com/docs/copilot/copilot-customization#_use-a-githubcopilotinstructionsmd-file -->

# GIS Shapefile Processor - Copilot Instructions

## Project Context
This is a modern C++ library for processing ESRI Shapefiles with geocoding capabilities, designed as a technical demonstration for enterprise GIS/geocoding systems.

## Code Generation Guidelines

### C++ Standards
- Use C++17/20 features (auto, range-based for, smart pointers, etc.)
- Prefer STL containers and algorithms over raw arrays/pointers
- Use RAII for all resource management
- Follow const-correctness principles
- Use modern CMake practices

### Performance Focus
- This code targets enterprise-scale geocoding systems
- Optimize for large datasets (>1GB shapefiles)
- Use efficient algorithms (R-tree indexing, memory-mapped I/O)
- Consider memory usage and cache efficiency
- Include performance benchmarks for new features

### Error Handling
- Use exceptions for exceptional conditions
- Return optional/expected types for normal failures
- Validate input parameters in public APIs
- Provide meaningful error messages

### Documentation
- Use Doxygen-style comments for all public APIs
- Include usage examples in class/function documentation
- Document performance characteristics for algorithms
- Explain spatial concepts for GIS operations

### Cross-Platform Considerations
- Ensure Windows, Linux, and macOS compatibility
- Use standard library features over platform-specific code
- Handle endianness correctly for binary file formats
- Use CMake for build configuration

### GIS-Specific Guidelines
- Follow ESRI Shapefile specification exactly
- Use proper coordinate system terminology
- Implement spatial algorithms correctly (point-in-polygon, etc.)
- Handle edge cases in geometric operations
- Consider floating-point precision issues

### Code Organization
- Separate headers and implementation files
- Use namespaces to avoid naming conflicts
- Group related functionality in logical modules
- Keep public APIs minimal and clean
- Use forward declarations to reduce compile times

### Testing and Examples
- Write comprehensive unit tests for core functionality
- Provide usage examples for all major features
- Include performance benchmarks and timing code
- Test with real-world shapefile data when possible

## Architecture Notes
- Core library (libgis-core) is the foundation
- CLI tools demonstrate practical usage
- Examples show API usage patterns
- Performance is critical for large-scale geocoding

When generating code, prioritize correctness, performance, and maintainability in that order.
