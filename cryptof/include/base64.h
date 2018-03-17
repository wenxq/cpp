
#include "interface.h"
#include "common/base64.h"

class Base64Crypto : public Crypto
{
public:
	Base64Crypto()
	  : mEncodeOut(0)
	  , mDecodeOut(0)
	{}

	virtual int type() const
	{
		return static_cast<int>(eCrypto::kBase64);
	}

	virtual const char* name() const
	{
		return "BASE64";
	}

	// encode:
	virtual int encode_outsize() const
	{
		return mEncodeOut;
	}

	virtual int encode_length(int len) const
	{
		return (len + 2) / 3 * 4;
	}

	virtual int encode_blocksize() const
	{
		//return integer_cast<int>(lcm(4, 3));
		return 12;
	}

	virtual void encode(const u_char* input, u_char* output)
	{
		mEncodeOut = base64_encode((char *)output, (const char *)input, encode_blocksize());
		SMART_ASSERT(mEncodeOut > 0);
	}

	// decode:
	virtual int decode_outsize() const
	{
		return mDecodeOut;
	}

	virtual int decode_length(int len) const
	{
		return len / 4 * 3;
	}

	virtual int decode_blocksize() const
	{
		return encode_length(encode_blocksize());
	}

	virtual void decode(const u_char* input, u_char* output)
	{
		mDecodeOut = base64_decode((char *)output, (const char *)input, decode_blocksize());
		SMART_ASSERT(mDecodeOut > 0);
	}

private:
	int mEncodeOut;
	int mDecodeOut;
};
