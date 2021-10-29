#include "PRNG.h"

#include <algorithm>
#include <cstring>

PRNG::PRNG(const block& seed, uint64_t bufferSize)
    :
    mBytesIdx(0),
    mBlockIdx(0)
{
    SetSeed(seed, bufferSize);
}

PRNG::PRNG(PRNG && s) :
    mBuffer(std::move(s.mBuffer)),
    mAes(std::move(s.mAes)),
    mBytesIdx(s.mBytesIdx),
    mBlockIdx(s.mBlockIdx),
    mBufferByteCapacity(s.mBufferByteCapacity)
{
    s.mBuffer.resize(0);
    memset(&s.mAes, 0, sizeof(AES));
    s.mBytesIdx = 0;
    s.mBlockIdx = 0;
    s.mBufferByteCapacity = 0;
}


void PRNG::SetSeed(const block& seed, uint64_t bufferSize)
{
    mAes.setKey(seed);
    mBlockIdx = 0;

    if (mBuffer.size() == 0)
    {
        mBuffer.resize(bufferSize);
        mBufferByteCapacity = (sizeof(block) * bufferSize);
    }


    refillBuffer();
}

uint8_t PRNG::getBit() { return get<bool>(); }

const block PRNG::getSeed() const
{
    if(mBuffer.size())
        return mAes.key;

    throw std::runtime_error("PRNG has not been keyed " LOCATION);
}

void PRNG::refillBuffer()
{
    if (mBuffer.size() == 0)
        throw std::runtime_error("PRNG has not been keyed " LOCATION);

    mAes.encryptCTR(mBlockIdx, mBuffer.size(), mBuffer.data());
    mBlockIdx += mBuffer.size();
    mBytesIdx = 0;
}
