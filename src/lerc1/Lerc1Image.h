/*
Copyright 2015 - 2024 Esri
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
A local copy of the license and additional notices are located with the
source distribution at:
http://github.com/Esri/lerc/
Contributors:  Thomas Maurer
               Lucian Plesea
*/

#ifndef LERC1IMAGE_H
#define LERC1IMAGE_H

#include <cstring>
#include <vector>

#ifndef NAMESPACE_LERC1_START
#define NAMESPACE_LERC1_START                                                  \
    namespace Lerc1NS                                                          \
    {
#define NAMESPACE_LERC1_END }
#define USING_NAMESPACE_LERC1 using namespace Lerc1NS;
#endif

NAMESPACE_LERC1_START
static_assert(sizeof(float) == 4, "lerc requires float to be exactly 4 bytes");

typedef unsigned char Byte;

/** BitMaskV1 - Convenient and fast access to binary mask bits
 * includes RLE compression and decompression
 */
class BitMaskV1
{
  public:
    BitMaskV1() : m_nRows(0), m_nCols(0)
    {
    }

    int size() const
    {
        return 1 + (m_nCols * m_nRows - 1) / 8;
    }

    void Set(int k, bool v)
    {
        if (v)
            SetValid(k);
        else
            SetInvalid(k);
    }

    Byte IsValid(int k) const
    {
        return (bits[k >> 3] & Bit(k)) != 0;
    }

    void resize(int nCols, int nRows)
    {
        m_nRows = nRows;
        m_nCols = nCols;
        bits.resize(size());
    }

    // max RLE compressed size is n + 4 + 2 * (n - 1) / 32767
    // Returns encoded size
    int RLEcompress(Byte *aRLE) const;
    // Encoded size, without doing the work
    int RLEsize() const;
    // Decompress a RLE bitmask, bitmask size should be already set
    // Returns false if input seems wrong
    bool RLEdecompress(const Byte *src, size_t sz);

  private:
    int m_nRows, m_nCols;
    std::vector<Byte> bits;

    static Byte Bit(int k)
    {
        return static_cast<Byte>(0x80 >> (k & 7));
    }

    void SetValid(int k)
    {
        bits[k >> 3] |= Bit(k);
    }

    void SetInvalid(int k)
    {
        bits[k >> 3] &= ~Bit(k);
    }
};

class Lerc1Image
{
  public:
    /// binary file IO with optional compression
    /// (maxZError = 0  means no lossy compression for Z; the mask part is
    /// compressed lossless or not at all) read succeeds only if maxZError on
    /// file <= maxZError requested (!)

    Lerc1Image() : width_(0), height_(0)
    {
    }

    ~Lerc1Image()
    {
    }

    bool setsize(int width, int height)
    {
        width_ = width;
        height_ = height;
        values.resize(getSize());
        return true;
    }

    int getWidth() const
    {
        return width_;
    }

    int getHeight() const
    {
        return height_;
    }

    int getSize() const
    {
        return width_ * height_;
    }

    const float& operator()(int row, int col) const
    {
        return values[row * width_ + col];
    }

    float& operator()(int row, int col)
    {
        return values[row * width_ + col];
    }

    const float* data() const
    {
        return values.data();
    }

    static unsigned int computeNumBytesNeededToWriteVoidImage();

    // Only initialize the size from the header, if LERC1
    static bool getwh(const Byte *ppByte, size_t nBytes, int &w, int &h);

    void resize(int width, int height)
    {
        setsize(width, height);
        mask.resize(getWidth(), getHeight());
    }

    bool IsValid(int row, int col) const
    {
        return mask.IsValid(row * getWidth() + col) != 0;
    }

    void SetMask(int row, int col, bool v)
    {
        mask.Set(row * getWidth() + col, v);
    }

    // Read and write LERC1 into a memory buffer, controlled by maxZError
    bool write(Byte **ppByte, double maxZError = 0) const;

    bool read(Byte **ppByte, size_t &nRemainingBytes, double maxZError);

private:
    struct InfoFromComputeNumBytes
    {
        double maxZError;
        int numTilesVertCnt;
        int numTilesHoriCnt;
        int numBytesCnt;
        float maxCntInImg;
        int numTilesVertZ;
        int numTilesHoriZ;
        int numBytesZ;
        float maxZInImg;

        InfoFromComputeNumBytes()
        {
            std::memset(this, 0, sizeof(*this));
        }
    };

    bool findTiling(double maxZError, int& numTilesVert, int& numTilesHori,
        int& numBytesOpt, float& maxValInImg) const;

    bool writeTiles(double maxZError, int numTilesVert, int numTilesHori,
        Byte* bArr, int& numBytes, float& maxValInImg) const;

    bool readTiles(double maxZErrorInFile, int numTilesVert, int numTilesHori,
        float maxValInImg, Byte* bArr, size_t nRemainingBytes);

    bool computeZStats(int r0, int r1, int c0, int c1, float& zMin, float& zMax,
        int& numValidPixel, int& numFinite) const;

    // returns true if all floating point values in the region have the same
    // binary representation
    bool isallsameval(int r0, int r1, int c0, int c1) const;

    bool writeZTile(Byte** ppByte, int& numBytes, int r0, int r1, int c0,
        int c1, int numValidPixel, float zMin, float zMax,
        double maxZError) const;

    bool readZTile(Byte** ppByte, size_t& nRemainingBytes, int r0, int r1,
        int c0, int c1, double maxZErrorInFile, float maxZInImg);

    unsigned int
        computeNumBytesNeededToWrite(double maxZError, bool onlyZPart,
            InfoFromComputeNumBytes* info) const;

    int width_, height_;
    std::vector<float> values;
    std::vector<unsigned int> idataVec;  // temporary buffer
    BitMaskV1 mask;
};

NAMESPACE_LERC1_END
#endif
