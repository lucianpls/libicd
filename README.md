# libicd
image codec library

Provides a standard API to multiple raster codecs

- JPEG  : Uses system provided jpeg 8 library, include jpeg 12bit. Also includes Zen extension (zero mask) support
- PNG   : Uses system provided PNG
- LERC1 : Rewrite of LERC1 for floating point rasters and mask
- QB3   : Optional, use -DUSE_QB3=ON as an argument to cmake

# Building notes
- QB3 utilities depend on libicd, but libQB3 itself does not. To fully build the QB3 utilities:
  -  Build and install libQB3 by itself first (the default)
  -  Build and install libicd with -DUSE_QB3=ON
  -  Reconfigure, rebuild and install QB3 with -DBUILD_CQB3=ON
