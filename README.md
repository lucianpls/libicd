# libicd
image codec library

Provides a uniform API to multiple raster codecs. It supports the following raster formats:

- JPEG  : libicd includes jpeg 12bit sources, and uses system provided jpeg 8 library. Supports the JPEG Zen extension (zero mask)
- PNG   : Uses system provided PNG
- LERC1 : Rewrite of LERC1 for floating point rasters and mask
- QB3   : Integer lossless compression, optional, use -DUSE_QB3=ON as an argument to cmake

# Building notes
- QB3 utilities depend on libicd, but libQB3 itself does not. To build libicd with QB3 support and the QB3 command line utility, follow these steps:
  -  Build and install libQB3 by itself first (the default)
  -  Build and install libicd with -DUSE_QB3=ON
  -  Reconfigure, rebuild and install QB3 with -DBUILD_CQB3=ON
