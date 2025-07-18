# Contributing to GIS Shapefile Processor

Thank you for your interest in contributing to this project! This is a technical demonstration project focused on modern C++ GIS capabilities.

## Development Setup

### Prerequisites
- C++17 compatible compiler (GCC 9+, Clang 10+, MSVC 2019+)
- CMake 3.15+
- Git

### Building from Source

```bash
git clone https://github.com/tuanluongwork/gis-shapefile-processor.git
cd gis-shapefile-processor
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Code Style Guidelines

- Use modern C++17/20 features where appropriate
- Follow RAII principles for resource management
- Use smart pointers for memory management
- Prefer STL containers and algorithms
- Use const-correctness throughout
- Include comprehensive documentation

### Architecture Principles

1. **Separation of Concerns**: Core library, CLI tools, and examples are separate
2. **Performance Focus**: Memory-efficient, optimized for large datasets
3. **Cross-Platform**: Works on Windows, Linux, and macOS
4. **Standards Compliance**: Follows ESRI Shapefile specification
5. **Modern C++**: Leverages C++17/20 features for clean, efficient code

## Areas for Enhancement

### Core Library
- Additional geometry types (MultiPoint, PolylineZ, etc.)
- More coordinate system transformations
- Enhanced spatial indexing algorithms
- Streaming parsers for very large files

### Geocoding Engine
- Machine learning-based address matching
- Phonetic address matching algorithms
- International address format support
- Address validation and standardization

### Performance Optimizations
- SIMD optimizations for geometric calculations
- Memory-mapped file I/O optimizations
- Multi-threading for batch operations
- GPU acceleration for spatial queries

### Integration Features
- Python/C# bindings
- WebAssembly compilation
- Cloud platform integrations
- Real-time streaming capabilities

## Testing

```bash
# Run unit tests (if Google Test is available)
cd build
make test

# Run performance benchmarks
./bin/example-performance

# Test with sample data
./bin/shp-info ../data/sample
```

## Documentation

- Use Doxygen-style comments for all public APIs
- Include usage examples in header comments
- Update README.md for any new features
- Add performance benchmarks for new algorithms

## Submitting Changes

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## Performance Considerations

This project is designed for enterprise-scale geocoding systems. All contributions should:

- Maintain or improve performance characteristics
- Handle large datasets (>1GB shapefiles)
- Use efficient algorithms and data structures
- Include performance benchmarks for new features

## Questions?

This is a portfolio/demonstration project. For questions about the implementation or architecture, please open an issue with the "question" label.

## License

This project is released under the MIT License. See LICENSE file for details.
