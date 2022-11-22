# Compile-time strings

Simple fixed-size string library for encoding strings at compile time.

Works out of the box in templates: e.g., `template <fixed::String Name> class ...`;
no need to manually declare the size of the string.

Bounds-checking at runtime by default; define `NDEBUG` (which will disable assertions across the entire translation unit) or `FS_NOASSERT` (for this library only) to bypass them.

## Requirements

C++20, for class-type non-type template parameters.

## Usage

This is a header-only "library" with no dependencies;
simply `#include` it and you're good to go!
