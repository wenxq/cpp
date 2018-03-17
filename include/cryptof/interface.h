
#ifndef _INTERFACT_H_
#define _INTERFACT_H_

#include "common/noncopyable.h"
#include "common/smart_assert.h"
#include "common/integer_cast.h"
#include "common/slice.h"
#include "common/tools.h"
#include "common/string_algo.h"
#include <vector>

#ifndef _HAVE_CXX11_
#include <tr1/memory>
#else
#include <memory>
#endif // _HAVE_CXX11_

typedef unsigned char u_char;

class Crypto : boost::noncopyable
{
public:
	Crypto() {}
	virtual ~Crypto() {}
	virtual int type() const = 0;
	virtual const char* name() const = 0;

	// encode:
	virtual int  encode_blocksize() const = 0;
	virtual int  encode_length(int len) const = 0;
	virtual int  encode_outsize() const = 0;
	virtual void encode(const u_char* input, u_char* output) = 0;

	// decode:
	virtual int  decode_blocksize() const = 0;
	virtual int  decode_length(int len) const = 0;
	virtual int  decode_outsize() const = 0;
	virtual void decode(const u_char* input, u_char* output) = 0;
};

class CryptoProxy : boost::noncopyable
{
public:
	CryptoProxy(Crypto* coder)
	  : mCoder(coder)
	  , mBuffer(nullptr)
      , mBlockSize(mCoder->encode_blocksize())
      , mBlockOut(mCoder->encode_length(mBlockSize))
	{
		SMART_ASSERT(mCoder != nullptr);
	}

	int size() const
	{
		return integer_cast<int>(mStore.size());
	}

	Slice encode(const u_char* data, int len)
	{
		SMART_ASSERT(len > 0);

		const int in_block  = mBlockSize;
		const int out_block = mBlockOut;
		const int blocks    = len / in_block;
		const int remain    = len % in_block;
		
		int need = out_block * blocks;
		if (remain > 0)
		{
			need += out_block + in_block;
		}

		if (size() < need)
		{
			mStore.resize(integer_cast<size_t>(need));
			mBuffer = &(mStore[0]);
		}

		const u_char* src = data;
		u_char* dest = mBuffer;
		SMART_ASSERT(src != nullptr && dest != nullptr);
		for (int i = 0; i < blocks; ++i)
		{
			mCoder->encode(src, dest);
			src  += in_block;
			dest += out_block;
		}

		if (remain > 0)
		{
			u_char* temp = dest + in_block;
			memcpy(temp, src, remain);
			memset(temp + remain, 0, in_block - remain);
			mCoder->encode(temp, dest);
			dest += out_block;
		}

		return make_slice(mBuffer, dest);
	}

	Slice encode(const Slice& str)
	{
		return encode((const u_char *)str.data(), str.size());
	}

	Slice decode(const u_char* data, int len)
	{
		const int blocksize = mCoder->decode_blocksize();
		const int blocks = len / blocksize;

		//SMART_ASSERT(len > 0 && (len % mCoder->decode_blocksize() == 0));

		int need = mCoder->decode_length(len);
		if (size() < need)
		{
			mStore.resize(integer_cast<size_t>(need));
			mBuffer = &(mStore[0]);
		}

		const u_char* src = data;
		u_char* dest = mBuffer;
		SMART_ASSERT(src != nullptr && dest != nullptr);
		for (int i = 0; i < blocks; ++i)
		{
			mCoder->decode(src, dest);
			src  += blocksize;
			dest += mCoder->decode_outsize();
		}

		return make_slice(mBuffer, dest);
	}

	Slice decode(const Slice& str)
	{
		return decode((const u_char *)str.data(), str.size());
	}

	const Crypto* operator->() const
	{
		return mCoder.get();
	}

private:
	std::shared_ptr<Crypto> mCoder;
	std::vector<u_char>     mStore;
	u_char*                 mBuffer;
    int                     mBlockSize;
    int                     mBlockOut;
};

typedef std::shared_ptr<CryptoProxy> CryptoProxyPtr;

// greatest common divisor by Euclid:
inline int64_t gcd(int64_t a, int64_t b)
{
	SMART_ASSERT(a > 0 && b > 0);

	if (a < b) { std::swap(a, b); }

	int64_t tmp = 0;
	while (b != 0)
	{
		tmp = a % b;
		a = b;
		b = tmp;
	}

	return a;
}

// greatest common divisor by Stein:
inline int64_t gcd_stein(int64_t a, int64_t b)
{
	SMART_ASSERT(a > 0 && b >= 0)("a", a)("b", b);

	if (a < b) { std::swap(a, b); }

	if (b == 0) return a;

	const int64_t ar = a % 2;
	const int64_t br = b % 2;

	if (ar == 0 && br == 0)
		return 2 * gcd_stein(a >> 1, b >> 1);
	else if (ar == 0)
		return gcd_stein(a >> 1, b);
	else if (br == 0)
		return gcd_stein(a, b >> 1);

	return gcd_stein((a + b) >> 1, (a - b) >> 1);
}

// least common multiple:
inline int64_t lcm(const int64_t& a, const int64_t& b)
{
	return (a * b) / gcd(a, b);
}

#define PIPE_SEP "->"

class CryptoPipeline
{
public:
	template <class CreateCoder>
	CryptoPipeline(const std::string& desc, const Slice& key, const CreateCoder& create)
		: mReadBSize(1)
		, mWriteBSize(1)
	{
		std::vector<Slice> codelist;
		split(codelist, desc, PIPE_SEP);
		for (BOOST_AUTO(iter, codelist.begin()); iter != codelist.end(); ++iter)
		{
			iter->trim();
			Crypto* coder = create(*iter, key);
			CryptoProxyPtr pipe(new CryptoProxy(coder));
			SMART_ASSERT((*pipe)->encode_blocksize() > 0);
			mPipeList.push_back(pipe);
			mReadBSize  = integer_cast<int>(lcm(mReadBSize,  (*pipe)->encode_blocksize()));
			mWriteBSize = integer_cast<int>(lcm(mWriteBSize, (*pipe)->encode_length(mReadBSize)));
		}
	}

	int read_bsize()  const { return mReadBSize;  }
	int write_bsize() const { return mWriteBSize; }

	Slice encode(Slice src)
	{
		for (BOOST_AUTO(pipe, mPipeList.begin()); pipe != mPipeList.end(); ++pipe)
		{
			src = (*pipe)->encode(src);
		}

		return src;
	}

	Slice encode(const u_char* data, int len)
	{
		return encode(make_slice(data, len));
	}

	Slice decode(Slice src)
	{
		for (BOOST_AUTO(pipe, mPipeList.rbegin()); pipe != mPipeList.rend(); ++pipe)
		{
			src = (*pipe)->decode(src);
		}

		return src;
	}

	Slice decode(const u_char* data, int len)
	{
		return decode(make_slice(data, len));
	}

private:
	std::vector<CryptoProxyPtr> mPipeList;
	int mReadBSize;
	int mWriteBSize;
};

#endif // _INTERFACT_H_
